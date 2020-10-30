
/***************************************************************************
 *cr
 *cr            (C) Copyright 2010 The Board of Trustees of the
 *cr                        University of Illinois
 *cr                         All Rights Reserved
 *cr
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "file.h"
#include "common.h"
#include "kernels.h"
#include "parboil.h"


static int read_data(float *A0, int nx,int ny,int nz,FILE *fp) 
{	
	int s=0;
        int i, j, k;
	for(i=0;i<nz;i++)
	{
		for(j=0;j<ny;j++)
		{
			for(k=0;k<nx;k++)
			{
                                fread(A0+s,sizeof(float),1,fp);
				s++;
			}
		}
	}
	return 0;
}

int main(int argc, char** argv) {
	struct pb_Parameters *parameters;
	struct timeval start, end;
    double spend_time;
	
	printf("CPU-based 7 points stencil codes****\n");
	printf("Original version by Li-Wen Chang <lchang20@illinois.edu> and I-Jui Sung<sung10@illinois.edu>\n");
	printf("This version maintained by Chris Rodrigues  ***********\n");
	parameters = pb_ReadParameters(&argc, argv);
    
    gettimeofday(&start, NULL);
	//declaration
	int nx,ny,nz;
	int size;
    int iteration;
	float c0=1.0f/6.0f;
	float c1=1.0f/6.0f/6.0f;

	if (argc<5) 
    {
      printf("Usage: probe nx ny nz tx ty t\n"
	     "nx: the grid size x\n"
	     "ny: the grid size y\n"
	     "nz: the grid size z\n"
		  "t: the iteration time\n");
      return -1;
    }

	nx = atoi(argv[1]);
	if (nx<1)
		return -1;
	ny = atoi(argv[2]);
	if (ny<1)
		return -1;
	nz = atoi(argv[3]);
	if (nz<1)
		return -1;
	iteration = atoi(argv[4]);
	if(iteration<1)
		return -1;

	
	//host data
	float *h_A0;
	float *h_Anext;

	size=nx*ny*nz;
	
	h_A0=(float*)malloc(sizeof(float)*size);
	h_Anext=(float*)malloc(sizeof(float)*size);
  FILE *fp = fopen(parameters->inpFiles[0], "rb");
	read_data(h_A0, nx,ny,nz,fp);
  fclose(fp);
  memcpy (h_Anext,h_A0 ,sizeof(float)*size);


  

  int t;
	for(t=0;t<iteration;t++)
	{
		cpu_stencil(c0,c1, h_A0, h_Anext, nx, ny,  nz);
    float *temp=h_A0;
    h_A0 = h_Anext;
    h_Anext = temp;

	}

  float *temp=h_A0;
  h_A0 = h_Anext;
  h_Anext = temp;

  gettimeofday(&end, NULL);
  spend_time = (end.tv_sec - start.tv_sec)*1e6 + (end.tv_usec - start.tv_usec);
  printf("Spend Time: %lf us \n", spend_time);
 
	if (parameters->outFile) {
		outputData(parameters->outFile,h_Anext,nx,ny,nz);
		
	}
	free (h_A0);
	free (h_Anext);

	pb_FreeParameters(parameters);

	return 0;

}
