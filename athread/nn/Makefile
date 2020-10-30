CC = sw5cc
CFLAGS = -O2 -Wall
LDFLAGS= -lm -lm_slave

nn: nn.o nn_slave.o
	$(CC) -hybrid $(CFLAGS) $^ $(LDFLAGS) -o $@

nn.o : nn.c
	$(CC) -host -c -o $@ $^  $(CFLAGS)

nn_slave.o: nn_slave.c
	$(CC) -slave -c -o $@ $^  $(CFLAGS)

.PHONY: clean run

clean:
	rm -rf *.o nn

run: 
	bsub -I -b -q q_sw_expr -n 1 -cgsp 64 -share_size 2048 ./nn filelist_4 5 30 90
