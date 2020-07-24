CC = sw5cc
CFLAGS = -O2

.PHONY: clean run

athread: athread_ppe.o athread_spe.o
	$(CC) -hybrid $(CFLAGS) $^ -o $@

%_spe.o: %_spe.c
	$(CC) -slave -c $^ -msimd -o $@

%_ppe.o: %_ppe.c
	$(CC) -host -c $^ -o $@

run:
	bsub -I -b -q q_sw_cess -n 1 -cgsp 64 -share_size 2048 ./athread

clean:
	rm *.o athread