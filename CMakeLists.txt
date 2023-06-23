cmake_minimum_required(VERSION 3.16)
project(rtdag LANGUAGES C CXX)

include(cmake/util.cmake)
include(cmake/CPM.cmake)

# ==================== GLOBAL OPTIONS ==================== #

# Choice-based features
add_option_choice_force(CMAKE_BUILD_TYPE "Release" "Debug;Release;MinSizeRel;RelWithDebInfo" "Select type of build")
add_option_numbered_choice(RTDAG_LOG_LEVEL "none" "none;error;warning;info;debug" "Logger verbosity level")
add_option_numbered_choice(RTDAG_TASK_IMPL "thread" "thread;process" "How the task is implemented (either a thread or a process)")
add_option_numbered_choice(RTDAG_INPUT_TYPE "yaml" "yaml;header" "How rtdag task configuration is provided")

# Booolean features
add_option_bool(RTDAG_COMPILER_BARRIER ON "Injects compiler barriers into code to prevent instruction reordering")
add_option_bool(RTDAG_MEM_ACCESS OFF "Enable memory rd/wr for every message sent.")
add_option_bool(RTDAG_COUNT_TICK ON "Enable tick-based emulation of computation. When OFF, uses 'clock_gettime' instead.")
add_option_bool(RTDAG_OMP_SUPPORT OFF "Enable OpenMP support for task acceleration.")

# Missing Optional Features (I think)

# #SET(ENABLE_PIPELINING OFF CACHE BOOL "Enable pipelining in the DAG, e.g., a task is processing a frame while the previous task is processing the previous frame and so on.")
# #SET(ENABLE_ZERO_COPY OFF CACHE BOOL "Sender and receiver access directly the shared memory")
# #SET(ENABLE_DAG_DEADLINE_CHECK OFF CACHE BOOL "Enable checking the DAG deadline")
# #
# #IF (ENABLE_PIPELINING)
# #    add_definitions(-DENABLE_PIPELINING)
# #ENDIF(ENABLE_PIPELINING)
# #IF (ENABLE_ZERO_COPY)
# #    add_definitions(-DENABLE_ZERO_COPY)
# #ENDIF(ENABLE_ZERO_COPY)
# #IF (ENABLE_DAG_DEADLINE_CHECK)
# #    add_definitions(-DENABLE_DAG_DEADLINE_CHECK)
# #ENDIF(ENABLE_DAG_DEADLINE_CHECK)

# ===================== DEPENDENCIES ===================== #

# YAML CPP (see below)
# find_package(YAML-CPP REQUIRED HINTS /usr/local/share/cmake)

# OpenCL
# add_option_bool(RTDAG_OPENCL_REQUIRED OFF "Makes OpenCL a REQUIRED library.")
# find_library(OpenCL NAMES libOpenCL libOpenCL.so.1 HINTS /usr/lib /usr/lib64)
# check_optional_library(OpenCL)

# FRED
add_option_bool(RTDAG_FRED_SUPPORT OFF "Enables Fred support in RTDAG. Makes Fred a required dependency.")
if (RTDAG_FRED_SUPPORT)
    CPMAddPackage(
        NAME libfred
        GIT_REPOSITORY https://github.com/fred-framework/fred-linux-client-lib.git
        GIT_TAG master # Do not care for version numbers for now
    )

    if (NOT libfred_ADDED STREQUAL "YES")
        message(FATAL_ERROR "Required dependency libfred not found!")
    endif()
endif()

# add_option_string(RTDAG_FRED_PATH "/usr" "Lookup path for FRED LIBRARY")
# add_option_bool(RTDAG_FRED_REQUIRED OFF "Makes FRED a REQUIRED library.")
# find_library(Fred NAMES fred libfred fred-lib fredlib HINTS ${RTDAG_FRED_PATH}/lib)
# check_optional_library(Fred)

# FIXME: includes for a library (shouldn't they be found automatically?)
# include_directories(${FRED_PATH}/include/ ${FRED_PATH})

# =================== CPM Dependencies =================== #

# Libfmt version 10.0.0
# CPMAddPackage("gh:fmtlib/fmt#10.0.0")

# Lib YAML CPP version 0.7.0
CPMAddPackage("gh:jbeder/yaml-cpp#yaml-cpp-0.7.0@0.7.0")

# ============== SAVE CONFIGURATION TO FILE ============== #

# Configuration file, will be place in $CMAKE_CURRENT_BINARY_DIR, visible
# to all targets via forced include (see src/CMakeLists.txt)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/rtdag_config.h.in rtdag_config.h @ONLY)

# ================= PRINT CONFIGURATION ================== #

# Print all configuration variables
message(STATUS "---------- CONFIGURATION OPTIONS ---------- ")
message(STATUS "CMAKE_BUILD_TYPE            ${CMAKE_BUILD_TYPE}")
message(STATUS "RTDAG_LOG_LEVEL             ${RTDAG_LOG_LEVEL} (${RTDAG_LOG_LEVEL_VALUE})")
message(STATUS "RTDAG_TASK_IMPL             ${RTDAG_TASK_IMPL} (${RTDAG_TASK_IMPL_VALUE})")
message(STATUS "RTDAG_INPUT_TYPE            ${RTDAG_INPUT_TYPE} (${RTDAG_INPUT_TYPE_VALUE})")
message(STATUS "RTDAG_COMPILER_BARRIER      ${RTDAG_COMPILER_BARRIER}")
message(STATUS "RTDAG_MEM_ACCESS            ${RTDAG_MEM_ACCESS}")
message(STATUS "RTDAG_COUNT_TICK            ${RTDAG_COUNT_TICK}")
message(STATUS "RTDAG_OMP_SUPPORT           ${RTDAG_OMP_SUPPORT}")
message(STATUS "RTDAG_FRED_SUPPORT          ${RTDAG_FRED_SUPPORT}")

# message_library(OpenCL)
# message_library(Fred)

message(STATUS "------------------------------------------- ")

# ======================= TARGETS ======================== #

add_executable(rtdag
    src/rtdag_main.cpp
    src/periodic_task.c
    src/time_aux.c
    src/rtgauss.cpp
)

set_target_properties(rtdag PROPERTIES
    LANGUAGE CXX
)

target_compile_features(rtdag PUBLIC cxx_std_20)
target_compile_options(rtdag PRIVATE
    -Werror
    -Wall
    -Wextra
    -Wpedantic
    -Wno-long-long
    -Wno-variadic-macros

    -include ${CMAKE_CURRENT_BINARY_DIR}/rtdag_config.h
)

# FIXME: some way to setup which openmp targets to compile
# at configuration time

if(RTDAG_OMP_SUPPORT)
    message(STATUS "Testing for OMP support...")
    find_package(OpenMP)

    if (OpenMP_FOUND)
        message(STATUS "OpenMP found: ${OpenMP_FOUND}")
    else()
        message(FATAL_ERROR "OpenMP support not found!!!")
    endif()

    target_compile_options(rtdag PRIVATE
        -fopenmp
        -fopenmp-targets=nvptx64-nvidia-cuda
        "${OpenMP_CXX_FLAGS}"
    )

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fopenmp -fopenmp-targets=nvptx64-nvidia-cuda ${OpenMP_CXX_FLAGS}")
endif()

# target_compile_definitions(rtdag PRIVATE)
target_include_directories(rtdag
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(rtdag
    m
    rt
    pthread
    yaml-cpp
)

if (RTDAG_FRED_SUPPORT)
    target_link_libraries(rtdag FredFramework::libfred_static)
endif()

# target_optional_link_library(rtdag Fred)
# target_optional_link_library(rtdag OpenCL)

# Required to make clang see the libc++?
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")


# ================== INSTALL PROPERTIES ================== #

include(GNUInstallDirs)

# Export targets to install
install(TARGETS rtdag
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
