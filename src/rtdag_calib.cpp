#include "rtdag_calib.h"
#include "time_aux.h"

#include <iostream>
#include <sstream>
#include <string>

#if RTDAG_USE_COMPILER_BARRIER == ON
// It tells the compiler to not reorder instructions around it
#define COMPILER_BARRIER() asm volatile("" ::: "memory")
#else
#define COMPILER_BARRIER()                                                     \
    do {                                                                       \
    } while (0)
#endif

int get_ticks_per_us(bool required) {
    if (ticks_per_us > 0) {
        return EXIT_SUCCESS;
    }

    char *TICKS_PER_US = getenv("TICKS_PER_US");

    auto &print_stream = (required) ? std::cerr : std::cout;
    auto kind = (required) ? "ERROR" : "WARN";

    if (TICKS_PER_US == nullptr) {
        print_stream << kind << ": TICKS_PER_US undefined!" << std::endl;
        return EXIT_FAILURE;
    } else {
        auto mstring = std::string(TICKS_PER_US);
        auto mstream = std::istringstream(mstring);

        mstream >> ticks_per_us;
        if (!mstream) {
            std::cerr << "Error! could not parse environment variable"
                      << std::endl;
        }
    }

    std::cout << "Using the following value for time accounting: "
              << ticks_per_us << std::endl;

    return EXIT_SUCCESS;
}

int waste_calibrate() {
    COMPILER_BARRIER();

    uint64_t retv = Count_Time_Ticks(microseconds(1), 1);

    COMPILER_BARRIER();

    return retv;
}

int test_calibration(microseconds duration, struct timespec &time_difference) {
    int res = get_ticks_per_us(true);
    if (res)
        return res;

    COMPILER_BARRIER();

    auto time_before = curtime();

    COMPILER_BARRIER();

    uint64_t retv = Count_Time_Ticks(duration, ticks_per_us);
    (void)(retv);

    COMPILER_BARRIER();

    auto time_after = curtime();

    COMPILER_BARRIER();

    time_difference = time_after - time_before;
    printf("Test duration %ld micros\n",
           to_duration_truncate<microseconds>(time_difference).count());

    return 0;
}

int test_calibration(microseconds duration) {
    struct timespec time_difference_unused;
    return test_calibration(duration, time_difference_unused);
}

int calibrate(microseconds duration) {
    int ret;
    struct timespec time_difference = {
        .tv_sec = 1,
        .tv_nsec = nanoseconds(microseconds(1)).count(),
    };

    ret = get_ticks_per_us(false);
    if (ret) {
        // Set a value that is not so small in ticks_per_us
        ticks_per_us = 10;
    }

    std::cout << "About to calibrate for (roughly) " << duration
              << " micros ..." << std::endl;

    // Will never return an error
    test_calibration(duration, time_difference);
    // fprintf(stderr, "DEBUG: %llu %llu %llu %llu\n", duration_us,
    // time_difference, ticks_per_us, duration_us * ticks_per_us);

    using ticks_type = decltype(ticks_per_us);

    double duration_d = duration.count();
    double time_difference_d = std::chrono::duration<double, std::micro>(
                                   to_nanoseconds(time_difference))
                                   .count();

    ticks_per_us = ticks_type((duration_d * ticks_per_us) / time_difference_d);

    std::cout << "Calibration successful, use: 'export TICKS_PER_US="
              << ticks_per_us << "'" << std::endl;

    return EXIT_SUCCESS;
}
