#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#define LIMIT -999
#define BLOCK_SIZE 16

#define MAX_ROWS (2048 + 1)
#define MAX_COLS (2048 + 1)
#define PENALTY 10

int blosum62[24][24] = {
    { 4, -1, -2, -2,  0, -1, -1,  0, -2, -1, -1, -1, -1, -2, -1,  1,  0, -3, -2,  0, -2, -1,  0, -4},
    {-1,  5,  0, -2, -3,  1,  0, -2,  0, -3, -2,  2, -1, -3, -2, -1, -1, -3, -2, -3, -1,  0, -1, -4},
    {-2,  0,  6,  1, -3,  0,  0,  0,  1, -3, -3,  0, -2, -3, -2,  1,  0, -4, -2, -3,  3,  0, -1, -4},
    {-2, -2,  1,  6, -3,  0,  2, -1, -1, -3, -4, -1, -3, -3, -1,  0, -1, -4, -3, -3,  4,  1, -1, -4},
    { 0, -3, -3, -3,  9, -3, -4, -3, -3, -1, -1, -3, -1, -2, -3, -1, -1, -2, -2, -1, -3, -3, -2, -4},
    {-1,  1,  0,  0, -3,  5,  2, -2,  0, -3, -2,  1,  0, -3, -1,  0, -1, -2, -1, -2,  0,  3, -1, -4},
    {-1,  0,  0,  2, -4,  2,  5, -2,  0, -3, -3,  1, -2, -3, -1,  0, -1, -3, -2, -2,  1,  4, -1, -4},
    { 0, -2,  0, -1, -3, -2, -2,  6, -2, -4, -4, -2, -3, -3, -2,  0, -2, -2, -3, -3, -1, -2, -1, -4},
    {-2,  0,  1, -1, -3,  0,  0, -2,  8, -3, -3, -1, -2, -1, -2, -1, -2, -2,  2, -3,  0,  0, -1, -4},
    {-1, -3, -3, -3, -1, -3, -3, -4, -3,  4,  2, -3,  1,  0, -3, -2, -1, -3, -1,  3, -3, -3, -1, -4},
    {-1, -2, -3, -4, -1, -2, -3, -4, -3,  2,  4, -2,  2,  0, -3, -2, -1, -2, -1,  1, -4, -3, -1, -4},
    {-1,  2,  0, -1, -3,  1,  1, -2, -1, -3, -2,  5, -1, -3, -1,  0, -1, -3, -2, -2,  0,  1, -1, -4},
    {-1, -1, -2, -3, -1,  0, -2, -3, -2,  1,  2, -1,  5,  0, -2, -1, -1, -1, -1,  1, -3, -1, -1, -4},
    {-2, -3, -3, -3, -2, -3, -3, -3, -1,  0,  0, -3,  0,  6, -4, -2, -2,  1,  3, -1, -3, -3, -1, -4},
    {-1, -2, -2, -1, -3, -1, -1, -2, -2, -3, -3, -1, -2, -4,  7, -1, -1, -4, -3, -2, -2, -1, -2, -4},
    { 1, -1,  1,  0, -1,  0,  0,  0, -1, -2, -2,  0, -1, -2, -1,  4,  1, -3, -2, -2,  0,  0,  0, -4},
    { 0, -1,  0, -1, -1, -1, -1, -2, -2, -1, -1, -1, -1, -2, -1,  1,  5, -2, -2,  0, -1, -1,  0, -4},
    {-3, -3, -4, -4, -2, -2, -3, -2, -2, -3, -2, -3, -1,  1, -4, -3, -2, 11,  2, -3, -4, -3, -2, -4},
    {-2, -2, -2, -3, -2, -1, -2, -3,  2, -1, -1, -2, -1,  3, -3, -2, -2,  2,  7, -1, -3, -2, -1, -4},
    { 0, -3, -3, -3, -1, -2, -2, -3, -3,  3,  1, -2,  1, -1, -2, -2,  0, -3, -1,  4, -3, -2, -1, -4},
    {-2, -1,  3,  4, -3,  0,  1, -1,  0, -3, -4,  0, -3, -3, -2,  0, -1, -4, -3, -3,  4,  1, -1, -4},
    {-1,  0,  0,  1, -3,  3,  4, -2,  0, -3, -3,  1, -1, -3, -1,  0, -1, -3, -2, -2,  1,  4, -1, -4},
    { 0, -1, -1, -1, -2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -2,  0,  0, -2, -1, -1, -1, -1, -1, -4},
    {-4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4,  1}
};

int reference[MAX_ROWS][MAX_COLS];
int input_itemsets[MAX_ROWS][MAX_COLS];
int output_itemsets[MAX_ROWS][MAX_COLS];

void runTest();
void nw_optimized();

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

int main(int argc, char *argv[])
{
    runTest();
    exit(EXIT_SUCCESS);
}

void runTest()
{
    int i, j;
    struct timeval start, end;
    double spend_time;

    srand(7);

    for (i = 0; i < MAX_ROWS; i++) {
        for (j = 0; j < MAX_COLS; j++) {
            input_itemsets[i][j] = 0;
        }
    }

    printf("Start Needleman-Wunsch\n");

    for (i = 1; i < MAX_ROWS; i++)
        input_itemsets[i][0] = rand() % 10 + 1;

    for (j = 1; j < MAX_COLS; j++)
        input_itemsets[0][j] = rand() % 10 + 1;
    
    for (i = 1; i < MAX_ROWS; i++) {
        for (j = 1; j < MAX_COLS; j++) {
            reference[i][j] = \
                blosum62[input_itemsets[i][0]][input_itemsets[0][j]];
        }
    }

    for (i = 1; i < MAX_ROWS; i++)
        input_itemsets[i][0] = -i * PENALTY;
    for (j = 1; j < MAX_COLS; j++)
        input_itemsets[0][j] = -j * PENALTY;
    
    printf("Processing top left matrix\n");

    gettimeofday(&start, NULL);
    nw_optimized();
    gettimeofday(&end, NULL);
    spend_time = (end.tv_sec - start.tv_sec) * 1000000 + \
                    end.tv_usec - start.tv_usec;
    printf("Total time: %lf (us)\n", spend_time);

    FILE *fpo = fopen("result.txt", "w");
    fprintf(fpo, "print traceback value GPU:\n");

    for (i = MAX_ROWS - 2, j = MAX_ROWS - 2; i >= 0, j >= 0;) {
        int nw, n, w, traceback;
        if (i == MAX_ROWS - 2 && j == MAX_ROWS - 2)
            fprintf(fpo, "%d ", input_itemsets[i][j]);
        if (i == 0 && j == 0)
            break;
        if (i > 0 && j > 0) {
            nw = input_itemsets[i-1][j - 1];
            w = input_itemsets[i][j - 1];
            n = input_itemsets[i - 1][j];
        } else if (i == 0) {
            nw = n = LIMIT;
            w = input_itemsets[i][j - 1];
        } else if (j == 0) {
            nw = w = LIMIT;
            n = input_itemsets[i - 1][j];
        }

        int new_nw, new_w, new_n;
        new_nw = nw + reference[i][j];
        new_w = w - PENALTY;
        new_n = n - PENALTY;

        traceback = maximum(new_nw, new_w, new_n);
        if (traceback == new_nw)
            traceback = nw;
        if (traceback == new_w)
            traceback = w;
        if (traceback == new_n) 
            traceback = n;
        
        fprintf(fpo, "%d ", traceback);

        if (traceback == nw) {
            i--;
            j--;
            continue;
        } else if (traceback == w) {
            j--;
            continue;
        } else if (traceback == n) {
            i--;
            continue;
        }
    }

    fclose(fpo);
}

void nw_optimized()
{
    int blk;
    int b_index_x;
    int i, j;

    for (blk = 1; blk <= (MAX_COLS - 1) / BLOCK_SIZE; blk++){
        /**************** openMP ****************/
        #pragma acc parallel for shared(input_itemsets, referrence) firstprivate(blk, max_rows, max_cols, penalty)
        for (b_index_x = 0; b_index_x < blk; b_index_x++) {
            int b_index_y = blk - 1 - b_index_x;
            int input_itemsets_l[BLOCK_SIZE + 1][BLOCK_SIZE + 1]__attribute__((aligned(64)));
            int reference_l[BLOCK_SIZE][BLOCK_SIZE]__attribute__((aligned(64)));

            for (i = 0; i < BLOCK_SIZE; i++) {
                #pragma acc loop vector
                for (j = 0; j < BLOCK_SIZE; j++) {
                    reference_l[i][j] = reference[b_index_y * BLOCK_SIZE + i + 1][b_index_x * BLOCK_SIZE + j + 1];
                }
            }

            for (i = 0; i < BLOCK_SIZE + 1; i++) {
                #pragma acc loop vector
                for (j = 0; j < BLOCK_SIZE + 1; j++) {
                    input_itemsets_l[i][j] = input_itemsets[b_index_y * BLOCK_SIZE + i][b_index_x * BLOCK_SIZE + j];
                }
            }

            for (i = 1; i < BLOCK_SIZE + 1; i++) {
                for (j = 1; j < BLOCK_SIZE + 1; j++) {
                    input_itemsets_l[i][j] = maximum(input_itemsets_l[i - 1][j - 1] + reference_l[i - 1][j - 1],
                            input_itemsets_l[i][j - 1] - PENALTY,
                            input_itemsets_l[i - 1][j] - PENALTY);
                }
            }

            for (i = 0; i < BLOCK_SIZE; i++) {
                #pragma acc loop vector
                for (j = 0; j < BLOCK_SIZE; j++) {
                    input_itemsets[b_index_y * BLOCK_SIZE + i + 1][b_index_x * BLOCK_SIZE +  j + 1] = input_itemsets_l[i + 1][j + 1];
                }
            }
        }
        /************* end of openMP *************/
    }

    printf("Processing bottom-right matrix\n");

    for (blk = 2; blk <= (MAX_COLS - 1) / BLOCK_SIZE; blk++) {
        /***************** openMP ***************/
        #pragma acc parallel for shared(input_itemsets, referrence) firstprivate(blk, max_rows, max_cols, penalty)
        for (b_index_x = blk - 1; b_index_x < (MAX_COLS - 1) / BLOCK_SIZE; b_index_x++) {
            int b_index_y = (MAX_COLS - 1) / BLOCK_SIZE + blk - 2 - b_index_x;
            int input_itemsets_l[BLOCK_SIZE + 1][BLOCK_SIZE + 1]__attribute__((aligned(64)));
            int reference_l[BLOCK_SIZE][BLOCK_SIZE]__attribute__((aligned(64)));

            for (i = 0; i < BLOCK_SIZE; i++) {
                #pragma acc loop vector
                for (j = 0; j < BLOCK_SIZE; j++) {
                    reference_l[i][j] = reference[b_index_y * BLOCK_SIZE + i + 1][b_index_x*BLOCK_SIZE +  j + 1];
                }
            }

            for (i = 0; i < BLOCK_SIZE + 1; i++) {
                #pragma acc loop vector
                for (j = 0; j < BLOCK_SIZE + 1; j++) {
                    input_itemsets_l[i][j] = input_itemsets[b_index_y * BLOCK_SIZE + i][b_index_x * BLOCK_SIZE + j];
                }
            }

            for (i = 1; i < BLOCK_SIZE + 1; i++) {
                for (j = 1; j < BLOCK_SIZE + 1; j++) {
                    input_itemsets_l[i][j] = maximum(input_itemsets_l[i - 1][j - 1] + reference_l[i - 1][j - 1],
                            input_itemsets_l[i][j - 1] - PENALTY,
                            input_itemsets_l[i - 1][j] - PENALTY);
                }
            }

            for (i = 0; i < BLOCK_SIZE; i++) {
                #pragma acc loop vector
                for (j = 0; j < BLOCK_SIZE; j++) {
                    input_itemsets[b_index_y * BLOCK_SIZE + i + 1][b_index_x * BLOCK_SIZE +  j + 1] = input_itemsets_l[i + 1][j + 1];
                }
            }
        }
        /************* end of openMP ************/
    }
}