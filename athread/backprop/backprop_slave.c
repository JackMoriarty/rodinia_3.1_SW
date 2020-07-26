#include <slave.h>
#include <math.h>
#include "backprop.h"
#include "common.h"

#define THREAD_NUM 64
#define BUFF_SIZE 1024
__thread_local float local_l1[BUFF_SIZE];
__thread_local float local_ly[BUFF_SIZE];

float slave_squash(float x)
{
  return (1.0 / (1.0 + exp(-x)));
}

void spe_func1(struct spe_func1_parameter *spe_param)
{
    int j, k;
    float *sum, *local_l2, *local_conn_k;
    volatile int DMA_reply = 0;
    // 该程序n1的取值为65536或16, n2的取值为16或1, 故l2对应的维度可以直接
    // 传输到局存, 而l1则需要分块传输
    int n1 = spe_param->n1;
    int n2 = spe_param->n2;
    float *l1 = spe_param->l1;
    float *l2 = spe_param->l2;
    float **conn = spe_param->conn;

    sum = (float *)ldm_malloc(sizeof(float) * (n2 + 1));
    local_l2 = (float *)ldm_malloc(sizeof(float) * (n2 + 1));
    local_conn_k = (float *)ldm_malloc(sizeof(float) * (n2 + 1));
    for (j = 1; j <= n2; j++)
        sum[j] = 0;

    int chunksize = n2 / THREAD_NUM;
    int chunkodd = n2 % THREAD_NUM;

    // 将剩余的部分给各线程的部分均匀分给前面的线程
    int chunk_left = ((_MYID < chunkodd) ? (chunksize + 1) * _MYID : (chunkodd + chunksize * _MYID)) + 1;
    int chunk_right;
    if (_MYID < chunkodd)
        chunk_right = (chunksize + 1) * (_MYID + 1) + 1;
    else if (_MYID != THREAD_NUM - 1)
        chunk_right = chunksize * (_MYID + 1) + chunkodd + 1;
    else 
        chunk_right = n2 + 1;

    for (k = 0; k <= n1; k++) {
        DMA_reply = 0;
        int wait_num = 1;
        int local_l1_start_k;
        if (k % BUFF_SIZE == 0) {
            int DMA_size = (n1 - k + 1 < BUFF_SIZE) ? n1 - k + 1: BUFF_SIZE;
            athread_get(PE_MODE, &l1[k], local_l1, sizeof(float) * DMA_size, &DMA_reply, 0, 0, 0);
            local_l1_start_k = k;
            wait_num++;
        }
        athread_get(PE_MODE, conn[k], local_conn_k, sizeof(float) * (n2 + 1), &DMA_reply, 0, 0, 0);
        while (DMA_reply != wait_num);
        for (j = chunk_left; j < chunk_right; j++) {
            sum[j] += local_conn_k[j] * local_l1[k - local_l1_start_k];
        }
    }

    for (j = chunk_left; j < chunk_right; j++) {
        local_l2[j] = slave_squash(sum[j]);
    }

    if (chunk_right > chunk_left) {
        DMA_reply = 0;
        athread_put(PE_MODE, &local_l2[chunk_left], &l2[chunk_left], \
            sizeof(float) * (chunk_right - chunk_left), & DMA_reply, 0, 0);
        while (DMA_reply != 1);
    }

    ldm_free(sum, sizeof(float ) * (n2 + 1));
    ldm_free(local_l2, sizeof(float ) * (n2 + 1));
    ldm_free(local_conn_k, sizeof(float ) * (n2 + 1));
}

void spe_func2(struct spe_func2_parameter *spe_param)
{
    float new_dw;    
    int k, j;
    volatile int DMA_reply = 0;
    float *local_delta, *local_oldw_k, *local_w_k;

    int ndelta = spe_param->ndelta;
    int nly = spe_param->nly;
    float *delta = spe_param->delta;
    float *ly = spe_param->ly;
    float **w = spe_param->w;
    float **oldw = spe_param->oldw;

    // ndelta 取值为1或16, nly取值为16或65536, 考虑循环特点, 将划分nly而不是ndelta
    
    int chunksize = (nly + 1) / THREAD_NUM;
    int chunkodd = (nly + 1) % THREAD_NUM;

    // 将剩余的部分给各线程的部分均匀分给前面的线程
    int chunk_left = (_MYID < chunkodd) ? (chunksize + 1) * _MYID : (chunkodd + chunksize * _MYID);
    int chunk_right;
    if (_MYID < chunkodd)
        chunk_right = (chunksize + 1) * (_MYID + 1);
    else if (_MYID != THREAD_NUM - 1)
        chunk_right = chunksize * (_MYID + 1) + chunkodd;
    else 
        chunk_right = nly + 1;

    local_delta = (float *)ldm_malloc(sizeof(float) * (ndelta + 1));
    local_oldw_k = (float *)ldm_malloc(sizeof(float) * (ndelta + 1));
    local_w_k = (float *)ldm_malloc(sizeof(float) * (ndelta + 1));
    
    DMA_reply = 0;
    athread_get(PE_MODE, delta, local_delta, sizeof(float) * (ndelta + 1), &DMA_reply, 0, 0, 0);
    while (DMA_reply != 1);

    int prev_k;
    for (k = chunk_left; k < chunk_right; k++) {
        DMA_reply = 0;
        int DMA_wait = 2;
        if ((k - chunk_left) % BUFF_SIZE == 0) {
            prev_k = k;
            int DMA_size = (chunk_right - k < BUFF_SIZE) ? (chunk_right - k) : BUFF_SIZE;
            athread_get(PE_MODE, &ly[k], local_ly, sizeof(float) * DMA_size, &DMA_reply, 0, 0, 0);
            DMA_wait++;
        }
        athread_get(PE_MODE, oldw[k], local_oldw_k, sizeof(float) * (ndelta + 1), &DMA_reply, 0, 0, 0);
        athread_get(PE_MODE, w[k], local_w_k, sizeof(float) * (ndelta + 1), &DMA_reply, 0, 0, 0);
        while (DMA_reply != DMA_wait);

        for (j = 1; j <= ndelta; j++) {
            new_dw = ((ETA * local_delta[j] * local_ly[k - prev_k]) + (MOMENTUM * local_oldw_k[j]));
            local_w_k[j] += new_dw;
            local_oldw_k[j] = new_dw;
        }

        DMA_reply = 0;
        athread_put(PE_MODE, local_w_k, w[k], sizeof(float) * (ndelta + 1), &DMA_reply, 0, 0);
        athread_put(PE_MODE, local_oldw_k, oldw[k], sizeof(float) * (ndelta + 1), &DMA_reply, 0, 0);
        while (DMA_reply != 2);
    }

    ldm_free(local_delta, sizeof(float) * (ndelta + 1));
    ldm_free(local_oldw_k, sizeof(float) * (ndelta + 1));
    ldm_free(local_w_k, sizeof(float) * (ndelta + 1));
}