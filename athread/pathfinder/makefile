CC = sw5cc
CFLAGS = -O2
APP_NAME = athread

.PHONY: clean run

athread: $(APP_NAME)_ppe.o $(APP_NAME)_spe.o
	$(CC) -hybrid  $^ $(CFLAGS) -o $@

%_spe.o: %_spe.c
	$(CC) -slave -c $(CFLAGS) $^ -o $@

%_ppe.o: %_ppe.c
	$(CC) -host -c $(CFLAGS) $^ -o $@

run:
	bsub -I -b -q q_sw_cess -n 1 -cgsp 64 ./athread

clean:
	rm *.o athread