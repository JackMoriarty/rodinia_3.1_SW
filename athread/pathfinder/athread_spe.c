#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <slave.h>

#define COLS 100000
#define ROWS 100 
#define M_SEED 9
#define MAX_THREADS 64

#define MIN(x, y) ((x) < (y) ? (x) : (y))

extern int wall[ROWS][COLS];
extern int result[COLS];
extern int global_src[COLS];

struct func_args {
    int *src;
    int *dst;
    int t;
};

void spe_func(struct func_args *fa)
{
    int *src = fa->src;
    int *dst = fa->dst;
    int t = fa->t;
    int n, min, i, j;
    int tid, chunk, low, high, size;
    int *local_src;
    int *local_dst;
    int *local_wall;
    volatile int DMA_reply;

    tid = _MYID;
    chunk = COLS / MAX_THREADS;
    low = tid * chunk;
    high = (tid == MAX_THREADS - 1) ? COLS : (tid + 1) * chunk;
    size = high - low;
    
    local_src = (int *)ldm_malloc(sizeof(int) * (size + 2));
    local_dst = (int *)ldm_malloc(sizeof(int) * size);
    local_wall = (int *)ldm_malloc(sizeof(int) * size);
    if (local_src == NULL || local_dst == NULL || local_wall == NULL) {
        printf("[%s:%d](%d): alloc ldm failed !\n", __FILE__, __LINE__, _MYID);
        exit(EXIT_FAILURE);
    }

    DMA_reply = 0;
    if (low == 0) {
        athread_get(PE_MODE, &src[low], local_src, sizeof(int) * (size + 1), \
                    &DMA_reply, 0, 0, 0);
    } else if (high == COLS){
        athread_get(PE_MODE, &src[low - 1], local_src, sizeof(int) * (size + 1), \
                    &DMA_reply, 0, 0, 0);
    }else {
        athread_get(PE_MODE, &src[low - 1], local_src, sizeof(int) * (size + 2), \
                    &DMA_reply, 0, 0, 0);
    }
    athread_get(PE_MODE, &wall[t + 1][low], local_wall, sizeof(int) * size, \
                &DMA_reply, 0, 0, 0);
    while(DMA_reply != 2);

    for (n = low; n < high; n++) {
        j = n - low;
        i = (low == 0) ? j : (j + 1);

        min = local_src[i];
        if (n > 0)
            min = MIN(min, local_src[i - 1]);
        if (n < COLS - 1)
            min = MIN(min, local_src[i + 1]);

        local_dst[j] = local_wall[j] + min;
    }

    DMA_reply = 0;
    athread_put(PE_MODE, local_dst, &dst[low], sizeof(int) * size, \
            &DMA_reply, 0, 0);
    while (DMA_reply != 1);

    ldm_free(local_src, sizeof(int) * (size + 2));
    ldm_free(local_dst, sizeof(int) * size);
    ldm_free(local_wall, sizeof(int) * size);
}
