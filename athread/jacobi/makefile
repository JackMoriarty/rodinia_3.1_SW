CC = sw5cc

jacobi_athread:jacobi_ppe.o jacobi_spe.o
	$(CC) -hybrid -O2 $^ -lm -o $@

jacobi_ppe.o: jacobi_ppe.c 
	$(CC) -host -c $^ -o $@

jacobi_spe.o: jacobi_spe.c 
	$(CC) -slave -c $^ -o $@

clean:
	rm -rf jacobi_athread *.o

run:
	bsub -I -b -q q_sw_expr -n 1 -cgsp 64 ./jacobi_athread
