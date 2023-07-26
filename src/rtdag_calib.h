#ifndef RTDAG_CALIB_H
#define RTDAG_CALIB_H

#include "newstuff/integers.h"
#include "time_aux.h"

int get_ticks_per_us(bool required);

int waste_calibrate();

int test_calibration(microseconds duration, u64 &time_difference);

int test_calibration(microseconds duration);

int calibrate(microseconds duration);

#endif // RTDAG_CALIB_H
