cmake_minimum_required(VERSION 3.15)
project(rt_dag LANGUAGES C CXX)

# no need to require this. it only makes it hardware to use older compilers
# set(CMAKE_C_STANDARD 17)
# set(CMAKE_C_STANDARD_REQUIRED TRUE)

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# TODO: add toggles from command line
add_definitions(-DUSE_COMPILER_BARRIER)

##############
# basic definitions
##############
# compilation parameters
add_compile_options(
  # -Werror
  -Wall
  -Wextra
  -Wpedantic
  -Wno-variadic-macros
  -Wno-long-long
  #-Wno-extra-semi only avaible in newer compilers
)

# set if the exec must be configured for debug or release/optimized
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()
message(STATUS "Build type: '${CMAKE_BUILD_TYPE}'")

##############
# logger vebosity level
##############

set(LOG_LEVEL "1" CACHE STRING "Logger vebosity level")
set(LOGValues "0;1;2;3;4" CACHE STRING
  "Possible verbosity values. 0 is the lowest verbosity")
set_property(CACHE LOG_LEVEL PROPERTY STRINGS ${LOGValues})
message(STATUS "Verbosity Level: '${LOG_LEVEL}'")
add_definitions(-DLOG_LEVEL=${LOG_LEVEL})

##############
# task implementation
##############

set(TASK_IMPL "thread" CACHE STRING
  "Task implementation method")
set(TaskImplValues "thread;process" CACHE STRING
  "thread-based (default) or 1 for process-based task implementation.")
set_property(CACHE TASK_IMPL PROPERTY STRINGS ${TaskImplValues})
message(STATUS "Task implementation: '${TASK_IMPL}-based'")
IF (${TASK_IMPL} STREQUAL "thread")
  set(TASK_IMPL_INT 0)
ELSE()
  set(TASK_IMPL_INT 1)
ENDIF()
add_definitions(-DTASK_IMPL=${TASK_IMPL_INT})

##############
# input implementation: C header or YAML input
##############

set(INPUT_TYPE "yaml" CACHE STRING
  "DAG input method")
set(InputImplValues "yaml;header" CACHE STRING
  "yaml-based (default) or C header input.")
set_property(CACHE INPUT_TYPE PROPERTY STRINGS ${InputImplValues})
message(STATUS "Input implementation: '${INPUT_TYPE}-based'")
IF (${INPUT_TYPE} STREQUAL "header")
  set(INPUT_TYPE_INT 0)
ELSE()
  set(INPUT_TYPE_INT 1)
ENDIF()
add_definitions(-DINPUT_TYPE=${INPUT_TYPE_INT})

##############
# check dependencies
##############

# YAML parser dependency
find_package(YAML-CPP REQUIRED
    HINTS /usr/local/share/cmake)

##############
# Optional features
##############

SET(ENABLE_MEM_ACCESS OFF CACHE BOOL "Enable memory rd/wr for every message sent. If disabled, it means that the execution time does not account for memory access time.")
IF (ENABLE_MEM_ACCESS)
    add_definitions(-DENABLE_MEM_ACCESS)
ENDIF(ENABLE_MEM_ACCESS)
message(STATUS "Memory access enabled: ${ENABLE_MEM_ACCESS}")

SET(ENABLE_COUNT_TICK ON CACHE BOOL "Enable tick-based method to emulate computation. If disabled, it means that the execution time is emulated using 'clock_gettime'.")
IF (ENABLE_COUNT_TICK)
    add_definitions(-DUSE_COUNT_TICK)
ENDIF(ENABLE_COUNT_TICK)
message(STATUS "Count tick enabled: ${ENABLE_COUNT_TICK}")

#SET(ENABLE_PIPELINING OFF CACHE BOOL "Enable pipelining in the DAG, e.g., a task is processing a frame while the previous task is processing the previous frame and so on.")
#SET(ENABLE_ZERO_COPY OFF CACHE BOOL "Sender and receiver access directly the shared memory")
#SET(ENABLE_DAG_DEADLINE_CHECK OFF CACHE BOOL "Enable checking the DAG deadline")
#
#IF (ENABLE_PIPELINING)
#    add_definitions(-DENABLE_PIPELINING)
#ENDIF(ENABLE_PIPELINING)
#IF (ENABLE_ZERO_COPY)
#    add_definitions(-DENABLE_ZERO_COPY)
#ENDIF(ENABLE_ZERO_COPY)
#IF (ENABLE_DAG_DEADLINE_CHECK)
#    add_definitions(-DENABLE_DAG_DEADLINE_CHECK)
#ENDIF(ENABLE_DAG_DEADLINE_CHECK)

##############
# when running in a Xilinx Ultrascale board, check whether FRED lig is installed
##############
# change here is libfred is not in the default lib path
set(FRED_PATH "/usr" CACHE STRING "PATH to fred lib")
find_library(LIBFRED
    NAMES fred libfred fred-lib fredlib
    HINTS
        ${FRED_PATH}/lib
)

if(LIBFRED)
  message(STATUS "fred_lib found")
  add_definitions(-DUSE_FRED)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -O2 -Wall -Werror -fpic")
  # in case FRED is not installed in the default location /usr/lib
  include_directories(${FRED_PATH}/include/ ${FRED_PATH})
  link_directories(${FRED_PATH}/lib/)
endif()

##############
# build internal depedencies as a library
##############

add_library(time-helper STATIC
  ${CMAKE_SOURCE_DIR}/../../../common/src/periodic_task.c
  ${CMAKE_SOURCE_DIR}/../../../common/src/time_aux.c
)

target_include_directories(time-helper
  PUBLIC ${CMAKE_SOURCE_DIR}/../../../common/include/
)

# set_source_files_properties(
#   ${CMAKE_SOURCE_DIR}/../../../common/src/periodic_task.c PROPERTIES LANGUAGE C)
# set_source_files_properties(
#   ${CMAKE_SOURCE_DIR}/../../../common/src/time_aux.c PROPERTIES LANGUAGE C)

# include_directories(
#     ${CMAKE_SOURCE_DIR}/../../../common/include/
#     ${CMAKE_SOURCE_DIR}/include/
#     ${CMAKE_SOURCE_DIR}/
# )

# set( LIB_SOURCES
# ${CMAKE_SOURCE_DIR}/../../../common/src/periodic_task.c
# ${CMAKE_SOURCE_DIR}/../../../common/src/time_aux.c
# )
# set( LIB_HEADERS
# ${CMAKE_SOURCE_DIR}/../../../common/include/periodic_task.h
# ${CMAKE_SOURCE_DIR}/../../../common/include/time_aux.h
# )
# add_library( YourLib ${LIB_SOURCES} ${LIB_HEADERS} )

# # for some reason, it is required to add this line in order to link the c code into cpp exec
# set_source_files_properties(${CMAKE_SOURCE_DIR}/../../../common/src/periodic_task.c PROPERTIES LANGUAGE CXX)
# set_source_files_properties(${CMAKE_SOURCE_DIR}/../../../common/src/time_aux.c PROPERTIES LANGUAGE CXX)

##############
# determining the source files to be compiled based on the configurable parameters
##############

add_executable(rt_dag main.cpp)
target_include_directories(rt_dag
  PRIVATE
  ${CMAKE_SOURCE_DIR}/include/
  ${CMAKE_SOURCE_DIR}/
)

target_link_libraries(rt_dag
  time-helper
  m
  rt
  pthread
  yaml-cpp
)

if(LIBFRED)
  target_link_libraries(rt_dag fred)
endif()
