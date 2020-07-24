CC = swacc
CCFLAGS = -g -O3
OUTPUT = *.out

openacc:
	$(CC) -o openacc $(CCFLAGS) openacc.c -lm

run:
	bsub -I -q q_sw_expr -n 1 -cgsp 64 ./openacc

clean:
	rm -f openacc $(OUTPUT)
