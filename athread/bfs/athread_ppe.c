#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <athread.h>

#define NO_OF_NODES 1000000
#define EDGE_LIST_SIZE 5999970

FILE *fp;
char *input_f = "../../data/bfs/graph1MW_6.txt";

struct Node {
    int starting;
    int no_of_edges;
};

struct Node h_graph_nodes[NO_OF_NODES];
unsigned char h_graph_mask[NO_OF_NODES];
unsigned char h_updating_graph_mask[NO_OF_NODES];
unsigned char h_graph_visited[NO_OF_NODES];
int h_cost[NO_OF_NODES];
int h_graph_edges[EDGE_LIST_SIZE];
unsigned char stop;

void BFSGraph();
void slave_spe_func_1();
void slave_spe_func_2();
void slave_spe_func();

int main(int argc, char *argv[])
{
    athread_init();

    BFSGraph();

    athread_halt();
    return 0;
}

void BFSGraph()
{
    int dummy_val = 0;
    unsigned int i;
    double spendtime;
    struct timeval start_time, end_time;
    
    printf("reading file\n");
    fp = fopen(input_f, "r");
    if (fp == NULL) {
        printf("Error reading graph file\n");
        exit(EXIT_FAILURE);
    }

    int source = 0;

    /* 该值无用 */
    fscanf(fp, "%d", &dummy_val);

    int start, edgeno;
    for (i = 0; i < NO_OF_NODES; i++) {
        fscanf(fp, "%d %d", &start, &edgeno);
        h_graph_nodes[i].starting = start;
        h_graph_nodes[i].no_of_edges = edgeno;
        h_graph_mask[i] = 0;
        h_updating_graph_mask[i] = 0;
        h_graph_visited[i] = 0;
    }

    fscanf(fp, "%d", &source);

    h_graph_mask[source] = 1;
    h_graph_visited[source] = 1;

    /* 该值无用 */
    fscanf(fp, "%d", &dummy_val);

    int id, cost;
    for (i= 0; i< EDGE_LIST_SIZE; i++) {
        fscanf(fp, "%d", &id);
        fscanf(fp, "%d", &cost);
        h_graph_edges[i] = id;
    }

    if (fp)
        fclose(fp);

    for (i = 0; i < NO_OF_NODES; i++)
        h_cost[i] = -1;
    h_cost[source] = 0;

    printf("Starting traversing the tree\n");
    gettimeofday(&start_time, NULL);
    do {
        stop = 0;
        
        athread_spawn(spe_func_1, NULL);
        athread_join();

        // int temp_i;
        // printf("********************************\n");
        // for (temp_i = 0; temp_i < NO_OF_NODES; temp_i++) {
        //     printf("%d %d %d %d\n", (int)h_graph_mask[temp_i], (int)h_updating_graph_mask[temp_i], (int)h_graph_visited[temp_i], h_cost[temp_i]);
        // }
        // exit(EXIT_SUCCESS);

        // athread_spawn(spe_func_2, NULL);
        // athread_join();
        int tid;
        for (tid = 0; tid <NO_OF_NODES; tid++) {
            if (h_updating_graph_mask[tid] != 0) {
                h_graph_mask[tid] = 1;
                h_graph_visited[tid] = 1;
                stop = 1;
                h_updating_graph_mask[tid] = 0;
            }
        }
        // printf("-------------------------------_\n");
        // for (temp_i = 0; temp_i < NO_OF_NODES; temp_i++) {
        //     printf("%d %d %d %d\n", (int)h_graph_mask[temp_i], (int)h_updating_graph_mask[temp_i], (int)h_graph_visited[temp_i], h_cost[temp_i]);
        // }
        // exit(EXIT_SUCCESS);
    } while (stop != 0);
    gettimeofday(&end_time, NULL);
    spendtime = 1000000*(end_time.tv_sec - start_time.tv_sec) + end_time.tv_usec - start_time.tv_usec;
    printf("spend time: %lf(us)\n", spendtime);
    
    FILE *fpo = fopen("result.txt", "w");
    for (i = 0; i < NO_OF_NODES; i++)
        fprintf(fpo,"%d) cost:%d\n", i, h_cost[i]);
    fclose(fpo);
    printf("Result stored in result.txt\n");
}

