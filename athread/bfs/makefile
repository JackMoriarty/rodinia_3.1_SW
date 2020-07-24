CC = sw5cc
CFLAGS = -O2
APP_NAME = athread

.PHONY: clean run

athread: $(APP_NAME)_ppe.o $(APP_NAME)_spe.o
	$(CC) -hybrid $(CFLAGS)  $^ -o $@

%_spe.o: %_spe.c
	$(CC) -slave -c $^ -o $@

%_ppe.o: %_ppe.c
	$(CC) -host -c  $^ -o $@

run:
	bsub -I -b -q q_sw_cess -n 1 -cgsp 64 -share_size 2048 ./athread

clean:
	rm -rf *.o athread
