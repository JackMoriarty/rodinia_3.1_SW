#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <athread.h>

#define NUM_OBJECTS 494020
#define NUM_ATTRIBUTES 34
#define NCLUSTERS 5
#define NTHREADS 64
#define FLT_MAX 3.40282347e+38
#define NFEATURES NUM_ATTRIBUTES
#define NPOINTS NUM_OBJECTS

char filename[30] = "../../data/kmeans/kdd_cup";

float attributes[NUM_OBJECTS][NUM_ATTRIBUTES];
float clusters[NCLUSTERS][NFEATURES];
int membership[NUM_OBJECTS];
int new_centers_len[NCLUSTERS];
float new_centers[NCLUSTERS][NFEATURES];
int partial_new_centers_len[NTHREADS][NCLUSTERS];
float partial_new_centers[NTHREADS][NCLUSTERS][NFEATURES];
float delta;

int cluster(float threshold, float **cluster_centres);
float *kmeans_clustering(float threshold);

extern void slave_spe_func();

int main(int argc, char *argv[])
{
    FILE *infile;
    char line[1024];
    int i, j;
    double spendtime;
    struct timeval start, end;
    int nloops = 1;
    float threshold = 0.001;
    float *buf;
    float *cluster_centres;

    athread_init();

    if ((infile = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "Error: no such file (%s)\n", filename);
        exit(EXIT_FAILURE);
    }

    i = 0;
    buf = (float *)attributes;
    while (fgets(line, 1024, infile) != NULL) {
        if (strtok(line, " \t\n") == NULL) continue;
        for (j = 0; j < NUM_ATTRIBUTES; j++) {
            buf[i] = atof(strtok(NULL, " ,\t\n"));
            i++;
        }
    }
    fclose(infile);
    printf("I/O completed\n");

    memcpy(attributes, buf, NUM_ATTRIBUTES * NUM_OBJECTS * sizeof(float));
    
    gettimeofday(&start, NULL);

    for (i = 0; i < nloops; i++) {
        cluster_centres = NULL;
        cluster(threshold, &cluster_centres);
    }

    gettimeofday(&end, NULL);
    spendtime = 1000000*(end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;

    printf("number of Clusters %d\n", NCLUSTERS);
    printf("number of Attributes %d\n\n", NUM_ATTRIBUTES);
    printf("Time for process: %lf(us)\n", spendtime);

    for (i = 0; i < NCLUSTERS; i++) {
        printf("%d)", i);
        for (j = 0; j < NUM_ATTRIBUTES; j++) {
            printf("%f ", clusters[i][j]);
        }
        printf("\n");
    }

    athread_halt();
    return 0;
}

int cluster(float threshold, float **cluster_centres)
{

    srand(7);
    *cluster_centres = kmeans_clustering(threshold);

    return 0;
}

float *kmeans_clustering(float threshold)
{
    int i, j, k;
    int n = 0;
    int loop = 0;

    for (i = 1; i < NCLUSTERS; i++) {
        for (j = 0; j < NFEATURES; j++)
            clusters[i][j] = attributes[n][j];
        n++;
    }

    for (i = 0; i < NPOINTS; i++)
        membership[i] = -1;
    
    memset(new_centers_len, 0, NCLUSTERS * sizeof(int));
    memset(new_centers, 0, NCLUSTERS * NFEATURES * sizeof(float));
    memset(partial_new_centers_len, 0, NTHREADS * NCLUSTERS * sizeof(int));
    memset(partial_new_centers, 0, NTHREADS * NCLUSTERS * NFEATURES * sizeof(float));

    printf("num of threads = %d\n", NTHREADS);
    do {
        delta = 0;
        
        athread_spawn(spe_func, NULL);
        athread_join();

        for (i = 0; i < NCLUSTERS; i++) {
            for (j = 0; j < NTHREADS; j++) {
                new_centers_len[i] += partial_new_centers_len[j][i];
                partial_new_centers_len[j][i] = 0;
                for (k = 0; k < NFEATURES; k++) {
                    new_centers[i][k] += partial_new_centers[j][i][k];
                    partial_new_centers[j][i][k] = 0;
                }
            }
        }

        // printf("-------------------------------\n");
        // for (i = 0; i < NCLUSTERS; i++) {
        //     printf("%d ", new_centers_len[i]);
        // }
        // printf("\n---------------------------------\n");
        // for (i = 0; i < NCLUSTERS; i++) {
        //     for (j = 0; j < NFEATURES; j++) {
        //         printf("%f ", new_centers[i][j]);
        //     }
        // }
        // exit(EXIT_SUCCESS);

        for (i = 0; i < NCLUSTERS; i++) {
            for (j = 0; j < NFEATURES; j++) {
                if (new_centers_len[i] > 0)
                    clusters[i][j] = new_centers[i][j] / new_centers_len[i];
                new_centers[i][j] = 0;
            }
            new_centers_len[i] = 0;
        }
    } while (delta > threshold && loop++ < 500);
    return (float *)clusters;
}
