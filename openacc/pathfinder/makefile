CC = swacc
CFLAGS = -O2

.PHONY: clean run

openacc: openacc.c
	$(CC) $(CFLAGS) $^ -o $@

run:
	bsub -I -q q_sw_expr -n 1 -cgsp 64 ./openacc

clean:
	rm -rf openacc
