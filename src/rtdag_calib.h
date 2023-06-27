#ifndef RTDAG_CALIB_H
#define RTDAG_CALIB_H

#include "time_aux.h"

#include <iostream>
#include <sstream>
#include <string>
using namespace std;

// ╔═══════════════════════════════════════════════════════════════════════════╗
// ║                          Calibration and Testing                          ║
// ╚═══════════════════════════════════════════════════════════════════════════╝

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

    auto &print_stream = (required) ? cerr : cout;
    auto kind = (required) ? "ERROR" : "WARN";

    if (TICKS_PER_US == nullptr) {
        print_stream << kind << ": TICKS_PER_US undefined!" << endl;
        return EXIT_FAILURE;
    } else {
        auto mstring = std::string(TICKS_PER_US);
        auto mstream = std::istringstream(mstring);

        mstream >> ticks_per_us;
        if (!mstream) {
            cerr << "Error! could not parse environment variable" << endl;
        }
    }

    cout << "Using the following value for time accounting: " << ticks_per_us
         << endl;

    return EXIT_SUCCESS;
}

int waste_calibrate() {
    COMPILER_BARRIER();

    uint64_t retv = Count_Time_Ticks(1, 1);

    COMPILER_BARRIER();

    return retv;
}

int test_calibration(uint64_t duration_us, uint64_t &time_difference) {
    int res = get_ticks_per_us(true);
    if (res)
        return res;

    COMPILER_BARRIER();

    auto time_before = micros();

    COMPILER_BARRIER();

    uint64_t retv = Count_Time_Ticks(duration_us, ticks_per_us);
    (void)(retv);

    COMPILER_BARRIER();

    auto time_after = micros();

    COMPILER_BARRIER();

    time_difference = time_after - time_before;
    cout << "Test duration: " << time_difference << " micros" << endl;

    return 0;
}

int test_calibration(uint64_t duration_us) {
    uint64_t time_difference_unused;
    return test_calibration(duration_us, time_difference_unused);
}

int calibrate(uint64_t duration_us) {
    int ret;
    uint64_t time_difference = 1;

    ret = get_ticks_per_us(false);
    if (ret) {
        // Set a value that is not so small in ticks_per_us
        ticks_per_us = 10;
    }

    cout << "About to calibrate for (roughly) " << duration_us << " micros ..."
         << endl;

    // Will never return an error
    test_calibration(duration_us, time_difference);
    // fprintf(stderr, "DEBUG: %llu %llu %llu %llu\n", duration_us,
    // time_difference, ticks_per_us, duration_us * ticks_per_us);

    using ticks_type = decltype(ticks_per_us);

    ticks_per_us = ticks_type(double(duration_us * ticks_per_us) /
                              double(time_difference));

    cout << "Calibration successful, use: 'export TICKS_PER_US=" << ticks_per_us
         << "'" << endl;

    return EXIT_SUCCESS;
}

#endif // RTDAG_CALIB_H
