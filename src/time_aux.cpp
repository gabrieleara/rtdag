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

// ----------------------- Public function definitions ---------------------- //

uint64_t Count_Time(uint64_t duration_usec) {
    uint64_t counted_sheeps = 0;
    uint64_t elapsed_usecs = 0;
    struct timespec ts1, ts2;

    // NOTICE that we care only of the time spent while running THIS task, not
    // the wall time.
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts1);
    do {
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts2);
        elapsed_usecs = to_duration_truncate<microseconds>(ts2 - ts1).count();
        counted_sheeps++;
    } while (elapsed_usecs < duration_usec);

    return counted_sheeps;
}

// This variable must be set by the user before calling Count_Time_Ticks().
float ticks_per_us = 0;

uint64_t Count_Time_Ticks(microseconds duration, float ticks_per_us) {
    uint64_t ticks = ticks_per_us * duration.count();
    return Count_Ticks(ticks);
}

uint64_t Count_Ticks(uint64_t sheeps) {
    uint64_t temp = 0;
    for (uint64_t counted = 0; counted < sheeps; ++counted) {
        temp += rtgauss_waste_time(temp);
    }
    return temp;
}
