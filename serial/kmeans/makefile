CC = sw5cc
CFLAGS = -O2

.PHONY: clean run

single: kmeans.c
	$(CC) -host $(CFLAGS) kmeans.c -I$(INCLUDE) $(LIBS) -L$(LIB_PATH) -o $@

run:
	bsub -I -q q_sw_expr -n 1 -cgsp 64 ./single

clean:
	rm -rf single
