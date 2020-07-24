/**
 * @file athread_spe.c
 * @author chenbangduo (chenbangduo@qq.com)
 * @brief hotspot3D, 数据重用及通信隐藏
 * @version 0.1
 * @date 2020-07-24
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include <slave.h>
#include "common.h"

#define MAX_THREADS 64
#define NUMCOLS 512
#define NUMROWS 512
#define LAYERS 8
#define SIZE (NUMROWS * NUMCOLS * LAYERS)

__thread_local float local_tIn_n_c_s[3][NUMCOLS];
__thread_local float local_tIn_b[NUMCOLS];
__thread_local float local_tIn_t[NUMCOLS];
__thread_local float local_tOut[NUMCOLS];
__thread_local float local_pIn[NUMCOLS];
// 预加载数组
__thread_local float local_tIn_s_2[NUMCOLS];
__thread_local float local_tIn_b_2[NUMCOLS];
__thread_local float local_tIn_t_2[NUMCOLS];
__thread_local float local_tOut_2[NUMCOLS];
__thread_local float local_pIn_2[NUMCOLS];

extern float amb_temp;

void spe_func(struct spe_parameter *spe_param)
{
    int x, y, z;
    volatile int DMA_reply, DMA_preload, DMA_push;
    float *tIn_t;
    float *tOut_t;
    float *pIn;
    int nx, ny, nz;
    float ce, cw, cn, cs, ct, cb, cc, stepDivCap, local_amb_temp;

    tIn_t = spe_param->tIn_t;
    tOut_t = spe_param->tOut_t;
    pIn = spe_param->pIn;
    nx = spe_param->nx;
    ny = spe_param->ny;
    nz = spe_param->nz;
    ce = spe_param->ce;
    cw = spe_param->cw;
    cn = spe_param->cn;
    cs = spe_param->cs;
    ct = spe_param->ct;
    cb = spe_param->cb;
    cc = spe_param->cc;
    stepDivCap = spe_param->stepDivCap;
    local_amb_temp = amb_temp;

    int chunksize = nz / MAX_THREADS;
    int odd = nz % MAX_THREADS;
    int left, right;
    if (_MYID < odd) {
        left = (chunksize + 1) * _MYID;
        right = left + (chunksize + 1);
    } else {
        left = (chunksize + 1) * odd + (_MYID - odd) * chunksize;
        right = (_MYID == MAX_THREADS - 1) ? nz : left + chunksize;
    }

    for (z = left; z < right; z++) {
        // 预加载数据
        int center = z * nx * ny; // center = y * nx + z * nx * ny, y = 0
        int wait_num = 2;
        DMA_reply = 0;
        athread_get(PE_MODE, &pIn[center], local_pIn, sizeof(float) * NUMCOLS, &DMA_reply, 0, 0, 0);

        athread_get(PE_MODE, &tIn_t[center], &local_tIn_n_c_s[1], sizeof(float) * NUMCOLS * 2, &DMA_reply, 0, 0 ,0);

        if (z > 0) {
            // b
            athread_get(PE_MODE, &tIn_t[center - ny * nx], local_tIn_b, sizeof(float) * NUMCOLS, &DMA_reply, 0, 0, 0);
            wait_num++;
        }
        if (z < nz - 1) {
            // t
            athread_get(PE_MODE, &tIn_t[center + ny * nx], local_tIn_t, sizeof(float) * NUMCOLS, &DMA_reply, 0, 0, 0);
            wait_num++;
        }
        while (DMA_reply != wait_num);

        // 采用指针访问
        float *tIn_n_ptr = local_tIn_n_c_s[0];
        float *tIn_c_ptr = local_tIn_n_c_s[1];
        float *tIn_s_ptr = local_tIn_n_c_s[2];
        float *tIn_b_ptr = local_tIn_b;
        float *tIn_t_ptr = local_tIn_t;
        float *pIn_ptr = local_pIn;
        float *tOut_ptr = local_tOut;
        float *tIn_s_preload_ptr = local_tIn_s_2;
        float *tIn_b_preload_ptr = local_tIn_b_2;
        float *tIn_t_preload_ptr = local_tIn_t_2;
        float *pIn_preload_ptr = local_pIn_2;
        float *tOut_push_ptr = local_tOut_2;

        DMA_push = 1;
        for (y = 0; y < ny; y++) {
            int center = y * nx + z * nx * ny;
            // 预取下一阶段数据
            DMA_preload = 0;
            wait_num = 0;
            if (y + 2 < ny) {
                int preload_south = (y + 2) * nx + z * nx * ny;
                athread_get(PE_MODE, &tIn_t[preload_south], tIn_s_preload_ptr, sizeof(float) * NUMCOLS, &DMA_preload, 0, 0, 0);
                wait_num++;
            }
            if (y + 1 < ny) {
                int preload_center = (y + 1) * nx + z * nx * ny;
                athread_get(PE_MODE, &pIn[preload_center], pIn_preload_ptr, sizeof(float) * NUMCOLS, &DMA_preload, 0, 0, 0);
                wait_num++;

                if (z > 0) { // preload b
                    athread_get(PE_MODE, &tIn_t[preload_center - ny * nx], tIn_b_preload_ptr, sizeof(float) * NUMCOLS, &DMA_preload, 0, 0, 0);
                    wait_num++;
                }
                if (z < nz - 1) { // preload t
                    athread_get(PE_MODE, &tIn_t[preload_center + ny * nx], tIn_t_preload_ptr, sizeof(float) * NUMCOLS, &DMA_preload, 0, 0, 0);
                    wait_num++;
                }
            }
            for (x = 0; x < nx; x++) {

                int w, e;
                float *n_select, *s_select, *b_select, *t_select;
                w = (x == 0) ? x : x - 1;
                e = (x == nx - 1) ? x : x + 1;
                n_select = (y == 0) ? tIn_c_ptr : tIn_n_ptr;
                s_select = (y == ny - 1) ? tIn_c_ptr : tIn_s_ptr;
                b_select = (z == 0) ? tIn_c_ptr : tIn_b_ptr;
                t_select = (z == nz - 1) ? tIn_c_ptr : tIn_t_ptr;
                tOut_ptr[x] = cc * tIn_c_ptr[x] + cw * tIn_c_ptr[w] + ce * tIn_c_ptr[e] \
                        + cs * s_select[x] + cn * n_select[x] + cb * b_select[x] \
                        + ct * t_select[x]+stepDivCap * pIn_ptr[x] + ct*local_amb_temp;
            }
            
            float * tmp;
            // 等待下一周期数据预取完成, 数据指针交换
            // tIn_{n,c,s}
            tmp = tIn_n_ptr;
            tIn_n_ptr = tIn_c_ptr;
            tIn_c_ptr = tIn_s_ptr;
            tIn_s_ptr = tIn_s_preload_ptr;
            tIn_s_preload_ptr = tmp;
            // tIn_b
            tmp = tIn_b_ptr;
            tIn_b_ptr = tIn_b_preload_ptr;
            tIn_b_preload_ptr = tmp;
            // tIn_t
            tmp = tIn_t_ptr;
            tIn_t_ptr = tIn_t_preload_ptr;
            tIn_t_preload_ptr = tmp;
            // pIn
            tmp = pIn_ptr;
            pIn_ptr = pIn_preload_ptr;
            pIn_preload_ptr = tmp;
            while (DMA_preload != wait_num);

            // 等待上一周期数据刷回
            // tOut
            tmp = tOut_ptr;
            tOut_ptr = tOut_push_ptr;
            tOut_push_ptr = tmp;
            while(DMA_push != 1);
            DMA_push = 0;
            athread_put(PE_MODE, tOut_push_ptr, &tOut_t[center], sizeof(float) * NUMCOLS, &DMA_push, 0, 0);
        }
        while(DMA_push != 1);
    }
}
