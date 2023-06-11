#!/bin/bash

clang-15 -O3 hellogauss.c -o hellogauss -fopenmp -fopenmp-targets=nvptx64-nvidia-cuda,x86_64

OMP_TARGET_OFFLOAD=MANDATORY ./hellogauss
