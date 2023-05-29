cmake_minimum_required(VERSION 3.15)
project(rtdag LANGUAGES C CXX)

include(cmake/util.cmake)
include(cmake/CPM.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# ==================== GLOBAL OPTIONS ==================== #

# Choice-based features
add_option_choice_force(CMAKE_BUILD_TYPE "Release" "Debug;Release;MinSizeRel;RelWithDebInfo" "Select type of build")
add_option_choice(CONFIG_LOG_LEVEL 0 "0;1;2;3;4" "Logger verbosity level (0 = no logging, 4 = DEBUG)")
add_option_numbered_choice(CONFIG_TASK_IMPL "thread" "thread;process" "How the task is implemented (either a thread or a process)")
add_option_numbered_choice(CONFIG_INPUT_TYPE "yaml" "yaml;header" "How rtdag task configuration is provided")

# Booolean features
add_option_bool(CONFIG_COMPILER_BARRIER ON "Injects compiler barriers into code to prevent instruction reordering")
add_option_bool(CONFIG_MEM_ACCESS OFF "Enable memory rd/wr for every message sent.")
add_option_bool(CONFIG_COUNT_TICK ON "Enable tick-based emulation of computation. When OFF, uses 'clock_gettime' instead.")

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
add_option_bool(CONFIG_OPENCL_REQUIRED OFF "Makes OpenCL a REQUIRED library.")
find_library(OpenCL NAMES libOpenCL libOpenCL.so.1 HINTS /usr/lib /usr/lib64)
check_optional_library(OpenCL)

# FRED
add_option_string(CONFIG_FRED_PATH "/usr" "Lookup path for FRED LIBRARY")
add_option_bool(CONFIG_FRED_REQUIRED OFF "Makes FRED a REQUIRED library.")
find_library(Fred NAMES fred libfred fred-lib fredlib HINTS ${CONFIG_FRED_PATH}/lib)
check_optional_library(Fred)

# FIXME: includes for a library (shouldn't they be found automatically?)
# include_directories(${FRED_PATH}/include/ ${FRED_PATH})

# =================== CPM Dependencies =================== #

# Libfmt version 10.0.0
CPMAddPackage("gh:fmtlib/fmt#10.0.0")

# Lib YAML CPP version 0.7.0
CPMAddPackage("gh:jbeder/yaml-cpp#yaml-cpp-0.7.0@0.7.0")

# ============== SAVE CONFIGURATION TO FILE ============== #

# Configuration file, will be place in $CMAKE_CURRENT_BINARY_DIR, visible
# to all targets via forced include
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/rtdag_config.h.in rtdag_config.h @ONLY)
add_definitions(-include ${CMAKE_CURRENT_BINARY_DIR}/rtdag_config.h)

# ================= PRINT CONFIGURATION ================== #

# Print all configuration variables
message(STATUS "---------- CONFIGURATION OPTIONS ---------- ")
message(STATUS "CMAKE_BUILD_TYPE            ${CMAKE_BUILD_TYPE}")
message(STATUS "CONFIG_LOG_LEVEL            ${CONFIG_LOG_LEVEL}")
message(STATUS "CONFIG_TASK_IMPL            ${CONFIG_TASK_IMPL} (${CONFIG_TASK_IMPL_VALUE})")
message(STATUS "CONFIG_INPUT_TYPE           ${CONFIG_INPUT_TYPE} (${CONFIG_INPUT_TYPE_VALUE})")
message(STATUS "CONFIG_COMPILER_BARRIER     ${CONFIG_COMPILER_BARRIER}")
message(STATUS "CONFIG_MEM_ACCESS           ${CONFIG_MEM_ACCESS}")
message(STATUS "CONFIG_COUNT_TICK           ${CONFIG_COUNT_TICK}")
message(STATUS "CONFIG_OPENCL_USE           ${CONFIG_OPENCL_USE}")
message(STATUS "CONFIG_FRED_USE             ${CONFIG_FRED_USE}")
message(STATUS "CONFIG_OPENCL_REQUIRED      ${CONFIG_OPENCL_REQUIRED}")
message(STATUS "CONFIG_FRED_REQUIRED        ${CONFIG_FRED_REQUIRED}")

message_library(OpenCL)
message_library(Fred)

message(STATUS "------------------------------------------- ")

# =================== COMPILER OPTIONS =================== #

#
add_compile_options(
  -Werror
  -Wall
  -Wextra
  -Wpedantic
  -Wno-variadic-functions
  -Wno-long-long
  -Wno-variadic-macros

  -Wfatal-errors
  # -Wno-extra-semi # only avaible in newer compilers
)

# ======================== TARGETS ======================= #

# See CMakeLists.txt in src
add_subdirectory(src)
