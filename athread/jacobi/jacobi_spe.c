#include <slave.h>
#include <stdlib.h>
#include <math.h>

#define ALLSYN  athread_syn(ARRAY_SCOPE,0xffff)

#define M 5000
#define N 50000
#define MAX_THREAD 64

double b[N];
double x[N];
double xnew[N];
double d_array[MAX_THREAD];
double r_array[MAX_THREAD];

void func()
{
    int tid, i, it;
    int high, low, chunk, size;
    double t, *local_xnew, *local_x, *local_b;
    double local_d, local_r;
    volatile int DMA_reply;

    chunk = N / MAX_THREAD;
    low = _MYID * chunk;
    high = (_MYID == (MAX_THREAD - 1)) ? N : (low + chunk);
    size = high - low;

    local_xnew = ldm_malloc(sizeof(double) * size);
    if (local_xnew == NULL)
        exit(EXIT_FAILURE);
    local_x = ldm_malloc(sizeof(double) * size);
    if (local_x == NULL)
        exit(EXIT_FAILURE);
    local_b = ldm_malloc(sizeof(double) * size);
    if (local_b == NULL)
        exit(EXIT_FAILURE);


    for ( i = 0; i < size; i++) {
        local_b[i] = 0.0;
    }
    if (_MYID == MAX_THREAD - 1)
        local_b[size - 1] = (double) (N + 1);
    
    for (i = 0; i < size; i++) {
        local_x[i] = 0.0;
    }

    DMA_reply = 0;
    athread_put(PE_MODE, local_b, &b[low], sizeof(double) * size, &DMA_reply, 0, 0);
    athread_put(PE_MODE, local_x, &x[low], sizeof(double) * size, &DMA_reply, 0, 0);
    while(DMA_reply != 2);
    // ALLSYN;

    for (it = 0; it < M; it++) {
        for (i = 0; i < size; i++) {
            local_xnew[i] = local_b[i];
            if (0 < i + low) {
                double temp_x;
                if (i == 0)
                    temp_x = x[low - 1];
                else 
                    temp_x = local_x[i - 1];
                local_xnew[i] += temp_x;
            }
            if (i + low < N - 1) {
                double temp_x;
                if (i == size - 1)
                    temp_x = x[i+low+1];
                else 
                    temp_x = local_x[i + 1];
                local_xnew[i] += temp_x;
            }
            local_xnew[i] = local_xnew[i] / 2.0;
        }

        local_d = 0;
        for (i = 0; i < size; i++) {
            double temp = local_x[i] - local_xnew[i];
            local_d += temp * temp;
        }

        d_array[_MYID] = local_d;

        for (i = 0; i < size; i++) {
            local_x[i] = local_xnew[i];
        }
        DMA_reply = 0;
        athread_put(PE_MODE, local_x, &x[low], sizeof(double) * size, &DMA_reply, 0, 0);
        while(DMA_reply != 1);

        ALLSYN;
        local_r = 0.0;
        for (i = 0; i < size; i++) {
            t = local_b[i] - 2.0 * local_x[i];
            if ( 0 < i + low) {
                double temp_x;
                if (i == 0)
                    temp_x = x[low - 1];
                else 
                    temp_x = local_x[i - 1];
                t += temp_x;
            }
            if (i + low < N - 1) {
                double temp_x;
                if (i == size - 1)
                    temp_x = x[i+low+1];
                else 
                    temp_x = local_x[i + 1];

                t += temp_x;
            }
            local_r += t * t;
        }
        r_array[_MYID] = local_r;
    }

    return;
}
