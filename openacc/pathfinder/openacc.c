#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define COLS 100000
#define ROWS 100 
#define M_SEED 9

#define MIN(x, y) ((x) < (y) ? (x) : (y))

int wall[ROWS][COLS];
int result[COLS];
int global_src[COLS];

void init();
void run();

int main(int argc, char *argv[])
{
    run();
    
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
    // unsigned long long cycles;
    int *src, *dst, *temp;
    int min;
    int t, n;
    struct timeval start, end;
    double spend_time;

    dst = result;
    src = global_src;
    init();

    gettimeofday(&start, NULL);
    
    for (t = 0; t < ROWS - 1; t++) {
        temp = src;
        src = dst;
        dst = temp;
        int k = t + 1;

        // #pragma acc parallel local(min) annotate(readonly(t))
        #pragma acc parallel local(min) cache(wall, src) copyout(dst) firstprivate(dst) annotate(readonly(t, wall, src); dimension(dst(100000)))
        {
            #pragma acc loop
            for (n = 0; n < COLS; n++) {
                min = src[n];
                if (n > 0)
                    min = MIN(min, src[n - 1]);
                if (n < COLS  - 1)
                    min = MIN(min, src[n + 1]);

                // #pragma acc data copyin(wall[k][*])
                dst[n] = wall[k][n] + min;
            }
        }
    }

    gettimeofday(&end, NULL);

    spend_time = \
        (end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec;

    // int i;
    // for (i = 0; i < COLS; i++)
    //     printf("%d ", wall[0][i]);
    // printf("\n");

    // for (i = 0; i < COLS; i++)
    //     printf("%d ", dst[i]);
    // printf("\n");

    printf("spend time: %lf(us)\n", spend_time);
}

