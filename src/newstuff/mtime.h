#ifndef RTDAG_MTIME_H
#define RTDAG_MTIME_H

#include <chrono>
#include <time.h>

using std::chrono::microseconds;
using std::chrono::milliseconds;
using std::chrono::nanoseconds;
using std::chrono::seconds;

static inline struct timespec curtime() {
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return time;
}

static inline struct timespec operator-(const struct timespec &t1,
                                        const struct timespec &t0) {
    struct timespec diff = {
        .tv_sec = t1.tv_sec - t0.tv_sec,
        .tv_nsec = t1.tv_nsec - t0.tv_nsec,
    };

    if (diff.tv_nsec < 0) {
        // Add one second to the nanosecond difference
        diff.tv_nsec +=
            std::chrono::nanoseconds(std::chrono::seconds(1)).count();
        diff.tv_sec--;
    }
    return diff;
}

template <class To>
static inline auto to_duration_truncate(const struct timespec &duration) {
    return std::chrono::duration_cast<To>(seconds(duration.tv_sec) +
                                          nanoseconds(duration.tv_nsec));
}

static inline auto to_nanoseconds(const struct timespec &duration) {
    return seconds(duration.tv_sec) + nanoseconds(duration.tv_nsec);
}

#endif // RTDAG_MTIME_H
