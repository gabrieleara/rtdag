#ifndef RTGAUSS_H
#define RTGAUSS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum rtgauss_type {
    RTGAUSS_CPU = 0,
#if RTDAG_OMP_SUPPORT == ON
    RTGAUSS_OMP = 2,
#endif
};

// Must be called by each cpu and omp thread!
extern void rtgauss_init(int size, enum rtgauss_type type, int omp_target_dev);

extern uint64_t rtgauss_waste_time(uint64_t in);

#ifdef __cplusplus
}
#endif

#endif // RTGAUSS_H
