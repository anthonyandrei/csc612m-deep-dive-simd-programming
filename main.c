#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <windows.h>

#define RUNS 30
#define PRINT_HEADER_COUNT 5

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
    printf("First %d:", PRINT_HEADER_COUNT);
    for (int i = 0; i < PRINT_HEADER_COUNT && i < n; i++)
    {
        printf(" %f", result[i]);
    }
    printf("\nLast %d:", PRINT_HEADER_COUNT);
    int startIndex = max(0, n - PRINT_HEADER_COUNT); // start at 0 if there are fewer than 5 elements
    for (int i = startIndex; i < n; i++)
    {
        printf(" %f", result[i]);
    }
    printf("\n");
}

/**
 * @param result The kernel result to check.
 * @param cResult The C reference result.
 * @param n The number of elements in each result array.
 * @return 1 if the results match, 0 otherwise.
 */
int verifyResults(float *result, float *cResult, int n)
{
    for (int i = 0; i < n; i++)
    {
        if (result[i] != cResult[i])
        {
            return 0;
        }
    }
    return 1;
}

/**
 * @param n The side length of the matrix.
 * @return 1 if all kernels pass, 0 if allocation or verification fails.
 * Runs each kernel 30 times and prints its average time and results.
 */
static int run_benchmark(int n)
{
    int matrixCount = n * n;
    float *A = malloc(matrixCount * sizeof *A);
    float *X = malloc(n * sizeof *X);
    float *Y = malloc(n * sizeof *Y);
    float *cResult = malloc(n * sizeof *cResult);
    if (A == NULL || X == NULL || Y == NULL || cResult == NULL)
    {
        fprintf(stderr, "Allocation failed for n=%d\n", n);
        free(A);
        free(X);
        free(Y);
        free(cResult);
        return 0;
    }

    // repeat values from 1 to 10, as in the previous SIMD seatwork
    for (int i = 0; i < matrixCount; i++)
    {
        A[i] = (float)(i % 10 + 1);
    }
    for (int i = 0; i < n; i++)
    {
        X[i] = (float)(i % 10 + 1);
    }
    printf("\n--------------------------------\n");
    printf("n=%d, matrix elements=%d, runs=%d\n", n, matrixCount, RUNS);
	printf("--------------------------------\n");
    printf("Matrix A:\n");
    printResults(A, matrixCount);
    printf("Vector X:\n");
    printResults(X, n);

    char *names[] = {"C", "YMM"};
    int kernelCount = sizeof names / sizeof names[0];
    LARGE_INTEGER start, end, freq;
    QueryPerformanceFrequency(&freq); // get QPC's count/sec
    int allCorrect = 1;

    for (int kernel = 0; kernel < kernelCount; kernel++)
    {
        float *result = kernel == 0 ? cResult : Y;
        double totalTimeInMs = 0.0;
        int kernelCorrect = 1;
        for (int run = 0; run < RUNS; run++)
        {
            // clear output outside timing so a missing store cannot reuse a result
            for (int i = 0; i < n; i++)
            {
                result[i] = NAN;
            }

            QueryPerformanceCounter(&start);
            if (kernel == 0)
            {
                matvec_c(n, n, A, X, result);
            }
            else
            {
                matvec_simd_ymm(n, n, A, X, result);
            }
            QueryPerformanceCounter(&end);
            
            // counts -> secs -> ms
            totalTimeInMs += 1000.0 * (double)(end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
            
            if (!verifyResults(result, cResult, n))
            {
                kernelCorrect = 0;
            }
        }
        printf("\nResults of %s version:\n", names[kernel]);
        printf("Average duration: %f ms\n", totalTimeInMs / RUNS);
        printResults(result, n);
        if (kernel != 0)
        {
            printf("Results match C: %d\n", kernelCorrect);
        }
        if (!kernelCorrect)
        {
            allCorrect = 0;
        }
    }

    free(A);
    free(X);
    free(Y);
    free(cResult);
    return allCorrect;
}

int main(void)
{
    int dimensions[] = {1 << 10, 1 << 13, 1 << 15, 1003};
    int count = sizeof dimensions / sizeof dimensions[0];
    for (int i = 0; i < count; i++)
    {
        if (!run_benchmark(dimensions[i]))
        {
            return 1;
        }
    }
    return 0;
}
