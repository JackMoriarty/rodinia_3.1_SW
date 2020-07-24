#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

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
float euclid_dist_2(float *pt1, float *pt2);
int find_nearest_point(float *pt);

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
    memset(partial_new_centers_len, 0, NTHREADS * NCLUSTERS * sizeof(float));
    memset(partial_new_centers, 0, NTHREADS * NCLUSTERS * NFEATURES * sizeof(float));

    printf("num of threads = %d\n", NTHREADS);
    do {
        delta = 0;
        int tid;
        /************** OPENMP **************/
        // #pragma acc parallel loop \
        //     cache(membership:16) \
        //     copyin(clusters) annotate(entire(clusters)) \
        //     reduction(+:delta) copy(partial_new_centers_len, partial_new_centers)
        #pragma acc parallel loop reduction(+:delta) copyin(clusters) annotate(entire(clusters)) cache(membership:16) copy(partial_new_centers_len, partial_new_centers)
        for (tid = 0; tid < NTHREADS; tid++)
        {
            int index;
            int chunksize = NPOINTS / 64;
            int left = tid * chunksize;
            int right = (tid == 63) ? NPOINTS : (tid + 1) * chunksize;
            #pragma acc data copyin(attributes[i][*])
            for (i = left; i < right; i++) {
                // index = find_nearest_point(attributes[i]);
                /************ modify ************/
                int loop_m, loop_n;
                float min_dist = FLT_MAX;
                for (loop_m = 0; loop_m < NCLUSTERS; loop_m++) {
                    float dist = 0;
                    for (loop_n = 0; loop_n < NFEATURES; loop_n++)
                        dist += (attributes[i][loop_n] - clusters[loop_m][loop_n]) * (attributes[i][loop_n] - clusters[loop_m][loop_n]);
                    if (dist < min_dist) {
                        min_dist = dist;
                        index = loop_m;
                    }
                }
                /************ modify ************/

                if (membership[i] != index)
                    delta += 1.0;
                membership[i] = index;

                partial_new_centers_len[tid][index]++;
                for (j = 0; j < NFEATURES; j++)
                    partial_new_centers[tid][index][j] += attributes[i][j];
            }
        }
        /********** end of OPENMP ***********/

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

        for (i = 0; i < NCLUSTERS; i++) {
            for (j = 0; j < NFEATURES; j++) {
                if (new_centers_len[i] > 0)
                    clusters[i][j] = new_centers[i][j] / new_centers_len[i];
                new_centers[i][j] = 0;
            }
            new_centers_len[i] = 0;
        }
        // printf("%lf %d\n", delta, loop);
    } while (delta > threshold && loop++ < 500);
    return (float *)clusters;
}

// #pragma acc routine
// inline float euclid_dist_2(float *pt1, float *pt2)
// {
//     int i;
//     float ans = 0;

//     for (i = 0; i < NFEATURES; i++)
//         ans += (pt1[i] - pt2[i]) * (pt1[i] - pt2[i]);
    
//     return ans;
// }

// #pragma acc routine
// int find_nearest_point(float *pt)
// {
//     int index, i;
//     float min_dist = FLT_MAX;

//     #pragma acc data copyin(clusters)
//     for (i = 0; i < NCLUSTERS; i++) {
//         float dist;
//         dist = euclid_dist_2(pt, clusters[i]);
//         if (dist < min_dist) {
//             min_dist = dist;
//             index = i;
//         }
//     }

//     return index;
// }
