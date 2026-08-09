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
    set_target_properties(lua_static PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
endif()
if(WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Oi-")
endif()
include(FetchContent)
FetchContent_Declare(
    ixwebsocket
    GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
    GIT_TAG        v12.0.1
)

FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)


set(USE_TLS ON CACHE BOOL "" FORCE)
if(WIN32)
    set(USE_MBED_TLS ON CACHE BOOL "" FORCE)
    set(USE_OPEN_SSL OFF CACHE BOOL "" FORCE)
    set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
    add_subdirectory(external/mbedtls)
endif()
if(WIN32)
    set(MBEDTLS_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/external/mbedtls/include" CACHE PATH "" FORCE)
    set(MBEDTLS_LIBRARY mbedtls CACHE STRING "" FORCE)
    set(MBEDX509_LIBRARY mbedx509 CACHE STRING "" FORCE)
    set(MBEDCRYPTO_LIBRARY mbedcrypto CACHE STRING "" FORCE)
endif()

add_subdirectory(external/zlib)
set(ZLIB_LIBRARY zlibstatic CACHE STRING "" FORCE)
set(ZLIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/zlib" "${CMAKE_CURRENT_BINARY_DIR}/external/zlib" CACHE STRING "" FORCE)
set(IXWEBSOCKET_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(ixwebsocket nlohmann_json)

find_package(LibArchive QUIET)
if(NOT LibArchive_FOUND)
    message(STATUS "libarchive not found via find_package, fetching from source...")
    set(HAVE_WCSCPY 0 CACHE INTERNAL "" FORCE)
    set(HAVE_WCSLEN 0 CACHE INTERNAL "" FORCE)
    set(ENABLE_TEST OFF CACHE BOOL "" FORCE)
    set(ENABLE_TAR OFF CACHE BOOL "" FORCE)
    set(ENABLE_CPIO OFF CACHE BOOL "" FORCE)
    set(ENABLE_CAT OFF CACHE BOOL "" FORCE)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    FetchContent_Declare(
        libarchive_src
        GIT_REPOSITORY https://github.com/libarchive/libarchive.git
        GIT_TAG        v3.7.4
    )
    set(HAVE_WCSCPY 1 CACHE INTERNAL "" FORCE)
set(HAVE_WCSLEN 1 CACHE INTERNAL "" FORCE)
    FetchContent_MakeAvailable(libarchive_src)
    if(WIN32)
    target_compile_options(archive_static PRIVATE /wd2491 /wd2169)
endif()
    if(NOT TARGET LibArchive::LibArchive)
        add_library(LibArchive::LibArchive ALIAS archive_static)
    endif()
endif()

file(GLOB_RECURSE SOURCES "src/*.cpp")

if(NOT SOURCES)
    set(SOURCES src/library.cpp)
endif()

if(WIN32)
    add_compile_definitions(NOMINMAX)
endif()

add_library(HPR-Store SHARED ${SOURCES})

target_link_libraries(HPR-Store PRIVATE Slint::Slint)
target_link_libraries(HPR-Store PRIVATE ixwebsocket)

if(WIN32)
    target_link_libraries(HPR-Store PRIVATE crypt32 ws2_32 secur32 mbedtls mbedx509 mbedcrypto)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/main.slint")
    slint_target_sources(HPR-Store src/ui/main.slint)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ui/main.slint")
    slint_target_sources(HPR-Store ui/main.slint)
endif()

set_target_properties(HPR-Store PROPERTIES
    SLINT_EMBED_RESOURCES embed-files
)

target_include_directories(lua_static PUBLIC external/lua/)
target_include_directories(HPR-Store PUBLIC external/lua/)
target_include_directories(HPR-Store PRIVATE external/sol2/)

target_link_libraries(HPR-Store PRIVATE
    lua_static
    nlohmann_json::nlohmann_json
    LibArchive::LibArchive
)

target_include_directories(HPR-Store
    PRIVATE
    include/
)

if(WIN32)
    add_custom_command(TARGET HPR-Store POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:HPR-Store> $<TARGET_FILE_DIR:HPR-Store>
        COMMAND_EXPAND_LISTS
    )
endif()

if(UNIX)
    find_library(SLINT_SO
        NAMES slint_cpp
        HINTS "${Slint_DIR}/../.."
              "${Slint_DIR}/../../.."
    )
    if(SLINT_SO)
        add_custom_command(TARGET HPR-Store POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${SLINT_SO}"
                $<TARGET_FILE_DIR:HPR-Store>
        )
    endif()
endif()

add_custom_command(TARGET HPR-Store POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/src/HPR-Store.lua"
        "$<TARGET_FILE_DIR:HPR-Store>/HPR-Store.lua"
)

if(WIN32)
    set(HPR_EXTENSION_DIR "$ENV{APPDATA}/HPR/HPR_Config/extensions/HPR-Store")
else()
    set(HPR_EXTENSION_DIR "$ENV{HOME}/.config/HPR/extensions/HPR-Store")
endif()

add_custom_command(TARGET HPR-Store POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${HPR_EXTENSION_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:HPR-Store>" "${HPR_EXTENSION_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE_DIR:HPR-Store>/HPR-Store.lua" "${HPR_EXTENSION_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/registry.json" "${HPR_EXTENSION_DIR}/registry.json"
    COMMAND_EXPAND_LISTS
)

add_custom_command(TARGET HPR-Store POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/registry.json"
        "$<TARGET_FILE_DIR:HPR-Store>/registry.json"
)

if(WIN32)
    add_custom_command(TARGET HPR-Store POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:HPR-Store>
            "${HPR_EXTENSION_DIR}/"
        COMMAND_EXPAND_LISTS
    )
elseif(UNIX AND SLINT_SO)
    add_custom_command(TARGET HPR-Store POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SLINT_SO}"
            "${HPR_EXTENSION_DIR}/"
    )
endif()