# NOTE: you need to source the ELINOS.sh file before running cmake!

if (DEFINED ENV{ELINOS_PREFIX})
    # Ok cool
    message(STATUS "!!!!! USING ELINOS CONFIGURATION TO CROSS-COMPILE !!!!!")
else()
    message(FATAL_ERROR "No ELINOS configuration found, please source ELINOS.sh file before running.")
endif()

# the name of the target operating system
set(CMAKE_SYSTEM_NAME ElinOS)

# C/CXX Compilers
set(CMAKE_C_COMPILER $ENV{CC})
set(CMAKE_CXX_COMPILER $ENV{CXX})

# Where the target environment is installed
set(CMAKE_FIND_ROOT_PATH $ENV{ELINOS_CDK})

# Adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search headers and libraries in the target environment
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
