#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define M 5000
#define N 50000

double b[N];
double x[N];
double xnew[N];

int main()
{
    double d;
    int i;
    int it;
    double r;
    double t;
    double spendtime;
    struct timeval start, end;

    printf("\n");
    printf("JACOBI_OPENMP:\n");
    printf("  C/OpenMP version\n");
    printf("  Jacobi iteration to solve A*x=b\n");
    printf("\n");
    printf("  Number of variables N = %d\n", N);
    printf("  Number of iterations M = %d\n", M);

    printf("\n");
    printf("  IT    l2(dX)  l2(resid)\n");
    printf("\n");

    gettimeofday(&start, NULL);

    for ( i = 0; i < N; i++) {
        b[i] = 0.0;
        x[i] = 0.0;
    }
    b[N-1] = (double) (N + 1);
    for (it = 0; it < M; it++) {
        for (i = 0; i < N; i++) {
            xnew[i] = b[i];
            if (0 < i) {
                xnew[i] = xnew[i] + x[i - 1];
            }
            if (i < N - 1) {
                xnew[i] = xnew[i] + x[i + 1];
            }
            xnew[i] = xnew[i] / 2.0;
        }

        d = 0.0;
        for (i = 0; i < N; i++) {
            d = d + pow(x[i] - xnew[i], 2);
        }

        for (i = 0; i < N; i++) {
            x[i] = xnew[i];
        }

        r = 0.0;
        for (i = 0; i < N; i++) {
            t = b[i] - 2.0 * x[i];
            if (0 < i) {
                t = t + x[i - 1];
            }
            if (i < N - 1) {
                t = t + x[i + 1];
            }
            r = r + t * t;
        }

        /* 中间结果不在输出 */
    }

    gettimeofday(&end, NULL);
    spendtime = 1000000*(end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;

    printf("\n");
    printf("  Part of final solutions estimate:\n");
    printf("\n");
    for ( i = 0; i < 10; i++) {
        printf(" %8d %14.6g\n", i, x[i]);
    }
    printf("...\n");
    for (i = N - 11; i < N; i++) {
        printf(" %8d %14.6g\n", i, x[i]);
    }

    printf("\n");
    printf("JACOBI_OPENMP:\n");
    printf("  Normal end of execution.\n");
    printf("spend time:%lf us.\n", spendtime);

    return 0;
}
