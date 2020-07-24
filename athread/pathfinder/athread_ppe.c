#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <athread.h>

#define COLS 100000
#define ROWS 100 
#define M_SEED 9

#define MIN(x, y) ((x) < (y) ? (x) : (y))

int wall[ROWS][COLS];
int result[COLS];
int global_src[COLS];

struct func_args {
    int *src;
    int *dst;
    int t;
};

void init();
void run();
extern slave_spe_func(struct func_args *fa);

int main(int argc, char *argv[])
{
    athread_init();
    run();
    athread_halt();
    
    exit(EXIT_SUCCESS);
}

void init()
{
    int seed = M_SEED;
    int i, j;

    srand(seed);

    for (i = 0; i < ROWS; i++) {
        for (j = 0; j < COLS; j++) {
            wall[i][j] = rand() % 10;
        }
    }

    for (j = 0; j < COLS; j++)
        result[j] = wall[0][j];
}

void run()
{
    unsigned long long cycles;
    int *src, *dst, *temp;
    int min;
    int i, t, n;
    struct timeval start, end;
    double spend_time;
    struct func_args fa;

    dst = result;
    src = global_src;
    init();

    gettimeofday(&start, NULL);
    
    for (t = 0; t < ROWS - 1; t++) {
        temp = src;
        src = dst;
        dst = temp;
        fa.src = src;
        fa.dst = dst;
        fa.t = t;
        athread_spawn(spe_func, &fa);
        athread_join();
    }

    gettimeofday(&end, NULL);

    spend_time = \
        (end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec;

    for (i = 0; i < COLS; i++)
        printf("%d ", wall[0][i]);
    printf("\n");

    for (i = 0; i < COLS; i++)
        printf("%d ", dst[i]);
    printf("\n");

    printf("spend time: %lf(us)\n", spend_time);
}
