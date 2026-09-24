cmake_minimum_required(VERSION 3.21)

project(HPR-Store)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BUILD_PARALLEL_LEVEL 8)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(WIN32)
    list(APPEND CMAKE_PREFIX_PATH "C:/Program Files/Slint-cpp 1.16.1")
endif()
find_package(Slint REQUIRED)

file(GLOB_RECURSE LUA_SOURCES "external/lua/*.c")
list(FILTER LUA_SOURCES EXCLUDE REGEX ".*(lua|luac)\\.c$")
add_library(lua_static STATIC ${LUA_SOURCES})

if(UNIX)
    set_target_properties(lua_static PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
if(MSVC)
    add_compile_options(/MP)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Oi-")
endif()

include(FetchContent)

# ── libcurl (built from source) ───────────────────────────────────────────────
set(BUILD_STATIC_LIBS ON  CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_CURL_EXE    OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING     OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES    OFF CACHE BOOL "" FORCE)
set(BUILD_MISC_DOCS   OFF CACHE BOOL "" FORCE)
set(ENABLE_CURL_MANUAL OFF CACHE BOOL "" FORCE)
set(HTTP_ONLY         ON  CACHE BOOL "" FORCE)
set(CURL_DISABLE_LDAP ON  CACHE BOOL "" FORCE)
set(CURL_DISABLE_LDAPS ON CACHE BOOL "" FORCE)
set(CURL_STATICLIB    ON  CACHE BOOL "" FORCE)
set(CURL_USE_LIBPSL   OFF CACHE BOOL "" FORCE)
set(CURL_USE_LIBSSH2  OFF CACHE BOOL "" FORCE)

if(WIN32)
    set(CURL_USE_SCHANNEL  ON  CACHE BOOL "" FORCE)
    set(CURL_USE_OPENSSL   OFF CACHE BOOL "" FORCE)
    set(CURL_USE_MBEDTLS   OFF CACHE BOOL "" FORCE)
else()
    set(CURL_USE_OPENSSL   ON  CACHE BOOL "" FORCE)
endif()

FetchContent_Declare(
    curl
    URL https://github.com/curl/curl/releases/download/curl-8_11_1/curl-8.11.1.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# ── nlohmann/json ─────────────────────────────────────────────────────────────
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(curl nlohmann_json)

# ── LibArchive & zlib (deflate decompression) ─────────────────────────────────
find_package(LibArchive QUIET)
if(NOT LibArchive_FOUND)
    message(STATUS "libarchive not found via find_package, fetching from source...")
    
    # zlib is required by libarchive to decompress ZIP files (compression method 8: deflation)
    set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        zlib
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG        v1.3.1
    )
    FetchContent_MakeAvailable(zlib)
    if(TARGET zlibstatic)
        set(ZLIB_INCLUDE_DIR "${zlib_SOURCE_DIR};${zlib_BINARY_DIR}" CACHE PATH "" FORCE)
        set(ZLIB_LIBRARY zlibstatic CACHE STRING "" FORCE)
        set(ZLIB_FOUND TRUE CACHE BOOL "" FORCE)
        if(NOT TARGET ZLIB::ZLIB)
            add_library(ZLIB::ZLIB ALIAS zlibstatic)
        endif()
    endif()

    set(ENABLE_TEST  OFF CACHE BOOL "" FORCE)
    set(ENABLE_TAR   OFF CACHE BOOL "" FORCE)
    set(ENABLE_CPIO  OFF CACHE BOOL "" FORCE)
    set(ENABLE_CAT   OFF CACHE BOOL "" FORCE)
    set(ENABLE_ZLIB  ON  CACHE BOOL "" FORCE)
    set(HAVE_WCSCPY  1   CACHE INTERNAL "" FORCE)
    set(HAVE_WCSLEN  1   CACHE INTERNAL "" FORCE)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    FetchContent_Declare(
        libarchive_src
        GIT_REPOSITORY https://github.com/libarchive/libarchive.git
        GIT_TAG        v3.7.4
    )
    FetchContent_MakeAvailable(libarchive_src)
    if(WIN32)
        target_compile_options(archive_static PRIVATE /wd4291 /wd4267)
    endif()
    if(NOT TARGET LibArchive::LibArchive)
        add_library(LibArchive::LibArchive ALIAS archive_static)
    endif()
endif()

# ── Main target ───────────────────────────────────────────────────────────────
file(GLOB_RECURSE SOURCES "src/*.cpp")
if(NOT SOURCES)
    set(SOURCES src/library.cpp)
endif()

if(WIN32)
    add_compile_definitions(NOMINMAX)
endif()

add_library(HPR-Store SHARED ${SOURCES})

target_link_libraries(HPR-Store PRIVATE
    Slint::Slint
    lua_static
    nlohmann_json::nlohmann_json
    LibArchive::LibArchive
)
if(TARGET zlibstatic)
    target_link_libraries(HPR-Store PRIVATE zlibstatic)
endif()

if(WIN32)
    target_compile_definitions(HPR-Store PRIVATE CURL_STATICLIB)
    target_link_libraries(HPR-Store PRIVATE libcurl_static ws2_32 crypt32 wldap32 normaliz)
else()
    target_link_libraries(HPR-Store PRIVATE libcurl_static)
endif()
add_dependencies(HPR-Store libcurl_static)
if(TARGET archive_static)
    add_dependencies(HPR-Store archive_static)
endif()
if(TARGET zlibstatic)
    add_dependencies(HPR-Store zlibstatic)
endif()

set_target_properties(HPR-Store PROPERTIES
    SLINT_EMBED_RESOURCES embed-files
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/main.slint")
    slint_target_sources(HPR-Store src/ui/main.slint)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ui/main.slint")
    slint_target_sources(HPR-Store ui/main.slint)
endif()

target_include_directories(lua_static PUBLIC external/lua/)
target_include_directories(HPR-Store PUBLIC  external/lua/)
target_include_directories(HPR-Store PRIVATE external/sol2/)
target_include_directories(HPR-Store PRIVATE ${curl_SOURCE_DIR}/include)
target_include_directories(HPR-Store PRIVATE include/)
target_compile_definitions(HPR-Store PRIVATE LIBARCHIVE_STATIC)

# ── Post-build copies ─────────────────────────────────────────────────────────
if(WIN32)
    set(HPR_EXTENSION_DIR "$ENV{APPDATA}/HPR/HPR_Config/extensions/HPR-Store")
else()
    set(HPR_EXTENSION_DIR "$ENV{HOME}/.config/HPR/extensions/HPR-Store")
endif()

add_custom_command(TARGET HPR-Store POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/src/HPR-Store.lua"
        "$<TARGET_FILE_DIR:HPR-Store>/HPR-Store.lua"
)

add_custom_command(TARGET HPR-Store POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${HPR_EXTENSION_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:HPR-Store>"                   "${HPR_EXTENSION_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE_DIR:HPR-Store>/HPR-Store.lua" "${HPR_EXTENSION_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/registry.json"  "${HPR_EXTENSION_DIR}/registry.json"
    COMMAND_EXPAND_LISTS
)

add_custom_command(TARGET HPR-Store POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/registry.json"
        "$<TARGET_FILE_DIR:HPR-Store>/registry.json"
)

if(WIN32)
    add_custom_command(TARGET HPR-Store POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:HPR-Store> $<TARGET_FILE_DIR:HPR-Store>
        COMMAND_EXPAND_LISTS
    )
    add_custom_command(TARGET HPR-Store POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:HPR-Store>
            "${HPR_EXTENSION_DIR}/"
        COMMAND_EXPAND_LISTS
    )
endif()

if(UNIX)
    find_library(SLINT_SO
        NAMES slint_cpp
        HINTS "${Slint_DIR}/../.." "${Slint_DIR}/../../.."
    )
    if(SLINT_SO)
        add_custom_command(TARGET HPR-Store POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SLINT_SO}" $<TARGET_FILE_DIR:HPR-Store>
        )
        add_custom_command(TARGET HPR-Store POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SLINT_SO}" "${HPR_EXTENSION_DIR}/"
        )
    endif()
endif()