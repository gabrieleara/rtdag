#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sched.h>
#include <time.h>

#include "rtgauss.h"
#include "time_aux.h"

// ----------------------- Local function declarations ---------------------- //

// Calculates the difference between two timespecs in useconds.
static inline uint64_t timespec_sub_us(const struct timespec *ts1,
                                       const struct timespec *ts2);

// ----------------------- Public function definitions ---------------------- //

uint64_t micros(void) {
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

uint64_t Count_Time_Ticks(uint64_t usec, float ticks_per_us) {
    uint64_t ticks = ticks_per_us * usec;
    return Count_Ticks(ticks);
}

uint64_t Count_Ticks(uint64_t sheeps) {
    uint64_t temp = 0;
    for (uint64_t counted = 0; counted < sheeps; ++counted) {
        temp += rtgauss_waste_time(temp);
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
