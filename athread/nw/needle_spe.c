#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <slave.h>

#define LIMIT -999
#define BLOCK_SIZE 16
#define MAX_THREADS 64

#define MAX_ROWS (2048 + 1)
#define MAX_COLS (2048 + 1)
#define PENALTY 10
extern int blosum62[24][24];

extern int reference[MAX_ROWS][MAX_COLS];
extern int input_itemsets[MAX_ROWS][MAX_COLS];
extern int output_itemsets[MAX_ROWS][MAX_COLS];

__thread_local int input_itemsets_l[BLOCK_SIZE + 1][BLOCK_SIZE + 1];
__thread_local int reference_l[BLOCK_SIZE][BLOCK_SIZE];

int maximum(int a, int b, int c)
{
    int k;
    if (a <= b)
        k = b;
    else
        k = a;
    
    if (k <= c)
        return c;
    else
        return k;
}

void spe_func_1(int *blk_p)
{
    int tid, low, high, chunk;
    int blk = *blk_p;
    int b_index_x;
    volatile int DMA_reply;
    int i, j;

    tid = _MYID;
    chunk = blk / MAX_THREADS;
    low = tid * chunk;
    high = (tid == MAX_THREADS - 1) ? blk : (tid + 1) * chunk;

    for (b_index_x = low; b_index_x < high; b_index_x++) {
        int b_index_y = blk - 1 - b_index_x;
        DMA_reply = 0;
        for (i = 0; i < BLOCK_SIZE; i++) {
            athread_get(PE_MODE, \
                &reference[b_index_y * BLOCK_SIZE + i + 1][b_index_x * BLOCK_SIZE + 1], \
                reference_l[i], sizeof(int) * BLOCK_SIZE, \
                &DMA_reply, 0, 0, 0);
        }
        while (DMA_reply != BLOCK_SIZE);
        DMA_reply = 0;
        for (i = 0; i < BLOCK_SIZE + 1; i++) {
            athread_get(PE_MODE, \
                &input_itemsets[b_index_y * BLOCK_SIZE + i][b_index_x * BLOCK_SIZE], \
                input_itemsets_l[i], sizeof(int) * (BLOCK_SIZE + 1), \
                &DMA_reply, 0, 0, 0);
        }
        while (DMA_reply != BLOCK_SIZE + 1);

        for (i = 1; i < BLOCK_SIZE + 1; i++) {
            for (j = 1; j < BLOCK_SIZE + 1; j++) {
                input_itemsets_l[i][j] = maximum(input_itemsets_l[i - 1][j - 1] + reference_l[i - 1][j - 1],
                    input_itemsets_l[i][j - 1] - PENALTY,
                    input_itemsets_l[i - 1][j] - PENALTY);
            }
        }
        DMA_reply = 0;
        for (i = 0; i < BLOCK_SIZE; i++) {
            athread_put(PE_MODE, &input_itemsets_l[i + 1][1], \
                        &input_itemsets[b_index_y * BLOCK_SIZE + i + 1][b_index_x * BLOCK_SIZE + 1], \
                        sizeof(int) * BLOCK_SIZE, &DMA_reply, 0, 0);
        }
        while (DMA_reply != BLOCK_SIZE);
    }
}

void spe_func_2(int *blk_p)
{
    int tid, low, high, chunk;
    int blk = *blk_p;
    int global_low = blk - 1;
    int global_high = (MAX_COLS - 1) / BLOCK_SIZE;
    int b_index_x;
    volatile int DMA_reply = 0;
    int i, j;

    tid = _MYID;
    chunk = (global_high - global_low) / MAX_THREADS;
    low = chunk * tid + global_low;
    high = (tid == MAX_THREADS - 1) ? global_high : ((tid + 1) * chunk + global_low);

    for (b_index_x = low; b_index_x < high; b_index_x++) {
        int b_index_y = (MAX_COLS - 1) / BLOCK_SIZE + blk - 2 - b_index_x;
        DMA_reply = 0;
        for (i = 0; i < BLOCK_SIZE; i++) {
            athread_get(PE_MODE, \
                &reference[b_index_y * BLOCK_SIZE + i + 1][b_index_x * BLOCK_SIZE + 1], \
                reference_l[i], sizeof(int) * BLOCK_SIZE, &DMA_reply, 0, 0, 0);
        }
        while (DMA_reply != BLOCK_SIZE);

        DMA_reply = 0;
        for (i = 0; i < BLOCK_SIZE + 1; i++) {
            athread_get(PE_MODE, \
                &input_itemsets[b_index_y * BLOCK_SIZE + i][b_index_x * BLOCK_SIZE], \
                input_itemsets_l[i], sizeof(int) * (BLOCK_SIZE + 1), \
                &DMA_reply, 0, 0, 0);
        }
        while (DMA_reply != BLOCK_SIZE + 1);

        for (i = 1; i < BLOCK_SIZE + 1; i++) {
            for (j = 1; j < BLOCK_SIZE + 1; j++) {
                input_itemsets_l[i][j] = maximum(input_itemsets_l[i - 1][j - 1] + reference_l[i - 1][j - 1], \
                                input_itemsets_l[i][j - 1] - PENALTY, \
                                input_itemsets_l[i - 1][j] - PENALTY);
            }
        }

        DMA_reply = 0;
        for (i = 0; i < BLOCK_SIZE; i++) {
            athread_put(PE_MODE, &input_itemsets_l[i + 1][1], \
                &input_itemsets[b_index_y * BLOCK_SIZE + i + 1][b_index_x * BLOCK_SIZE + 1], \
                sizeof(int) * BLOCK_SIZE, &DMA_reply, 0, 0);
        }
        while (DMA_reply != BLOCK_SIZE);
    }
}
