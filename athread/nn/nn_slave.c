#include <slave.h>
#include <stdlib.h>
#include <math.h>
#include "common.h"

#define REC_LENGTH 48	// size of a record in db
#define REC_WINDOW 2048	// number of records to read at a time
#define LATITUDE_POS 27	// location of latitude coordinates in input record

#define THREAD_NUM 64

__thread_local float local_z[REC_WINDOW / THREAD_NUM];
__thread_local char local_sandbox[REC_WINDOW / THREAD_NUM][REC_LENGTH];
void nn_slave(struct spe_parameter *spe_param)
{
    int local_rec_count = spe_param->rec_count;
    char (*mpe_sandbox)[REC_LENGTH] = (char (*)[REC_LENGTH])(spe_param->sandbox);
    float *mpe_z = spe_param->z;
    float target_lat = spe_param->target_lat;
    float target_long = spe_param->target_long;

    int chunksize = local_rec_count / THREAD_NUM;

    int i_left = _MYID * chunksize;
    int i_right = (_MYID == THREAD_NUM - 1) ? local_rec_count : (_MYID + 1) * chunksize;

    int i_size = i_right - i_left;

    if (i_size == 0) return;

    volatile int DMA_reply;
    DMA_reply = 0;

    athread_get(PE_MODE, mpe_sandbox[i_left], local_sandbox, \
        sizeof(char [REC_LENGTH]) * i_size, &DMA_reply, 0, 0, 0);
    while (DMA_reply != 1);
    int i;
    char *rec_iter;
    for (i = 0; i < i_size; i++) {
        rec_iter = &local_sandbox[i][LATITUDE_POS - 1];
        // float tmp_lat = atof(rec_iter);
        // float tmp_long = atof(rec_iter + 5);
        float tmp_lat, tmp_long;
        sscanf(rec_iter, "%f %f", &tmp_lat, &tmp_long);
        local_z[i] = sqrt(( (tmp_lat-target_lat) * (tmp_lat-target_lat) )+( (tmp_long-target_long) * (tmp_long-target_long) ));
    }

    DMA_reply = 0;
    athread_put(PE_MODE, local_z, &mpe_z[i_left], sizeof(float) * i_size, &DMA_reply, 0, 0);
    while (DMA_reply != 1);
}
