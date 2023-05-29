cmake_minimum_required(VERSION 3.0)
project(dag_test_examples)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
# make sure the code work with O3 optimization
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include_directories(
    ${CMAKE_SOURCE_DIR}/../include/
    ${CMAKE_SOURCE_DIR}/../
)

add_executable(thread_v2
               thread_v2.cpp)
target_link_libraries(thread_v2 rt pthread) 

add_executable(thread_v3
               thread_v3.cpp)
target_link_libraries(thread_v3 rt pthread) 

add_executable(mini
               mini.cpp)
target_link_libraries(mini rt pthread) 

add_executable(thread_circ
               thread_circ.cpp)
target_link_libraries(thread_circ rt pthread) 

add_executable(writer_circ
               writer_circ.cpp)
target_link_libraries(writer_circ rt pthread) 

add_executable(reader_circ
               reader_circ.cpp)
target_link_libraries(reader_circ rt pthread) 