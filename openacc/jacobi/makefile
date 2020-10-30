CC = swacc

jacobi_openacc: jacobi_openacc.c
	$(CC) -O2 $^ -lm -o $@

.PHONY: clean run
clean:
	rm -rf jacobi_openacc *.o

run:
	bsub -I -b -q q_sw_expr -n 1 -cgsp 64 -share_size 1024 ./jacobi_openacc
