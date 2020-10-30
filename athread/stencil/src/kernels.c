/***************************************************************************
 *cr
 *cr            (C) Copyright 2010 The Board of Trustees of the
 *cr                        University of Illinois
 *cr                         All Rights Reserved
 *cr
 ***************************************************************************/

#include <athread.h>
#include "common.h"
#include "kernels_slave.h"

extern void slave_spe_stencil(struct spe_param *param);

void cpu_stencil(float c0,float c1, float *A0,float * Anext,int nx, int ny, int nz)
{
    // 打包传入的参数
    struct spe_param param = {
        .c0 = c0,
        .c1 = c1,
        .A0 = A0,
        .Anext = Anext,
        .nx = nx,
        .ny = ny,
        .nz = nz
    };
    
    athread_spawn(spe_stencil, &param);
    athread_join();
}


