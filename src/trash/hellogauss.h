#ifndef HELLOGAUSS_H
#define HELLOGAUSS_H

#ifdef __cpluplus
extern "C" {
#endif

// NOTE: change here to test different matrix sizes (64, 128, etc)
#define GAUSS_SIZE 4
#define GAUSS_MSIZE (GAUSS_SIZE * GAUSS_SIZE)

#define gauss_at(m, i, j) m[i * GAUSS_SIZE + j]

#include <math.h>
#include <stdbool.h>

// Since this is an header-only matrix "library", all the functions are
// potentially unused.

static void gauss_invert(const double in[GAUSS_MSIZE], double out[GAUSS_MSIZE])
    __attribute__((unused));

static void gauss_transpose(const double in[GAUSS_MSIZE],
                            double out[GAUSS_MSIZE]) __attribute__((unused));

static void gauss_multiply(const double in1[GAUSS_MSIZE],
                           const double in2[GAUSS_MSIZE],
                           double out[GAUSS_MSIZE]) __attribute__((unused));

static bool gauss_is_identity(const double in[GAUSS_MSIZE])
    __attribute__((unused));

static inline double gauss_absd(double x) {
    if (x >= 0)
        return x;
    return -x;
}

static inline void gauss_swapd(double *x, double *y) {
    double t = *x;
    *x = *y;
    *y = t;
}

static void gauss_copy(const double in[GAUSS_MSIZE], double out[GAUSS_MSIZE]) {
    for (int i = 0; i < GAUSS_SIZE; ++i) {
        for (int j = 0; j < GAUSS_SIZE; ++j) {
            gauss_at(out, i, j) = gauss_at(in, i, j);
        }
    }
}

static void gauss_invert(const double in[GAUSS_MSIZE],
                         double out[GAUSS_MSIZE]) {
    double m[GAUSS_MSIZE];

    gauss_copy(in, m);

    // Initialize output as I
    for (int i = 0; i < GAUSS_SIZE; ++i) {
        for (int j = 0; j < GAUSS_SIZE; ++j) {
            gauss_at(out, i, j) = (i == j) ? 1 : 0;
        }
    }

    for (int i = 0; i < GAUSS_SIZE; ++i) {
        // Find max |m[i,j]|
        int imax = i;
        for (int i2 = i + 1; i2 < GAUSS_SIZE; ++i2) {
            if (gauss_absd(gauss_at(m, i2, i)) >
                gauss_absd(gauss_at(m, imax, i)))
                imax = i2;
        }

        // Swap rows i and imax if needed, both in m and in out
        if (i != imax) {
            for (int j = 0; j < GAUSS_SIZE; ++j) {
                gauss_swapd(&gauss_at(m, i, j), &gauss_at(m, imax, j));
                gauss_swapd(&gauss_at(out, i, j), &gauss_at(out, imax, j));
            }
        }

        // Divide row i by d
        double d = gauss_at(m, i, i);
        for (int j = 0; j < GAUSS_SIZE; j++) {
            gauss_at(m, i, j) /= d;
            gauss_at(out, i, j) /= d;
        }

        // Subtract from subsequent rows
        for (int i2 = i + 1; i2 < GAUSS_SIZE; i2++) {
            double f = gauss_at(m, i2, i);
            for (int j = 0; j < GAUSS_SIZE; j++) {
                gauss_at(m, i2, j) -= gauss_at(m, i, j) * f;
                gauss_at(out, i2, j) -= gauss_at(out, i, j) * f;
            }
        }
    }

    for (int i = GAUSS_SIZE - 1; i >= 0; i--) {
        // double d = gauss_at(m, i, i);
        for (int i2 = i - 1; i2 >= 0; i2--) {
            double f = gauss_at(m, i2, i);
            for (int j2 = 0; j2 < GAUSS_SIZE; j2++) {
                gauss_at(m, i2, j2) -= gauss_at(m, i, j2) * f;
                gauss_at(out, i2, j2) -= gauss_at(out, i, j2) * f;
            }
        }
    }
}

static void gauss_transpose(const double in[GAUSS_MSIZE],
                            double out[GAUSS_MSIZE]) {
    for (int i = 0; i < GAUSS_SIZE; i++) {
        for (int j = 0; j < GAUSS_SIZE; j++) {
            gauss_at(out, j, i) = gauss_at(in, i, j);
        }
    }
}

static void gauss_multiply(const double in1[GAUSS_MSIZE],
                           const double in2[GAUSS_MSIZE],
                           double out[GAUSS_MSIZE]) {
// NOTE: parallelising only the first two loops!
#pragma omp parallel for collapse(2)
    for (int i = 0; i < GAUSS_SIZE; i++) {
        for (int j = 0; j < GAUSS_SIZE; j++) {
            double acc = 0;
            for (int k = 0; k < GAUSS_SIZE; k++)
                acc += gauss_at(in1, i, k) * gauss_at(in2, k, j);
            gauss_at(out, i, j) = acc;
        }
    }
}

static inline bool gauss_is_equal(double x, double y) {
    const double epsilon = 1e-5;
    return gauss_absd(x - y) <=
           epsilon; // I know, doesn't make sense for big or small numbers
}

static bool gauss_is_identity(const double in[GAUSS_MSIZE]) {
    bool valid = true;
    bool temp;
    double test;

#pragma omp parallel for collapse(2) reduction(&& : valid)
    for (int i = 0; i < GAUSS_SIZE; ++i) {
        for (int j = 0; j < GAUSS_SIZE; ++j) {
            test = (i == j) ? 1.0 : 0.0;
            temp = gauss_is_equal(gauss_at(in, i, j), test);
            valid = valid && temp;
        }
    }

    return valid;
}

/*
#include <stdio.h>
static void gauss_dump(const double in[GAUSS_MSIZE]) {
    for (int i = 0; i < GAUSS_SIZE; ++i) {
        printf("|\t");
        for (int j = 0; j < GAUSS_SIZE; ++j) {
            printf("%f\t", gauss_at(in, i, j));
        }
        printf("|\n");
    }
    printf("\n");
}
*/

#ifdef __cpluplus
}
#endif

#endif // HELLOGAUSS_H
