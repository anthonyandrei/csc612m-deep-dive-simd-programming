#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

// ============================================================================
// Kernel Declarations
// ============================================================================

// 1. C Baseline (Answer Key)
void matvec_c(long long m, long long n, const float *A, const float *X, float *Y);

// 2. x86-64 Scalar Assembly Kernel (implemented in matvec_scalar.asm)
extern void matvec_scalar(long long m, long long n, const float *A, const float *X, float *Y);

// 3. Placeholders for Group Members' SIMD Kernels:
// extern void matvec_simd_xmm(long long m, long long n, const float *A, const float *X, float *Y);
// extern void matvec_simd_ymm(long long m, long long n, const float *A, const float *X, float *Y);

// ============================================================================
// Timer Utilities (Cross-Platform)
// ============================================================================

#ifdef _WIN32
static double get_time_seconds(void) {
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    return (double)count.QuadPart / (double)freq.QuadPart;
}
#else
static double get_time_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}
#endif

// ============================================================================
// Kernel Implementation: C Baseline
// ============================================================================

void matvec_c(long long m, long long n, const float *A, const float *X, float *Y) {
    for (long long i = 0; i < m; i++) {
        float sum = 0.0f;
        const float *row = &A[i * n];
        for (long long j = 0; j < n; j++) {
            sum += row[j] * X[j];
        }
        Y[i] = sum;
    }
}

// ============================================================================
// Helper Utilities
// ============================================================================

void init_data(long long m, long long n, float *A, float *X) {
    for (long long i = 0; i < m * n; i++) {
        A[i] = (float)sin((double)i * 0.001);
    }
    for (long long j = 0; j < n; j++) {
        X[j] = (float)cos((double)j * 0.002);
    }
}

void print_sample_elements(const char *kernel_name, long long m, const float *Y) {
    printf("[%s] First 5 elements:\n  ", kernel_name);
    long long count_first = (m < 5) ? m : 5;
    for (long long i = 0; i < count_first; i++) {
        printf("%10.4f ", Y[i]);
    }
    printf("\n");

    if (m > 5) {
        printf("[%s] Last 5 elements:\n  ", kernel_name);
        long long start_last = (m - 5 > 0) ? (m - 5) : 0;
        for (long long i = start_last; i < m; i++) {
            printf("%10.4f ", Y[i]);
        }
        printf("\n");
    }
}

bool check_correctness(long long m, const float *expected, const float *actual, float epsilon) {
    for (long long i = 0; i < m; i++) {
        float diff = fabsf(expected[i] - actual[i]);
        float tol = epsilon * (1.0f + fabsf(expected[i]));
        if (diff > tol) {
            printf("  [FAIL] Discrepancy at index %lld: Expected=%f, Actual=%f (diff=%e, tol=%e)\n",
                   i, expected[i], actual[i], diff, tol);
            return false;
        }
    }
    return true;
}

// ============================================================================
// Benchmark Runner
// ============================================================================

void run_test(long long m, long long n, int iterations) {
    printf("======================================================================\n");
    printf("Matrix Vector Multiplication: m = %lld, n = %lld (Iterations: %d)\n", m, n, iterations);
    printf("======================================================================\n");

    size_t matrix_bytes = (size_t)m * (size_t)n * sizeof(float);
    size_t vec_x_bytes  = (size_t)n * sizeof(float);
    size_t vec_y_bytes  = (size_t)m * sizeof(float);

    float *A = (float *)malloc(matrix_bytes);
    float *X = (float *)malloc(vec_x_bytes);
    float *Y_c = (float *)malloc(vec_y_bytes);
    float *Y_scalar = (float *)malloc(vec_y_bytes);

    if (!A || !X || !Y_c || !Y_scalar) {
        fprintf(stderr, "Error: Memory allocation failed for size %lld x %lld.\n", m, n);
        free(A);
        free(X);
        free(Y_c);
        free(Y_scalar);
        return;
    }

    init_data(m, n, A, X);

    // ------------------------------------------------------------------------
    // 1. Run C Kernel (Answer Key)
    // ------------------------------------------------------------------------
    double total_time_c = 0.0;
    for (int it = 0; it < iterations; it++) {
        double start = get_time_seconds();
        matvec_c(m, n, A, X, Y_c);
        double end = get_time_seconds();
        total_time_c += (end - start);
    }
    double avg_time_c = (total_time_c / iterations) * 1000.0; // ms
    printf("\n[C Kernel Baseline]\n");
    printf("  Average Execution Time: %.4f ms (over %d runs)\n", avg_time_c, iterations);
    print_sample_elements("C Kernel", m, Y_c);

    // ------------------------------------------------------------------------
    // 2. Run x86-64 Scalar Assembly Kernel
    // ------------------------------------------------------------------------
    double total_time_scalar = 0.0;
    for (int it = 0; it < iterations; it++) {
        double start = get_time_seconds();
        matvec_scalar(m, n, A, X, Y_scalar);
        double end = get_time_seconds();
        total_time_scalar += (end - start);
    }
    double avg_time_scalar = (total_time_scalar / iterations) * 1000.0; // ms
    printf("\n[x86-64 Scalar Assembly Kernel]\n");
    printf("  Average Execution Time: %.4f ms (over %d runs)\n", avg_time_scalar, iterations);
    print_sample_elements("x86-64 Scalar", m, Y_scalar);

    // Correctness Verification against C
    bool is_correct = check_correctness(m, Y_c, Y_scalar, 1e-4f);
    if (is_correct) {
        printf("  Verification against C: [PASSED] (Output matches C baseline within tolerance)\n");
    } else {
        printf("  Verification against C: [FAILED]\n");
    }

    if (avg_time_scalar > 0.0) {
        double speedup = avg_time_c / avg_time_scalar;
        printf("  Speedup vs C: %.2fx\n", speedup);
    }

    printf("\n");

    free(A);
    free(X);
    free(Y_c);
    free(Y_scalar);
}

int main(void) {
    const int RUNS = 30; // Specification requires at least 30 executions

    printf("======================================================================\n");
    printf("  Deep Dive: SIMD Programming - Matrix-Vector Multiplication\n");
    printf("======================================================================\n\n");

    // Boundary condition test (non-power-of-2 / odd dimension)
    printf(">>> RUNNING BOUNDARY SITUATION TEST (e.g. n = 1003)\n");
    run_test(1003, 1003, RUNS);

    // Standard square matrix tests
    // Note: Since A is m x n, memory for matrix is m * n * 4 bytes.
    // n = 1024 (2^10) -> 1024^2 elements = 4 MB
    // n = 2048 (2^11) -> 2048^2 elements = 16 MB
    // n = 4096 (2^12) -> 4096^2 elements = 64 MB
    // n = 8192 (2^13) -> 8192^2 elements = 256 MB (Total 2^26 elements)
    printf(">>> RUNNING BENCHMARK: n = 1024 (2^10 x 2^10)\n");
    run_test(1024, 1024, RUNS);

    printf(">>> RUNNING BENCHMARK: n = 4096 (2^12 x 2^12)\n");
    run_test(4096, 4096, RUNS);

    printf(">>> RUNNING BENCHMARK: n = 8192 (2^13 x 2^13, 2^26 matrix elements)\n");
    run_test(8192, 8192, RUNS);

    return 0;
}