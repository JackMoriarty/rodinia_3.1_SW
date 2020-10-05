# C compiler
CC = sw5cc
CC_FLAGS = -host -O2


backprop: backprop.o facetrain.o imagenet.o backprop_kernel.o 
	$(CC) $(CC_FLAGS) facetrain.o imagenet.o backprop_kernel.o backprop.o -o $@ -lm

%.o: %.[ch]
	$(CC) $(CC_FLAGS) $< -c

facetrain.o: facetrain.c backprop.h
	$(CC) $(CC_FLAGS) facetrain.c -c
	
backprop.o: backprop.c backprop.h
	$(CC) $(CC_FLAGS) backprop.c -c

backprop_kernel.o: backprop_kernel.c backprop.h
	$(CC) $(CC_FLAGS) backprop_kernel.c -c

imagenet.o: imagenet.c backprop.h
	$(CC) $(CC_FLAGS) imagenet.c -c


run:
	bsub -I -b -q q_sw_expr -n 1 -cgsp 64 -share_size 2048 ./backprop 65536
	
clean:
	rm -f *.o *~ backprop backprop_cuda.linkinfo
