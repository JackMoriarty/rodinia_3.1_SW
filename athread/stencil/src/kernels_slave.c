#include <slave.h>
#include <stdlib.h>
#include <simd.h>
#include "common.h"
#include "kernels_slave.h"

#define THREAD_NUM 64

float *local_A0[4];
float *local_Anext[2];

/*将y对应的维度进行划分*/
void spe_stencil(struct spe_param *param)
{
    // 解包传入的参数
    float c0 = param->c0;
    float c1 = param->c1;
    float *A0 = param->A0;
    float *Anext = param->Anext;
    int nx = param->nx;
    int ny = param->ny;
    int nz = param->nz;

    int i, j, k;
    int chunk_size = ny / THREAD_NUM;
    int chunk_remainder = ny % THREAD_NUM;

    // 确定要加载的数据边界，包含halo区
    int y_begin, y_end;
    y_begin = (_MYID < chunk_remainder) ? (chunk_size+1)*_MYID : chunk_size*_MYID + chunk_remainder;
    y_end = (_MYID < chunk_remainder) ? (chunk_size+1)*(_MYID+1) : chunk_size*(_MYID+1) + chunk_remainder;
    if (_MYID != 0)
        y_begin -= 1;
    if (_MYID != THREAD_NUM-1)
        y_end += 1;
    
    // 申请空间
    int index = 0;
    for (index = 0; index < 4; index++) {
        local_A0[index] = (float *)ldm_malloc(sizeof(float)*nx*5+2);
        if (local_A0[index] == NULL) {
            printf("ERROR: alloc local_A0 failed!!!\n");
            exit(EXIT_FAILURE);
        }
    }
    for (index = 0; index < 2; index++) {
        local_Anext[index] = (float *)ldm_malloc(sizeof(float)*nx*3+1);
        if (local_Anext[index] == NULL) {
            printf("ERROR: alloc local_Anext failed!!!\n");
            exit(EXIT_FAILURE);
        }
    }

    volatile int A0_preload_reply;
    volatile int Anext_push_reply[2] = {0, 1};
    
    // 数据准备
    int A0_load;
    int Anext_push = 0;
// printf("pos 1\n");
    floatv4 head, foot, front, back, left, right, self, sum1, mul2, result;
    floatv4 vector_c0 = simd_set_floatv4(c0, c0, c0, c0);
    floatv4 vector_c1 = simd_set_floatv4(c1, c1, c1, c1);
    for (j = y_begin+1; j < y_end-5; j+=3) {
        A0_preload_reply = 0;
        athread_get(PE_MODE, &A0[Index3D(nx, ny, 0, j-1, 0)], local_A0[0], sizeof(float)*nx*5, &A0_preload_reply, 0, 0, 0);
        athread_get(PE_MODE, &A0[Index3D(nx, ny, 0, j-1, 1)], local_A0[1], sizeof(float)*nx*5, &A0_preload_reply, 0, 0, 0);
        athread_get(PE_MODE, &A0[Index3D(nx, ny, 0, j-1, 2)], local_A0[2], sizeof(float)*nx*5, &A0_preload_reply, 0, 0, 0);
        A0_load = 3;
        Anext_push_reply[0] = 0;
        Anext_push_reply[1] = 1;
        Anext_push = 0;
        while (A0_preload_reply != 3);
// printf("pos 2\n");
        for (k = 1; k < nz-1; k++) {
            // 预取数据
            if (k+2 < nz) {
                A0_preload_reply = 0;
                athread_get(PE_MODE, &A0[Index3D(nx, ny, 0, j-1, k+2)], local_A0[A0_load], sizeof(float)*nx*5, &A0_preload_reply, 0, 0, 0);
            }
            // // 填充local_Anext位置0和位置nx-1的值
            // local_Anext[Anext_push][0] = local_A0[(A0_load+2)&0x3][nx];
            // local_Anext[Anext_push][nx-1] = local_A0[(A0_load+2)&0x3][2*nx-1];
            // local_Anext[Anext_push][nx] = local_A0[(A0_load+2)&0x3][2*nx];
            // local_Anext[Anext_push][2*nx-1] = local_A0[(A0_load+2)&0x3][3*nx-1];
            // local_Anext[Anext_push][2*nx] = local_A0[(A0_load+2)&0x3][3*nx];
            // local_Anext[Anext_push][3*nx-1] = local_A0[(A0_load+2)&0x3][4*nx-1];
            // 输入数据nx = 512, 向量化宽度为4, 因此能够整除, 同时编译器保证内存是对齐32字节,此处不做特殊处理
            for (i=1; i < nx-1; i+=4) {
                // 将y循环展开3次
                // local_Anext[Anext_push][i] =                         // Anext[k][j][i]
                //          (local_A0[(A0_load+3) & 0x3][nx+i] +        // A0[k+1][j][i]
                //           local_A0[(A0_load+1) & 0x3][nx+i] +        // A0[k-1][j][i]
                //           local_A0[(A0_load+2) & 0x3][2*nx+i] +      // A0[k][j+1][i]
                //           local_A0[(A0_load+2) & 0x3][i] +           // A0[k][j-1][i]
                //           local_A0[(A0_load+2) & 0x3][nx+i-1] +      // A0[k][j][i-1]
                //           local_A0[(A0_load+2) & 0x3][nx+i+1])*c1    // A0[k][j][i+1]
                //          - local_A0[(A0_load+2) & 0x3][nx+i]*c0;     // A0[k][j][i]
               
                // 加载数据
                simd_loadu(head, &local_A0[(A0_load+1) & 0x3][nx+i]);
                simd_loadu(self, &local_A0[(A0_load+2) & 0x3][nx+i]);
                simd_loadu(foot, &local_A0[(A0_load+3) & 0x3][nx+i]);
                simd_loadu(front, &local_A0[(A0_load+2) & 0x3][i]);
                simd_loadu(back, &local_A0[(A0_load+2) & 0x3][2*nx+i]);
                simd_loadu(left, &local_A0[(A0_load+2) & 0x3][nx+i-1]);
                simd_loadu(right, &local_A0[(A0_load+2) & 0x3][nx+i+1]);
                sum1 = head+foot+front+back+left+right;
                mul2 = self * vector_c0;
                result = simd_vmss(sum1, vector_c1, mul2);
                simd_storeu(result, &local_Anext[Anext_push][i]);

                // local_Anext[Anext_push][nx+i] =                      // Anext[k][j+1][i]
                //          (local_A0[(A0_load+3) & 0x3][2*nx+i] +      // A0[k+1][j+1][i]
                //           local_A0[(A0_load+1) & 0x3][2*nx+i] +      // A0[k-1][j+1][i]
                //           local_A0[(A0_load+2) & 0x3][3*nx+i] +      // A0[k][j+2][i]
                //           local_A0[(A0_load+2) & 0x3][nx+i] +        // A0[k][j][i]
                //           local_A0[(A0_load+2) & 0x3][2*nx+i-1] +    // A0[k][j+1][i-1]
                //           local_A0[(A0_load+2) & 0x3][2*nx+i+1])*c1  // A0[k][j+1][i+1]
                //           - local_A0[(A0_load+2) & 0x3][2*nx+i]*c0;  // A0[k][j+1][i]
                simd_loadu(head, &local_A0[(A0_load+1) & 0x3][2*nx+i]);
                simd_loadu(self, &local_A0[(A0_load+2) & 0x3][2*nx+i]);
                simd_loadu(foot, &local_A0[(A0_load+3) & 0x3][2*nx+i]);
                simd_loadu(front, &local_A0[(A0_load+2) & 0x3][nx+i]);
                simd_loadu(back, &local_A0[(A0_load+2) & 0x3][3*nx+i]);
                simd_loadu(left, &local_A0[(A0_load+2) & 0x3][2*nx+i-1]);
                simd_loadu(right, &local_A0[(A0_load+2) & 0x3][2*nx+i+1]);
                sum1 = head+foot+front+back+left+right;
                mul2 = self * vector_c0;
                result = simd_vmss(sum1, vector_c1, mul2);
                simd_storeu(result, &local_Anext[Anext_push][nx+i]);
 
                //  local_Anext[Anext_push][2*nx+i] =                    // Anext[k][j+2][i]
                //           (local_A0[(A0_load+3) & 0x3][3*nx+i] +      // A0[k+1][j+2][i]
                //            local_A0[(A0_load+1) & 0x3][3*nx+i] +      // A0[k-1][j+2][i]
                //            local_A0[(A0_load+2) & 0x3][4*nx+i] +      // A0[k][j+3][i]
                //            local_A0[(A0_load+2) & 0x3][2*nx+i] +      // A0[k][j+1][i]
                //            local_A0[(A0_load+2) & 0x3][3*nx+i-1] +    // A0[k][j+2][i-1]
                //            local_A0[(A0_load+2) & 0x3][3*nx+i+1])*c1  // A0[k][j+2][i+1]
                //            - local_A0[(A0_load+2) & 0x3][3*nx+i]*c0;  // A0[k][j+2][i]
                simd_loadu(head, &local_A0[(A0_load+1) & 0x3][3*nx+i]);
                simd_loadu(self, &local_A0[(A0_load+2) & 0x3][3*nx+i]);
                simd_loadu(foot, &local_A0[(A0_load+3) & 0x3][3*nx+i]);
                simd_loadu(front, &local_A0[(A0_load+2) & 0x3][2*nx+i]);
                simd_loadu(back, &local_A0[(A0_load+2) & 0x3][4*nx+i]);
                simd_loadu(left, &local_A0[(A0_load+2) & 0x3][3*nx+i-1]);
                simd_loadu(right, &local_A0[(A0_load+2) & 0x3][3*nx+i+1]);
                sum1 = head+foot+front+back+left+right;
                mul2 = self * vector_c0;
                result = simd_vmss(sum1, vector_c1, mul2);
                simd_storeu(result, &local_Anext[Anext_push][2*nx+i]);
 
            }
            // 填充local_Anext位置0和位置nx-1的值
            local_Anext[Anext_push][0] = local_A0[(A0_load+2)&0x3][nx];
            local_Anext[Anext_push][nx-1] = local_A0[(A0_load+2)&0x3][2*nx-1];
            local_Anext[Anext_push][nx] = local_A0[(A0_load+2)&0x3][2*nx];
            local_Anext[Anext_push][2*nx-1] = local_A0[(A0_load+2)&0x3][3*nx-1];
            local_Anext[Anext_push][2*nx] = local_A0[(A0_load+2)&0x3][3*nx];
            local_Anext[Anext_push][3*nx-1] = local_A0[(A0_load+2)&0x3][4*nx-1];

            // 将计算结果写回主存
            Anext_push_reply[Anext_push] = 0;
            athread_put(PE_MODE, local_Anext[Anext_push], &Anext[Index3D(nx, ny, 0, j, k)], sizeof(float)*nx*3, &Anext_push_reply[Anext_push], 0, 0);
            Anext_push = 1 - Anext_push;
            // 等待数据预取完毕
            while (A0_preload_reply != 1);
// printf("pos 3\n");
            A0_load = (A0_load+1) & 0x3;
            // 等待前一次结果写回完毕
            while (Anext_push_reply[Anext_push] != 1);
// printf("pos 4\n");
        }
        // 等待结果写回完毕
        while(Anext_push_reply[1-Anext_push] != 1);
// printf("pos 5\n");
    }

    // 检测是否有剩余, 如有则进行处理
//    j = y_begin + 1;
    if (j < y_end - 1) {
        for (; j < y_end-1; j++) {
            A0_preload_reply = 0;
            athread_get(PE_MODE, &A0[Index3D(nx, ny, 0, j-1, 0)], local_A0[0], sizeof(float)*nx*3, &A0_preload_reply, 0, 0, 0);
            athread_get(PE_MODE, &A0[Index3D(nx, ny, 0, j-1, 1)], local_A0[1], sizeof(float)*nx*3, &A0_preload_reply, 0, 0, 0);
            athread_get(PE_MODE, &A0[Index3D(nx, ny, 0, j-1, 2)], local_A0[2], sizeof(float)*nx*3, &A0_preload_reply, 0, 0, 0);
            A0_load = 3;
            Anext_push_reply[0] = 0;
            Anext_push_reply[1] = 1;
            Anext_push = 0;
            while (A0_preload_reply != 3);
            for (k = 1; k < nz-1; k++) {
                // 预取数据
                if (k+2 < nz) {
                    A0_preload_reply = 0;
                    athread_get(PE_MODE, &A0[Index3D(nx, ny, 0, j-1, k+2)], local_A0[A0_load], sizeof(float)*nx*3, &A0_preload_reply, 0, 0, 0);
                }
                // // 填充local_Anext位置0和位置nx-1的值
                // local_Anext[Anext_push][0] = local_A0[(A0_load+2)&0x3][nx];
                // local_Anext[Anext_push][nx-1] = local_A0[(A0_load+2)&0x3][2*nx-1];
                for (i=1; i < nx-1; i+=4) {
                   // local_Anext[Anext_push][i] =                         // Anext[k][j][i]
                   //          (local_A0[(A0_load+3) & 0x3][nx+i] +        // A0[k+1][j][i]
                   //           local_A0[(A0_load+1) & 0x3][nx+i] +        // A0[k-1][j][i]
                   //           local_A0[(A0_load+2) & 0x3][2*nx+i] +      // A0[k][j+1][i]
                   //           local_A0[(A0_load+2) & 0x3][i] +           // A0[k][j-1][i]
                   //           local_A0[(A0_load+2) & 0x3][nx+i-1] +      // A0[k][j][i-1]
                   //           local_A0[(A0_load+2) & 0x3][nx+i+1])*c1    // A0[k][j][i+1]
                   //          - local_A0[(A0_load+2) & 0x3][nx+i]*c0;     // A0[k][j][i]
                 
                    // 加载数据
                    simd_loadu(head, &local_A0[(A0_load+1) & 0x3][nx+i]);
                    simd_loadu(self, &local_A0[(A0_load+2) & 0x3][nx+i]);
                    simd_loadu(foot, &local_A0[(A0_load+3) & 0x3][nx+i]);
                    simd_loadu(front, &local_A0[(A0_load+2) & 0x3][i]);
                    simd_loadu(back, &local_A0[(A0_load+2) & 0x3][2*nx+i]);
                    simd_loadu(left, &local_A0[(A0_load+2) & 0x3][nx+i-1]);
                    simd_loadu(right, &local_A0[(A0_load+2) & 0x3][nx+i+1]);
                    sum1 = head+foot+front+back+left+right;
                    mul2 = self * vector_c0;
                    result = simd_vmss(sum1, vector_c1, mul2);
                    simd_storeu(result, &local_Anext[Anext_push][i]);
                }
                // 填充local_Anext位置0和位置nx-1的值
                local_Anext[Anext_push][0] = local_A0[(A0_load+2)&0x3][nx];
                local_Anext[Anext_push][nx-1] = local_A0[(A0_load+2)&0x3][2*nx-1];

               // 将计算结果写回主存
                Anext_push_reply[Anext_push] = 0;
                athread_put(PE_MODE, local_Anext[Anext_push], &Anext[Index3D(nx, ny, 0, j, k)], sizeof(float)*nx, &Anext_push_reply[Anext_push], 0, 0);
                Anext_push = 1 - Anext_push;
                // 等待数据预取完毕
                while (A0_preload_reply != 1);
                A0_load = (A0_load+1) & 0x3;
                // 等待前一次结果写回完毕
                while (Anext_push_reply[Anext_push] != 1);
            }
            // 等待结果写回完毕
            while(Anext_push_reply[1-Anext_push] != 1);
        }
    }

    // 释放申请的空间
    for (index = 0; index < 4; index++)
        ldm_free(local_A0[index], sizeof(float)*nx*5+2);
    for (index = 0; index < 2; index++)
        ldm_free(local_Anext[index], sizeof(float)*nx*3+1);
}
