#include "hellogauss.h"

#include <iostream>
#include <omp.h>
#include <stdio.h>

#include <sys/time.h>
#include <time.h>

/**
 * Calculate the difference between two timespecs.
 * Returned value is a third timespec containing the difference.
 */
timespec operator-(const timespec &time1, const timespec &time0) {
    timespec diff = {.tv_sec = time1.tv_sec - time0.tv_sec,
                     .tv_nsec = time1.tv_nsec - time0.tv_nsec};
    if (diff.tv_nsec < 0) {
        diff.tv_nsec += 1'000'000'000; // nsec/sec
        diff.tv_sec--;
    }
    return diff;
}

int main() {
    using matrix = double[GAUSS_MSIZE];

    matrix A = {
        1, 2, 0, 7, //
        2, 1, 0, 3, //
        0, 0, 1, 0, //
        0, 7, 2, 1, //
    };

    matrix B = {
        -0.270270, 0.635135,  0.027027,  -0.013514, //
        -0.027027, 0.013514,  -0.297297, 0.148649,  //
        0.000000,  0.000000,  1.000000,  0.000000,  //
        0.189189,  -0.094595, 0.081081,  -0.040541, //
    };

    matrix C;

    bool is_identity = false;

    timespec before;
    timespec after;

    clock_gettime(CLOCK_MONOTONIC_RAW, &before);

    // Hope it doesn't get optimized away
    for (int i = 0; i < 10; ++i) {

#pragma omp target \
    map(to:     A[0:GAUSS_MSIZE]) \
    map(to:     B[0:GAUSS_MSIZE]) \
    map(from:   C[0:GAUSS_MSIZE]) \
    map(from:   is_identity)
    {
        gauss_multiply(A, B, C);
        is_identity = gauss_is_identity(C);
    }

    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &after);

    std::cout << "Identity? ";
    std::cout << (is_identity ? "yes!" : "no!");
    std::cout << '\n';

    timespec difference = after - before;
    std::printf("That took: %lld.%.9ld seconds\n", (long long)difference.tv_sec,
                difference.tv_nsec);
}
