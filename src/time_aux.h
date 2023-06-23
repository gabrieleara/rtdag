#ifndef TIME_AUX_H_
#define TIME_AUX_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// clang-format off

/// Convert seconds to milliseconds
#define SEC_TO_MSEC(sec)    ((sec)*1000)
/// Convert seconds to microseconds
#define SEC_TO_USEC(sec)    ((sec)*1000000)
/// Convert seconds to nanoseconds
#define SEC_TO_NSEC(sec)    ((sec)*1000000000)

/// Convert milliseconds to seconds
#define MS_TO_SEC(ms)       ((ms)/1000)
/// Convert milliseconds to nanoseconds
#define MS_TO_NSEC(ms)      ((ms)*1000)
/// Convert milliseconds to microseconds
#define MS_TO_USEC(ms)      ((ms)*1000000)

/// Convert microseconds to seconds
#define US_TO_SEC(us)       ((us)/1000000)
/// Convert microseconds to miliseconds
#define US_TO_MSEC(us)      ((us)/1000)
/// Convert microseconds to nanoseconds
#define US_TO_NSEC(us)      ((us)*1000)

/// Convert nanoseconds to seconds
#define NS_TO_SEC(ns)       ((ns)/1000000000)
/// Convert nanoseconds to milliseconds
#define NS_TO_MSEC(ns)      ((ns)/1000000)
/// Convert nanoseconds to microseconds
#define NS_TO_USEC(ns)      ((ns)/1000)

// clang-format on

// Returns the number of microseconds wrt to wall time (CLOCK_MONOTONIC)
extern uint64_t micros(void);

#if defined(__GNUC__) &&                                                       \
    (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7))
#define ATTRIBUTE_DISABLE_OPTIMIZATIONS __attribute__((optimize("0")))
#elif defined(__clang__) &&                                                    \
    (__clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 3))
#define ATTRIBUTE_DISABLE_OPTIMIZATIONS __attribute__((optnone))
#else
#error "Unsupported compiler. Expecting gcc 7.0 or newer, or clang 10 or newer"
#endif

// Execute a fixed amount of work, depending on the number of ticks supplied.
// The work done depends on the implementation of waste_time(), defined in the
// source of time_aux.c.
extern uint64_t Count_Ticks(uint64_t ticks) ATTRIBUTE_DISABLE_OPTIMIZATIONS;

// Execute for an amount of ticks derived from the time span indicated by
// usec and ticks_per_us (use `ticks_per_us` global variable if you don't
// want special time accounting)
extern uint64_t Count_Time_Ticks(uint64_t usec, float ticks_per_us)
    ATTRIBUTE_DISABLE_OPTIMIZATIONS;

// Actively wait for the specified amount of microseconds, by repeatedly
// checking whether the time has elapsed.
// Returns the number of calls to clock_gettime.
extern uint64_t Count_Time(uint64_t usec);

// Global variable used to calculate the number of ticks in Count_Time_Ticks().
extern float ticks_per_us;

#ifdef __cplusplus
}
#endif

#endif // TIME_AUX_H_
