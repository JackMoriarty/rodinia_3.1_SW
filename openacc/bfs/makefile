CC = swacc
CFLAGS = -O2
LIBS = -lm

.PHONY: clean run

openacc: bfs.c
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

run:
	bsub -I -b -q q_sw_expr -n 1 -cgsp 64 -share_size 2048 ./openacc

clean:
	rm -rf openacc
