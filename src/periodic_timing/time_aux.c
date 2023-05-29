#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <sched.h>
#include <time.h>

#include "hellogauss.h"
#include "time_aux.h"

// ----------------------- Local function declarations ---------------------- //

// Wastes some time performing a constant amount of operations.
static uint64_t waste_time(uint64_t in);

// Calculates the difference between two timespecs in useconds.
static inline uint64_t timespec_sub_us(const struct timespec *ts1,
                                       const struct timespec *ts2);

// ----------------------- Public function definitions ---------------------- //

uint64_t micros() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t us =
        SEC_TO_USEC((uint64_t)ts.tv_sec) + NS_TO_USEC((uint64_t)ts.tv_nsec);
    return us;
}

uint64_t Count_Time(uint64_t duration_usec) {
    uint64_t counted_sheeps = 0;
    uint64_t elapsed_usecs = 0;
    struct timespec ts1, ts2;

    // NOTICE that we care only of the time spent while running THIS task, not
    // the wall time.
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts1);
    do {
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts2);
        elapsed_usecs = timespec_sub_us(&ts2, &ts1);
        counted_sheeps++;
    } while (elapsed_usecs < duration_usec);

    return counted_sheeps;
}

// This variable must be set by the user before calling Count_Time_Ticks().
float ticks_per_us = 0;
uint64_t Count_Time_Ticks(uint64_t usec) {
    uint64_t ticks = ticks_per_us * usec;
    return Count_Ticks(ticks);
}

uint64_t Count_Ticks(uint64_t sheeps) {
    uint64_t temp = 0;
    for (uint64_t counted = 0; counted < sheeps; ++counted) {
        temp = waste_time(temp);
    }
    return temp;
}

// ------------------------ Local function definitions ---------------------- //

uint64_t timespec_sub_us(const struct timespec *ts1,
                         const struct timespec *ts2) {
    // NOTE: assumes that ts2 >= ts1!
    return (ts1->tv_sec - ts2->tv_sec) * 1000000 +
           (ts1->tv_nsec - ts2->tv_nsec) / 1000;
}

// NOTE: the following global variables are NOT static by intention, so that
// they will NOT be optimized at compile time. The compiler may notice that they
// are never checked by somebody else if they were static!

typedef double matrix_type[GAUSS_MSIZE];

// clang-format off

// Keep the data together and align with cache lines to reduce the number of
// cache lines used to the minimum.
//
// In total, the data comprises of 32*3 words, or about 384 bytes in memory.
// This totals to (exactly) 6 lines of cache on platforms with 64 bytes per
// cache line.
//
// Due to compiler optimizations, matrix_C may NOT be allocated in memory at
// all, reducing the number of cache lines used even further.
//
// See https://godbolt.org/z/hKr9GYs4x for further reference.
struct waste_time_type {
    matrix_type matrix_A;
    matrix_type matrix_B;
};

static __thread matrix_type matrix_C;

struct waste_time_type waste_time_data __attribute__((__aligned__(64))) = {
    .matrix_A = {
        1, 2, 0, 7,
        2, 1, 0, 3,
        0, 0, 1, 0,
        0, 7, 2, 1,
    },
    .matrix_B = {
        -0.270270,  0.635135,  0.027027, -0.013514,
        -0.027027,  0.013514, -0.297297,  0.148649,
         0.000000,  0.000000,  1.000000,  0.000000,
         0.189189, -0.094595,  0.081081, -0.040541,
    },
};

// clang-format on

uint64_t waste_time(uint64_t in) {
    // unsigned int cpu = 0;
    // int res = getcpu(&cpu, NULL);
    // if (res != 0) {
    //     fprintf(stderr, "getcpu: %s\n", strerror(errno));
    //     exit(1);
    // }

    // This function (even when optimized and all the loops are unfolded by
    // the compiler) performs several operations on the content of the
    // matrices, including multiplications, additions, etc.
    gauss_multiply(waste_time_data.matrix_A, waste_time_data.matrix_B,
                   matrix_C);
    bool result = gauss_is_identity(matrix_C);
    return in + ((result) ? 2 : 1);
}
