CC = sw5cc
CFLAGS = -O2

.PHONY: clean run

single: single.c
	$(CC) -host $(CFLAGS) $^ -o $@

run:
	bsub -I -q q_sw_expr -n 1 -cgsp 64 ./single

clean:
	rm -rf single
