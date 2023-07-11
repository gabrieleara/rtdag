#ifndef RTDAG_CALIB_H
#define RTDAG_CALIB_H

#include "newstuff/integers.h"

int get_ticks_per_us(bool required);

int waste_calibrate();

int test_calibration(u64 duration_us, u64 &time_difference);

int test_calibration(u64 duration_us);

int calibrate(u64 duration_us);

#endif // RTDAG_CALIB_H
