CC = sw5cc
LIB_PATH = ../../../
LIBS = -lswcs
INCLUDE = ../../../include
CFLAGS = -O2

.PHONY: clean run

needle: needle_openacc.c
	$(CC) -host $(CFLAGS) $^ -I$(INCLUDE) $(LIBS) -L$(LIB_PATH) -o $@

run:
	bsub -I -q q_sw_cess -n 1 -cgsp 64 ./needle

clean:
	rm -rf needle result.txt