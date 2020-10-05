CC = sw5cc
CCFLAGS = -g -O3
OUTPUT = *.out

single:
	$(CC) -host -o single $(CCFLAGS) single.c -lm

run:
	bsub -I -q q_sw_expr -n 1 -cgsp 64 ./single

clean:
	rm -f single $(OUTPUT)
