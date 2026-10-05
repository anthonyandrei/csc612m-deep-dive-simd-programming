#include <stdio.h>
#include <math.h>
#include <time.h>

#define RUNS 30
#define _C 0
#define SCALAR 1
#define XMM 2
#define YMM 3

/**
 *
 */
typedef struct
{
    double accuracy;
    double duration_ms;
} Result;

// extern void matvec_scalar(long long m, long long n, double* A, double* X, double* Y);

extern void matvec_simd_xmm(int m, int n, float *A, float *X, float *Y);

extern void matvec_simd_ymm(int m, int n, float *A, float *X, float *Y);

void matvec_c(int m, int n, float *A, float *X, float *Y) {}

/**
 * @param runs number of times the benchmark will run
 * @param mode which function to test (C, SCALAR, XMM, or YMM)
 */
Result run_benchmark(int runs, int mode)
{
    int sizes[] = {pow(2, 20), pow(2, 26), pow(2, 30), pow(2, 30) + 1};
    int len_sizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int i = 0; i < RUNS; i++)
    {
        for (int j = 0; j < len_sizes; j++)
        {
            // init values for benchmark arrays
            int m = sqrt(sizes[j]);
            int n = sqrt(sizes[j]);
            float *A = malloc(sizes[j] * sizeof(float));
            float *X = malloc(m * sizeof(float));
            float *Y = malloc(n * sizeof(float));
            // TODO: set values in arrays

            if (mode == YMM)
            {
                clock_t start = clock();
                matvec_simd_ymm(m, n, A, X, Y);
                clock_t end = clock();

                double duration_ms = (double)(end - start) / CLOCKS_PER_SEC;
                // TODO: check answer function
            }
        }
    }
}

int main(void)
{
    // C
    // scalar
    // xmm
    // ymm
    Result ymm_result = run_benchmark(RUNS, YMM);

    return 0;
}