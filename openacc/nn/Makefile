CC = swacc
CFLAGS = -O2 -lm -Wall


all : nn



nn : nn.c
	$(CC) -o $@ $< $(LDFLAGS) $(CFLAGS) 


.PHONY: clean run

clean:
	rm -rf *.o nn

run: 
	bsub -I -b -q q_sw_expr -n 1 -cgsp 64 -share_size 2048 ./nn filelist_4 5 30 90
