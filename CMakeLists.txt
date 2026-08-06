cmake_minimum_required(VERSION 3.21)

project(HPR-Store)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BUILD_PARALLEL_LEVEL 8)

# Find Slint (only dependency required)
find_package(Slint REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")

if(NOT SOURCES)
    set(SOURCES src/main.cpp)
endif()

if(WIN32)
    add_compile_definitions(NOMINMAX)
    add_executable(HPR-Store WIN32 ${SOURCES})
    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}")
elseif(LINUX)
    add_executable(HPR-Store ${SOURCES})
endif()

# Link Slint
target_link_libraries(HPR-Store PRIVATE Slint::Slint)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/main.slint")
    slint_target_sources(HPR-Store src/ui/main.slint)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ui/main.slint")
    slint_target_sources(HPR-Store ui/main.slint)
endif()

set_target_properties(HPR-Store PROPERTIES 
    SLINT_EMBED_RESOURCES embed-files
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
