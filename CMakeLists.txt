cmake_minimum_required(VERSION 3.21)

project(HPR-Store)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BUILD_PARALLEL_LEVEL 8)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Find Slint (only dependency required)
find_package(Slint REQUIRED)

file(GLOB_RECURSE LUA_SOURCES "external/lua/*.c")
list(FILTER LUA_SOURCES EXCLUDE REGEX ".*(lua|luac)\\.c$")
add_library(lua_static STATIC ${LUA_SOURCES})

if(UNIX)
    set_target_properties(lua_static PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
endif()

include(FetchContent)
FetchContent_Declare(
    ixwebsocket
    GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
    GIT_TAG        v12.0.1 # Use the latest stable tag
)

FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# 2. Tell IXWebSocket to use native OS TLS (Schannel on Win / SecureTransport on Mac)
set(USE_TLS ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(ixwebsocket nlohmann_json)


file(GLOB_RECURSE SOURCES "src/*.cpp")

if(NOT SOURCES)
    set(SOURCES src/library.cpp)
endif()

if(WIN32)
    add_compile_definitions(NOMINMAX)
endif()



add_library(HPR-Store SHARED ${SOURCES})

# Link Slint
target_link_libraries(HPR-Store PRIVATE Slint::Slint)
target_link_libraries(HPR-Store PRIVATE ixwebsocket)

if(WIN32)
    target_link_libraries(HPR-Store PRIVATE crypt32 ws2_32)
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
)

target_include_directories(HPR-Store
    PRIVATE
    include/
)

# On Windows, copy the Slint DLL next to the application binary so that it's found
if (WIN32)
    add_custom_command(TARGET HPR-Store POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:HPR-Store> $<TARGET_FILE_DIR:HPR-Store> COMMAND_EXPAND_LISTS)
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
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${HPR_EXTENSION_DIR}"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:HPR-Store>"
        "${HPR_EXTENSION_DIR}/"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE_DIR:HPR-Store>/HPR-Store.lua"
        "${HPR_EXTENSION_DIR}/"

    COMMAND_EXPAND_LISTS
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
