# Logging library (header only)
add_library(logging INTERFACE)
target_include_directories(logging INTERFACE logging)

# Timing and periodic library (header + sources)
add_library(periodic_timing STATIC
    periodic_timing/periodic_task.c
    periodic_timing/time_aux.c
)
target_include_directories(periodic_timing
    PUBLIC periodic_timing
)

# Dag Input library
add_library(daginput INTERFACE)
target_include_directories(daginput INTERFACE daginput)
target_link_libraries(daginput
    INTERFACE logging
)

# Dag Task library
add_library(dagtask INTERFACE)
target_include_directories(dagtask INTERFACE dagtask)
target_link_libraries(dagtask
    INTERFACE logging
    INTERFACE daginput
)

# RTDAG executable
add_executable(rtdag
    rtdag_main.cpp
)

target_include_directories(rtdag
    PRIVATE .
    # PRIVATE include
    PRIVATE tofix
)

target_link_libraries(rtdag
    m
    rt
    pthread
    yaml-cpp

    # Internal libraries
    logging
    daginput
    dagtask
    periodic_timing
)

target_optional_link_library(rtdag Fred)
target_optional_link_library(rtdag OpenCL)
