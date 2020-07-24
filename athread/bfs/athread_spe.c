#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <slave.h>

#define NO_OF_NODES 1000000
#define EDGE_LIST_SIZE 5999970
#define MAX_THREADS 64
// #define LOCAL_SIZE (NO_OF_NODES / MAX_THREADS + NO_OF_NODES % MAX_THREADS)
#define DMA_SIZE 2048
#define LOCAL_SIZE DMA_SIZE
#define ALLSYN  athread_syn(ARRAY_SCOPE,0xffff)

struct Node {
    int starting;
    int no_of_edges;
};

extern struct Node h_graph_nodes[NO_OF_NODES];
extern unsigned char h_graph_mask[NO_OF_NODES];
extern unsigned char h_updating_graph_mask[NO_OF_NODES];
extern unsigned char h_graph_visited[NO_OF_NODES];
extern int h_cost[NO_OF_NODES];
extern int h_graph_edges[EDGE_LIST_SIZE];
extern unsigned char stop;

__thread_local struct Node local_h_graph_nodes[2][LOCAL_SIZE];
__thread_local unsigned char local_h_graph_mask[2][LOCAL_SIZE];
__thread_local unsigned char local_h_updating_graph_mask[2][LOCAL_SIZE];
__thread_local unsigned char local_h_graph_visited[2][LOCAL_SIZE];

void spe_func_1()
{
    int tid = _MYID;
    int low, high, chunk, size;
    volatile int DMA_reply[2];
    int i, j, k;
    int DMA_size[2];
    int index, dma_index[2];
    struct Node * local_h_graph_nodes_ptr;
    unsigned char * local_h_graph_mask_ptr;
    unsigned char * local_h_updating_graph_mask_ptr;
    unsigned char * local_h_graph_visited_ptr;

    chunk = ((NO_OF_NODES / MAX_THREADS) / 4 + 1) * 4;
    low = tid * chunk;
    high = (tid == MAX_THREADS - 1) ? NO_OF_NODES : (tid + 1) * chunk;
    size = high - low;

    DMA_size[0] = (size > DMA_SIZE) ? DMA_SIZE : size;

    /** 加载第一个循环所需要的数据 */
    DMA_reply[0] = 0;
    DMA_reply[1] = 1;
    athread_get(PE_MODE, &h_graph_mask[low], local_h_graph_mask[0], \
                        sizeof(unsigned char) * DMA_size[0], &DMA_reply[0], 0, 0, 0);
    athread_get(PE_MODE, &h_graph_nodes[low], local_h_graph_nodes[0], \
                        sizeof(struct Node) * DMA_size[0], &DMA_reply[0], 0, 0, 0);
    dma_index[0] = low;
    local_h_graph_mask_ptr = local_h_graph_mask[0];
    local_h_graph_nodes_ptr = local_h_graph_nodes[0];

    while (DMA_reply[0] != 2);
    k = 0;
    for (i = 0; i < size; i++) {
        if (i % DMA_SIZE == 0) {
            /* 等待上一轮数据刷回完毕 */
            while (DMA_reply[1 - k] != 1);
            /* 提前加载下一轮循环所需要的数据 */
            DMA_size[1 - k] = (high - (dma_index[k] + DMA_size[k]) > DMA_SIZE) ? DMA_SIZE : (high - (dma_index[k] + DMA_size[k]));
            dma_index[1 - k] = dma_index[k] + DMA_size[k];
            DMA_reply[1 - k] = 0;
            if (DMA_size[1 - k] != 0) {
                athread_get(PE_MODE, &h_graph_mask[dma_index[1 - k]], local_h_graph_mask[1 - k], \
                            sizeof(unsigned char) * DMA_size[1 - k], &DMA_reply[1 - k], 0, 0, 0);
                athread_get(PE_MODE, &h_graph_nodes[dma_index[1 - k]], local_h_graph_nodes[1 - k], \
                            sizeof(struct Node) * DMA_size[1 - k], &DMA_reply[1 - k], 0, 0, 0);
            }
        }

        index = i % DMA_SIZE;

        if (local_h_graph_mask_ptr[index] != 0) {
            local_h_graph_mask_ptr[index] = 0;
            for (j = local_h_graph_nodes_ptr[index].starting; j < \
                    (local_h_graph_nodes_ptr[index].no_of_edges + \
                        local_h_graph_nodes_ptr[index].starting); j++) {
                int id = h_graph_edges[j];
                if (!h_graph_visited[id]) {
                    h_cost[id] = h_cost[i + low] + 1;
                    h_updating_graph_mask[id] = 1;
                }
            }
        }

        if (((i + 1) % DMA_SIZE == 0) || i + 1 == size) {
            // 刷回本轮数据
            DMA_reply[k] = 0;
            athread_put(PE_MODE, local_h_graph_mask[k], &h_graph_mask[dma_index[k]], \
                    sizeof(unsigned char) * DMA_size[k], &DMA_reply[k], 0, 0);

            // 等待提前预取的数据完成
            if (DMA_size[1 - k] != 0)
                while (DMA_reply[1 - k] != 2);
            
            local_h_graph_mask_ptr = local_h_graph_mask[1 - k];
            local_h_graph_nodes_ptr = local_h_graph_nodes[1 - k];
            k = 1 - k;
        }
    }
    while (DMA_reply[1 - k] == 0);
}

// void spe_func_2()
// {
//     int tid = _MYID;
//     int low, high, chunk, size;
//     volatile int DMA_reply[2];
//     int i, j, k;
//     int DMA_size[2];
//     int index, dma_index[2];
//     int local_stop = 0;

//     unsigned char * local_h_graph_mask_ptr;
//     unsigned char * local_h_updating_graph_mask_ptr;
//     unsigned char * local_h_graph_visited_ptr;

//     chunk = ((NO_OF_NODES / MAX_THREADS) / 4 + 1) * 4;
//     low = tid * chunk;
//     high = (tid == MAX_THREADS - 1) ? NO_OF_NODES : (tid + 1) * chunk;
//     size = high - low; 

//     DMA_size[0] = (size > DMA_SIZE) ? DMA_SIZE : size;

//     DMA_reply[0] = 0;
//     DMA_reply[1] = 3;

//     DMA_size[0] = (size > DMA_SIZE) ? DMA_SIZE : size;
//     athread_get(PE_MODE, &h_graph_mask[low], local_h_graph_mask[0], \
//                 sizeof(unsigned char) * DMA_size[0], &DMA_reply[0], 0, 0, 0);
//     athread_get(PE_MODE, &h_updating_graph_mask[low], local_h_updating_graph_mask[0],\
//                 sizeof(unsigned char) * DMA_size[0], &DMA_reply[0], 0, 0, 0);
//     athread_get(PE_MODE, &h_graph_visited[low], local_h_graph_visited[0], \
//                 sizeof(unsigned char) * DMA_size[0], &DMA_reply[0], 0, 0, 0);
//     dma_index[0] = low;
//     local_h_graph_mask_ptr = local_h_graph_mask[0];
//     local_h_updating_graph_mask_ptr = local_h_updating_graph_mask[0];
//     local_h_graph_visited_ptr = local_h_graph_visited[0];

//     while (DMA_reply[0] != 3);
//     k = 0;
//     for (i = 0; i < size; i++) {
//         if (i % DMA_SIZE == 0) {
//             // 等待上一轮数据刷回完毕
//             while (DMA_reply[1 - k] != 3);
//             // 提前加载下一轮所需要的数据
//             DMA_size[1 - k] = (high - (dma_index[k] + DMA_size[k]) > DMA_SIZE) ? DMA_SIZE : (high - (dma_index[k] + DMA_size[k]));
//             dma_index[1 - k] = dma_index[k] + DMA_size[k];
//             DMA_reply[1 - k] = 0;
//             if (DMA_size[1 - k] != 0) {
//                 athread_get(PE_MODE, &h_graph_mask[dma_index[1 - k]], local_h_graph_mask[1 - k], \
//                             sizeof(unsigned char) * DMA_size[1 - k], &DMA_reply[1 - k], 0, 0, 0);
//                 athread_get(PE_MODE, &h_updating_graph_mask[dma_index[1 - k]], local_h_updating_graph_mask[1 - k],\
//                             sizeof(unsigned char) * DMA_size[1 - k], &DMA_reply[1 - k], 0, 0, 0);
//                 athread_get(PE_MODE, &h_graph_visited[dma_index[1 - k]], local_h_graph_visited[1 - k], \
//                             sizeof(unsigned char) * DMA_size[1 - k], &DMA_reply[1 - k], 0, 0, 0);
//             }
//         }
        
//         index = i % DMA_SIZE;

//         if (local_h_updating_graph_mask_ptr[index] != 0) {
//             local_h_graph_mask_ptr[index] = 1;
//             local_h_graph_visited_ptr[index] = 1;
//             local_stop = 1;
//             local_h_updating_graph_mask_ptr[index] = 0;
//         }

//         if (((i + 1) % DMA_SIZE == 0) || i + 1 == size) {
//             // 刷回本轮数据
//             DMA_reply[k] = 0;
//             athread_put(PE_MODE, local_h_graph_mask[k], &h_graph_mask[dma_index[k]], \
//                         sizeof(unsigned char) * DMA_size[k], &DMA_reply[k], 0, 0);
//             athread_put(PE_MODE, local_h_updating_graph_mask[k], &h_updating_graph_mask[dma_index[k]], \
//                         sizeof(unsigned char) * DMA_size[k], &DMA_reply[k], 0, 0);
//             athread_put(PE_MODE, local_h_graph_visited[k], &h_graph_visited[dma_index[k]], \
//                         sizeof(unsigned char) * DMA_size[k], &DMA_reply[k], 0, 0);
//             // 等待提前语句的数据完成
//             if (DMA_size[1 - k] != 0)
//                 while (DMA_reply[1 - k] != 3);

//             local_h_graph_mask_ptr = local_h_graph_mask[0];
//             local_h_updating_graph_mask_ptr = local_h_updating_graph_mask[0];
//             local_h_graph_visited_ptr = local_h_graph_visited[0];
//             k = 1 - k;
//         }
//     }
//     if (local_stop != 0) stop = local_stop;
//     while (DMA_reply[1 - k] != 3);
// }
