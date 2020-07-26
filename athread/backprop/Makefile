# C compiler
CC = sw5cc
CC_FLAGS = -O2


openacc: facetrain.o imagenet.o backprop_kernel.o backprop.o backprop_slave.o
	$(CC) -hybrid $(CC_FLAGS) $^ -o $@ -lm -lm_slave

%.o: %.[ch]
	$(CC) $(CC_FLAGS) $< -c

facetrain.o: facetrain.c backprop.h
	$(CC) -host $(CC_FLAGS) facetrain.c -c
	
backprop.o: backprop.c backprop.h
	$(CC) -host $(CC_FLAGS) backprop.c -c

backprop_slave.o: backprop_slave.c
	$(CC) -slave $(CC_FLAGS) backprop_slave.c -c

backprop_kernel.o: backprop_kernel.c backprop.h
	$(CC) -host $(CC_FLAGS) backprop_kernel.c -c

imagenet.o: imagenet.c backprop.h
	$(CC) -host $(CC_FLAGS) imagenet.c -c


run:
	bsub -I -b -q q_sw_expr -n 1 -cgsp 64 -share_size 2048 ./openacc 65536
	
clean:
	rm -f *.o *~ backprop backprop_cuda.linkinfo
