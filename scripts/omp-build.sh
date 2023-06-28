#!/bin/bash

export CC=/usr/local/llvm-17.0.0-bsc/bin/clang-17
export CXX=/usr/local/llvm-17.0.0-bsc/bin/clang++

# if ! [ -f "./omp-build.sh" ]; then
#     echo "You must run this script from within its own directory!!!"
#     return 1
# fi

if [ "$1" = clean ] ; then
    rm -rf build/
fi

cmake -S . -B build -DRTDAG_OMP_SUPPORT=ON # -DRTDAG_LOG_LEVEL=info
cmake --build build -j
# ldd build/rtdag
