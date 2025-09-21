cmake_minimum_required(VERSION 3.10)

project(kernelpatch_build LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Git REQUIRED)
file(GLOB_RECURSE PATCH_FILES "${CMAKE_CURRENT_SOURCE_DIR}/patches/*.patch")

option(BUILD_KPIMG "if build kpimg" OFF)

add_custom_target(reset_submodule_to_head
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ResetSubmoduleToHash.cmake
    COMMENT "Resetting Git submodule in src/ to its recorded commit hash"
    VERBATIM
)

# Reset all submodules
message(STATUS reset submodules...)
execute_process(
    COMMAND git submodule foreach "git reset --hard"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE RESULT
)
foreach(PATCH ${PATCH_FILES})
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/patches" 
                  "${CMAKE_CURRENT_SOURCE_DIR}/src" 
                  PATCH_DEST_DIR ${PATCH})
    get_filename_component(PATCH_DEST_DIR ${PATCH_DEST_DIR} DIRECTORY)

    #execute_process(
    #    COMMAND git submodule update --init --force ${PATCH_DEST_DIR}
    #    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    #    RESULT_VARIABLE RESULT
    #)
    execute_process(
        COMMAND patch -p1 -i ${PATCH} -d ${PATCH_DEST_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE RESULT
    )
    if(NOT RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to apply patch: ${PATCH}")
    endif()
endforeach()

# include zlib src
add_subdirectory(src/zlib)

if(WIN32)
    add_subdirectory(src/winsup)
endif()

if(USE_SYSTEM_ZLIB)
    find_package(ZLIB REQUIRED)
endif()

include(CMakeLists.kptools.txt)