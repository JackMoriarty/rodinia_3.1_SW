#include <slave.h>
#include <simd.h>
#include <stdio.h>
#include <stdlib.h>

#define COL(x) ((x) & 0x07) 
#define ROW(x) (((x) & 0x3F ) >> 3)
#define reg_putr(dst, var) __asm__ __volatile__ ("putr %0, %1\n"::"r"(var), "r"(dst):"memory")
#define reg_putc(dst, var) __asm__ __volatile__ ("putc %0, %1\n"::"r"(var), "r"(dst):"memory")
#define reg_getr(var) __asm__ __volatile__ ("getr %0\n":"=r"(var)::"memory")
#define reg_getc(var) __asm__ __volatile__ ("getc %0\n":"=r"(var)::"memory")

#define NUM_OBJECTS 494020
#define NUM_ATTRIBUTES 34
#define NCLUSTERS 5
#define NTHREADS 64
#define FLT_MAX 3.40282347e+38
#define NFEATURES NUM_ATTRIBUTES
#define NPOINTS NUM_OBJECTS

extern float attributes[NUM_OBJECTS][NUM_ATTRIBUTES];
extern float clusters[NCLUSTERS][NFEATURES];
extern int membership[NUM_OBJECTS];
extern int new_centers_len[NCLUSTERS];
extern float new_centers[NCLUSTERS][NFEATURES];
extern int partial_new_centers_len[NTHREADS][NCLUSTERS];
extern float partial_new_centers[NTHREADS][NCLUSTERS][NFEATURES];
extern float delta;

__thread_local float local_attributes[NUM_ATTRIBUTES];
__thread_local int local_membership[NUM_OBJECTS / NTHREADS + NUM_OBJECTS % NTHREADS];
__thread_local int local_partial_new_centers_len[NCLUSTERS];
__thread_local float local_partial_new_centers[NCLUSTERS][NFEATURES];
__thread_local float local_delta;
__thread_local float local_clusters[NCLUSTERS][NFEATURES];

float euclid_dist_2(float *pt1, float *pt2);
int find_nearest_point(float *pt);

int find_nearest_point(float *pt)
{
    int index, i;
    float min_dist = FLT_MAX;

    for (i = 0; i < NCLUSTERS; i++) {
        float dist;
        dist = euclid_dist_2(pt, local_clusters[i]);
        if (dist < min_dist) {
            min_dist = dist;
            index = i;
        }
    }

    return index;
}

float euclid_dist_2(float *pt1, float *pt2)
{
    int i;
    float ans = 0;

    for (i = 0; i < NFEATURES; i++)
        ans += (pt1[i] - pt2[i]) * (pt1[i] - pt2[i]);
    
    return ans;
}

void spe_func()
{
    int tid = _MYID;
    int index;
    int i, j;
    volatile int DMA_reply;

    int chunk = NPOINTS / NTHREADS;
    int low = tid * chunk;
    int high = (tid == NTHREADS - 1) ? NPOINTS : (tid + 1) * chunk;
    int block_size = high - low;
    
    DMA_reply = 0;
    athread_get(PE_MODE, &membership[low], local_membership, \
                sizeof(int) * block_size, &DMA_reply, 0, 0, 0);
    athread_get(PE_MODE, partial_new_centers_len[tid], \
                local_partial_new_centers_len, sizeof(float) * NCLUSTERS, \
                &DMA_reply, 0, 0, 0);
    athread_get(PE_MODE, partial_new_centers[tid], \
                local_partial_new_centers, sizeof(float) * NCLUSTERS * NFEATURES, \
                &DMA_reply, 0, 0, 0);
    athread_get(PE_MODE, clusters, local_clusters, sizeof(float) * NCLUSTERS * NFEATURES, \
                &DMA_reply, 0, 0, 0);

    local_delta = 0;
    while (DMA_reply != 4);
    

    for (i = 0; i < block_size; i++) {
        
        DMA_reply = 0;
        athread_get(PE_MODE, attributes[low + i], local_attributes, \
                    sizeof(float) * NUM_ATTRIBUTES, &DMA_reply, 0, 0, 0);
        while (DMA_reply != 1);
        
        index = find_nearest_point(local_attributes);
        
        if (local_membership[i] != index)
            local_delta += 1.0;
        local_membership[i] = index;

        local_partial_new_centers_len[index]++;

        for (j = 0; j < NFEATURES; j++)
            local_partial_new_centers[index][j] += local_attributes[j];
    }
    
    DMA_reply = 0;
    athread_put(PE_MODE, local_membership, &membership[low], \
                sizeof(int) * block_size, &DMA_reply, 0, 0);
    athread_put(PE_MODE, local_partial_new_centers_len, partial_new_centers_len[tid],\
                sizeof(float) * NCLUSTERS, &DMA_reply, 0, 0);
    athread_put(PE_MODE, local_partial_new_centers, partial_new_centers[tid],\
                sizeof(float) * NCLUSTERS * NFEATURES, &DMA_reply, 0, 0);
    
    int256 reduction_val;
    int256 comm_val;
    float *reduction_ptr = (float *)&reduction_val;
    float *comm_ptr = (float *)&comm_val;
    reduction_ptr[0] = local_delta;
    if (COL(_MYID) == 0) {
        for (i = 0; i < 7; i++) {
            reg_getr(comm_val);
            reduction_ptr[0] += comm_ptr[0];
        }

        if (_MYID == 0) {
            for (i = 0; i < 7; i++) {
                reg_getc(comm_val);
                reduction_ptr[0] += comm_ptr[0];
            }
            delta = reduction_ptr[0];
        } else {
            reg_putc(0, reduction_val);
        }
    } else {
        reg_putr(0, reduction_val);
    }
    
    while (DMA_reply != 3);
}
