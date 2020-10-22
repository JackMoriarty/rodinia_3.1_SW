#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h> 
#include <math.h> 
#include <sys/time.h>
#include <string.h>

#define STR_SIZE (256)
#define MAX_PD	(3.0e6)
/* required precision in degrees	*/
#define PRECISION	0.001
#define SPEC_HEAT_SI 1.75e6
#define K_SI 100
/* capacitance fitting factor	*/
#define FACTOR_CHIP	0.5

#define NUMCOLS 512
#define NUMROWS 512
#define LAYERS 8
#define SIZE 2097152 //(NUMROWS * NUMCOLS * LAYERS) line:157

float powerIn[SIZE], tempCopy[SIZE], tempIn[SIZE], tempOut[SIZE], answer[SIZE];

/* chip parameters	*/
float t_chip = 0.0005;
float chip_height = 0.016; float chip_width = 0.016; 
/* ambient temperature, assuming no package at all	*/
float amb_temp = 80.0;

void fatal(char *s)
{
    fprintf(stderr, "Error: %s\n", s);
}

void readinput(float *vect, int grid_rows, int grid_cols, int layers, char *file) {
    int i,j,k;
    FILE *fp;
    char str[STR_SIZE];
    float val;

    if( (fp  = fopen(file, "r" )) ==0 )
      fatal( "The file was not opened" );


    for (i=0; i <= grid_rows-1; i++) 
      for (j=0; j <= grid_cols-1; j++)
        for (k=0; k <= layers-1; k++)
          {
            if (fgets(str, STR_SIZE, fp) == NULL) fatal("Error reading file\n");
            if (feof(fp))
              fatal("not enough lines in file");
            if ((sscanf(str, "%f", &val) != 1))
              fatal("invalid file format");
            vect[i*grid_cols+j+k*grid_rows*grid_cols] = val;
          }

    fclose(fp);	

}


void writeoutput(float *vect, int grid_rows, int grid_cols, int layers, char *file) {

    int i,j,k, index=0;
    FILE *fp;
    char str[STR_SIZE];

    if( (fp = fopen(file, "w" )) == 0 )
      printf( "The file was not opened\n" );

    for (i=0; i < grid_rows; i++) 
      for (j=0; j < grid_cols; j++)
        for (k=0; k < layers; k++)
          {
            sprintf(str, "%d\t%g\n", index, vect[i*grid_cols+j+k*grid_rows*grid_cols]);
            fputs(str,fp);
            index++;
          }

    fclose(fp);	
}



void computeTempCPU(float *pIn, float* tIn, float *tOut, 
        int nx, int ny, int nz, float Cap, 
        float Rx, float Ry, float Rz, 
        float dt, int numiter) 
{   float ce, cw, cn, cs, ct, cb, cc;
    float stepDivCap = dt / Cap;
    ce = cw =stepDivCap/ Rx;
    cn = cs =stepDivCap/ Ry;
    ct = cb =stepDivCap/ Rz;

    cc = 1.0 - (2.0*ce + 2.0*cn + 3.0*ct);

    int c,w,e,n,s,b,t;
    int x,y,z;
    int i = 0;
    do{
        for(z = 0; z < nz; z++)
            for(y = 0; y < ny; y++)
                for(x = 0; x < nx; x++)
                {
                    c = x + y * nx + z * nx * ny;

                    w = (x == 0) ? c      : c - 1;
                    e = (x == nx - 1) ? c : c + 1;
                    n = (y == 0) ? c      : c - nx;
                    s = (y == ny - 1) ? c : c + nx;
                    b = (z == 0) ? c      : c - nx * ny;
                    t = (z == nz - 1) ? c : c + nx * ny;


                    tOut[c] = tIn[c]*cc + tIn[n]*cn + tIn[s]*cs + tIn[e]*ce + tIn[w]*cw + tIn[t]*ct + tIn[b]*cb + (dt/Cap) * pIn[c] + ct*amb_temp;
                }
        float *temp = tIn;
        tIn = tOut;
        tOut = temp; 
        i++;
    }
    while(i < numiter);

}

float accuracy(float *arr1, float *arr2, int len)
{
    float err = 0.0; 
    int i;
    for(i = 0; i < len; i++)
    {
        err += (arr1[i]-arr2[i]) * (arr1[i]-arr2[i]);
    }

    return (float)sqrt(err/len);


}
void computeTempOMP(float *pIn, float* tIn, float *tOut, 
        int nx, int ny, int nz, float Cap, 
        float Rx, float Ry, float Rz, 
        float dt, int numiter) 
{  

    float ce, cw, cn, cs, ct, cb, cc;

    float stepDivCap = dt / Cap;
    ce = cw =stepDivCap/ Rx;
    cn = cs =stepDivCap/ Ry;
    ct = cb =stepDivCap/ Rz;

    cc = 1.0 - (2.0*ce + 2.0*cn + 3.0*ct);


    #pragma acc parallel packin(amb_temp,nx,ny,nz,Cap,dt,numiter,ce,cw,cn,cs,ct,cb,cc) cache(pIn) annotate(readonly(pIn))
    {
        int count = 0;
        float *tIn_t = tIn;
        float *tOut_t = tOut;

        do {
            int z; 
            #pragma acc loop
            for (z = 0; z < nz; z++) {
                int y;
                for (y = 0; y < ny; y++) {
                    int x;
                    for (x = 0; x < nx; x++) {
                        int c, w, e, n, s, b, t;
                        c =  x + y * nx + z * nx * ny;
                        w = (x == 0)    ? c : c - 1;
                        e = (x == nx-1) ? c : c + 1;
                        n = (y == 0)    ? c : c - nx;
                        s = (y == ny-1) ? c : c + nx;
                        b = (z == 0)    ? c : c - nx * ny;
                        t = (z == nz-1) ? c : c + nx * ny;
                        tOut_t[c] = cc * tIn_t[c] + cw * tIn_t[w] + ce * tIn_t[e]
                            + cs * tIn_t[s] + cn * tIn_t[n] + cb * tIn_t[b] + ct * tIn_t[t]+(dt/Cap) * pIn[c] + ct*amb_temp;
                    }
                }
            }
            float *t_p = tIn_t;
            tIn_t = tOut_t;
            tOut_t = t_p; 
            count++;
        } while (count < numiter);
    } 
    return; 
} 

int main(int argc, char** argv)
{
    char *pfile, *tfile, *ofile;// *testFile;
    int iterations = 100;

    pfile = "../../data/hotspot3D/power_512x8";
    tfile = "../../data/hotspot3D/temp_512x8";
    ofile = "output.out";

    /* calculating parameters*/

    float dx = chip_height/NUMROWS;
    float dy = chip_width/NUMCOLS;
    float dz = t_chip/LAYERS;

    float Cap = FACTOR_CHIP * SPEC_HEAT_SI * t_chip * dx * dy;
    float Rx = dy / (2.0 * K_SI * t_chip * dx);
    float Ry = dx / (2.0 * K_SI * t_chip * dy);
    float Rz = dz / (K_SI * dx * dy);

    // cout << Rx << " " << Ry << " " << Rz << endl;
    float max_slope = MAX_PD / (FACTOR_CHIP * t_chip * SPEC_HEAT_SI);
    float dt = PRECISION / max_slope;

    memset(powerIn, 0, sizeof(float) * SIZE);
    memset(tempIn, 0, sizeof(float) * SIZE);
    memset(tempOut, 0, sizeof(float) * SIZE);
    memset(answer, 0, sizeof(float) * SIZE);

    // outCopy = (float*)calloc(size, sizeof(float));
    readinput(powerIn,NUMROWS, NUMCOLS, LAYERS,pfile);
    readinput(tempIn, NUMROWS, NUMCOLS, LAYERS, tfile);

    memcpy(tempCopy,tempIn, SIZE * sizeof(float));

    struct timeval start, stop;
    float time;
    gettimeofday(&start,NULL);
    computeTempOMP(powerIn, tempIn, tempOut, NUMCOLS, NUMROWS, LAYERS, Cap, Rx, Ry, Rz, dt,iterations);
    gettimeofday(&stop,NULL);
    time = (stop.tv_usec-start.tv_usec) + (stop.tv_sec - start.tv_sec)*1e6;
    computeTempCPU(powerIn, tempCopy, answer, NUMCOLS, NUMROWS, LAYERS, Cap, Rx, Ry, Rz, dt,iterations);

    float acc = accuracy(tempOut,answer,NUMROWS*NUMCOLS*LAYERS);
    printf("Time: %f (us)\n",time);
    printf("Accuracy: %e\n",acc);
    writeoutput(tempOut,NUMROWS, NUMCOLS, LAYERS, ofile);
    // free(tempIn);
    // free(tempOut); free(powerIn);
    return 0;
}	


