#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <athread.h>
#include <sys/time.h>

#define M 5000
#define N 50000
#define MAX_THREAD 64

double b[N];
double x[N];
double xnew[N];
double d;
double r;
double d_array[MAX_THREAD];
double r_array[MAX_THREAD];


extern void slave_func();

int main()
{
    int i;
    int it;
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
    r = 0;
    d = 0;
    athread_init();

    athread_spawn(func, NULL);
    athread_join();
    for (i = 0; i < MAX_THREAD; i++) {
        r += r_array[i];
        d += r_array[i];
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

    athread_halt();

    return 0;
}