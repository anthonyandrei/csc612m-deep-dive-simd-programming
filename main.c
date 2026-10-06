#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <windows.h>

#define RUNS 30
#define PRINT_COUNT 5
#define _C_ 0
#define SCALAR 1
#define XMM 2
#define YMM 3


// add the scalar declaration when its NASM version is ready
extern void matvec_simd_ymm(int m, int n, float *A, float *X, float *Y);

/**
 * @param m The number of rows in A.
 * @param n The number of columns in A and elements in X.
 * @param A The matrix stored in row-major order.
 * @param X The input vector.
 * @param Y The vector where the result will be stored.
 * Computes the matrix-vector product as the C reference.
 */
void matvec_c(int m, int n, float *A, float *X, float *Y)
{
    if (m <= 0 || n <= 0 || A == NULL || X == NULL || Y == NULL)
        return;

    for (int i = 0; i < m; i++)
    {
        float sum = 0.0f;
        for (int j = 0; j < n; j++)
        {
            sum += A[i * n + j] * X[j];
        }
        Y[i] = sum;
    }
}

/**
 * @param result The result array to print.
 * @param n The number of elements in the array.
 * Prints the first and last five elements of the array.
 */
void printResults(float *result, int n)
{
    printf("First %d:", PRINT_COUNT);
    for (int i = 0; i < PRINT_COUNT && i < n; i++)
    {
        printf(" %f", result[i]);
    }

    printf("\nLast %d:", PRINT_COUNT);
    int startIndex = max(0, n - PRINT_COUNT); // start at 0 if there are fewer than 5 elements
    for (int i = startIndex; i < n; i++)
    {
        printf(" %f", result[i]);
    }
    printf("\n");
}

/**
 * @param result The kernel result to check.
 * @param Y_c The C reference result.
 * @param n The number of elements in each result array.
 * @return 1 if the results match, 0 otherwise.
 */
int verifyResults(float *result, float *Y_c, int n)
{
    for (int i = 0; i < n; i++)
    {
        if (result[i] != Y_c[i])
        {
            return 0;
        }
    }
    return 1;
}

/**
 * @param n The side length of the matrix.
 * @param A The initialized matrix.
 * @param X The initialized input vector.
 * @param Y_scalar The scalar assembly output buffer.
 * @param Y_xmm The XMM output buffer.
 * @param Y_ymm The YMM output buffer.
 * @param Y_c The C reference output buffer.
 * @return 1 if all kernels pass, 0 if verification fails.
 * Runs each kernel 30 times and prints its average time and results.
 */
static int run_benchmark(int n, float *A, float *X, float *Y_scalar, float *Y_xmm, float *Y_ymm, float *Y_c)
{
    char *names[] = {"C", "Scalar", "XMM", "YMM"};
    float *results[] = {Y_c, Y_scalar, Y_xmm, Y_ymm};
    int kernelCount = sizeof names / sizeof names[0];
    LARGE_INTEGER start, end, freq;
    QueryPerformanceFrequency(&freq); // get QPC's count/sec
    int isAllCorrect = 1;

    //per kernel
    for (int kernel = 0; kernel < kernelCount; kernel++)
    {
        float *result = results[kernel];
        double totalTimeInMs = 0.0;
        int isKernelCorrect = 1;

        // run each kernel RUNS times
        for (int run = 0; run < RUNS; run++)
        {
            // clear result
            for (int i = 0; i < n; i++)
            {
                result[i] = NAN;
            }

            QueryPerformanceCounter(&start);    //start count
            switch (kernel) {
                case _C_:
                    matvec_c(n, n, A, X, result);
                    break;
                case YMM:
                    matvec_simd_ymm(n, n, A, X, result);
                    break;
                default:
                    break;
            }
            QueryPerformanceCounter(&end);  //end count

            // counts -> secs -> ms
            totalTimeInMs += 1000.0 * (end.QuadPart - start.QuadPart) / freq.QuadPart;

            if (!verifyResults(result, Y_c, n))
            {
                isKernelCorrect = 0;
            }
        }
        printf("\nResults of %s version:\n", names[kernel]);
        printf("Average duration: %f ms\n", totalTimeInMs / RUNS);
        printResults(result, n);

        //verify results against C reference
        if (kernel != 0)
        {
            printf("Results match C: %d\n", isKernelCorrect);
        }
        if (!isKernelCorrect)
        {
            isAllCorrect = 0;
        }
    }

    return isAllCorrect;
}

int main(void)
{
    int n = 1 << 20;
    long long matrixCount = (long long)n * n;
    float *A = malloc(matrixCount * sizeof *A);
    float *X = malloc(n * sizeof *X);
    float *Y_scalar = malloc(n * sizeof *Y_scalar);
    float *Y_xmm = malloc(n * sizeof *Y_xmm);
    float *Y_ymm = malloc(n * sizeof *Y_ymm);
    float *Y_c = malloc(n * sizeof *Y_c);
    if (A == NULL || X == NULL || Y_scalar == NULL || Y_xmm == NULL || Y_ymm == NULL || Y_c == NULL)
    {
        fprintf(stderr, "Allocation failed for n=%d\n", n);
        free(A);
        free(X);
        free(Y_scalar);
        free(Y_xmm);
        free(Y_ymm);
        free(Y_c);
        return 1;
    }

    // repeat values from 1 to 10
    for (long long i = 0; i < matrixCount; i++)
    {
        A[i] = (float)(i % 10 + 1);
    }
    for (int i = 0; i < n; i++)
    {
        X[i] = (float)(i % 10 + 1);
    }
    printf("\n--------------------------------\n");
    printf("n=%d, matrix elements=%d, runs=%lld\n", n, matrixCount, RUNS);
	printf("--------------------------------\n");
    printf("Matrix A:\n");
    printResults(A, matrixCount);
    printf("Vector X:\n");
    printResults(X, n);

    run_benchmark(n, A, X, Y_scalar, Y_xmm, Y_ymm, Y_c);
    free(A);
    free(X);
    free(Y_scalar);
    free(Y_xmm);
    free(Y_ymm);
    free(Y_c);

    return 0;
}
