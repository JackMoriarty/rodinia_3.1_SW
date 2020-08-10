CC = swacc
CFLAGS = -O2

.PHONY: clean run

needle: needle_openacc.c
	$(CC) $(CFLAGS) $^ -o $@

run:
	bsub -I -b -q q_sw_cess -n 1 -cgsp 64 -share_size 2048 ./needle

clean:
	rm -rf needle result.txt