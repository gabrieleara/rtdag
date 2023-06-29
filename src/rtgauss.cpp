#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sched.h>
#include <time.h>

#if RTDAG_OMP_SUPPORT == ON
#include <omp.h>
#endif

#include <vector>

#include "rtgauss.h"
#include "time_aux.h"

// Row size of the matrices to multiply.
//
// If you decide to multiply a submatrix, only the top-left corner will be
// affected.
// #define GAUSS_SIZE 512
// #define GAUSS_MSIZE (GAUSS_SIZE * GAUSS_SIZE)

//----------------------------------------------------------
// Helper functions and macros
//----------------------------------------------------------

#define gauss_ats(m, i, j, s) m[i * s + j]
#define gauss_at(m, i, j) gauss_ats(m, i, j, GAUSS_SIZE)

static inline double gauss_absd(double x) {
    if (x >= 0)
        return x;
    return -x;
}

static inline bool gauss_is_equal(double x, double y) {
    // I know, doesn't make sense for big or small numbers
    const double epsilon = 1e-5;
    return gauss_absd(x - y) <= epsilon;
}

//----------------------------------------------------------
// Multiplication
//----------------------------------------------------------

#define GAUSS_MUL_BODY(in1, in2, out, size)                                    \
    for (int i = 0; i < size; i++) {                                           \
        for (int j = 0; j < size; j++) {                                       \
            double acc = 0;                                                    \
            for (int k = 0; k < size; k++)                                     \
                acc +=                                                         \
                    gauss_ats(in1, i, k, size) * gauss_ats(in2, k, j, size);   \
            gauss_ats(out, i, j, size) = acc;                                  \
        }                                                                      \
    }

static void gauss_mul(const double *in1, const double *in2, double *out,
                      const int size) {
    GAUSS_MUL_BODY(in1, in2, out, size)
}

#if RTDAG_OMP_SUPPORT == ON
static void gauss_mul_omp_target(const double *in1, const double *in2,
                                 double *out, const int size, int dev) {
// This macro will parallelize the first two loop levels, forcing OMP to
// take action on the specified device in a distributed and parallel way
#pragma omp target teams distribute parallel for device(dev) collapse(2)       \
    map(to : in1[0 : size], in2[0 : size]) map(from : out[0 : size])

    GAUSS_MUL_BODY(in1, in2, out, size)
}
#endif

//----------------------------------------------------------
// Identity Check
//----------------------------------------------------------

#define GAUSS_IS_IDENTITY_BODY(in, size, valid)                                \
    for (int i = 0; i < size; ++i) {                                           \
        for (int j = 0; j < size; ++j) {                                       \
            bool test = (i == j) ? 1.0 : 0.0;                                  \
            bool temp = gauss_is_equal(gauss_ats(in, i, j, size), test);       \
            valid = valid && temp;                                             \
        }                                                                      \
    }

static bool gauss_is_eye(const double *in, const int size) {
    bool valid = true;

    GAUSS_IS_IDENTITY_BODY(in, size, valid)

    return valid;
}

#if RTDAG_OMP_SUPPORT == ON
static bool gauss_is_eye_omp_target(const double *in, const int size, int dev) {
    bool valid = true;

#pragma omp target teams distribute parallel for device(dev) collapse(2)       \
    reduction(&& : valid) map(to : in[0 : size])
    GAUSS_IS_IDENTITY_BODY(in, size, valid)

    return valid;
}
#endif

//----------------------------------------------------------
// Fill matrix
//----------------------------------------------------------

static void gauss_fill_eye_matrix(double *out, const int size) {
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            gauss_ats(out, i, j, size) = (i == j ? 1 : 0);
        }
    }
}

//----------------------------------------------------------
// RTGAUSS WASTE TIME
//----------------------------------------------------------

// Pack thread-allocated data together
struct task_matrix_data {
    const int size;
    const enum rtgauss_type type;
    std::vector<double> A;
    std::vector<double> B;
    std::vector<double> C;

    explicit task_matrix_data(const int size, const rtgauss_type type) :
        size(size),
        type(type),
        A(size * size),
        B(size * size),
        C(size * size) {}
};

static __thread int omp_dev = -1;
static __thread task_matrix_data *tdata = nullptr;

// Must be called by each cpu and omp thread!
void rtgauss_init(int size, rtgauss_type type, int omp_target_dev) {
    // Construct the data with the right size
    tdata = new task_matrix_data(size, type);
    omp_dev = omp_target_dev;

    // TODO: fill with different matrices perhaps?
    gauss_fill_eye_matrix(tdata->A.data(), tdata->size);
    gauss_fill_eye_matrix(tdata->B.data(), tdata->size);
    gauss_fill_eye_matrix(tdata->C.data(), tdata->size);
}

static uint64_t rtgauss_waste_time_cpu(uint64_t in) {
    // Operates on thread-private data of the right size!
    gauss_mul(tdata->A.data(), tdata->B.data(), tdata->C.data(), tdata->size);
    bool result = gauss_is_eye(tdata->C.data(), tdata->size);
    return in + ((result) ? 2 : 1);
}

#if RTDAG_OMP_SUPPORT == ON
static uint64_t rtgauss_waste_time_omp(uint64_t in) {
    // Operates on thread-private data of the right size!
    gauss_mul_omp_target(tdata->A.data(), tdata->B.data(), tdata->C.data(),
                         tdata->size, omp_dev);
    bool result =
        gauss_is_eye_omp_target(tdata->C.data(), tdata->size, omp_dev);
    return in + ((result) ? 2 : 1);
}
#endif

uint64_t rtgauss_waste_time(uint64_t in) {
    switch (tdata->type) {
    case RTGAUSS_CPU:
        return rtgauss_waste_time_cpu(in);
#if RTDAG_OMP_SUPPORT == ON
    case RTGAUSS_OMP:
        return rtgauss_waste_time_omp(in);
#endif
    default:
        fprintf(stderr, "ERROR: Invalid RTGAUSS type %d!\n", tdata->type);
        exit(EXIT_FAILURE);
    }
}
