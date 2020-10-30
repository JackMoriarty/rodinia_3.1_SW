CC = sw5cc
CFLAGS = -host -O2

SRC_DIR := src
BIN_DIR := bin

stencil: $(SRC_DIR)/main.o $(SRC_DIR)/file.o $(SRC_DIR)/kernels.o $(SRC_DIR)/parboil.o
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $(BIN_DIR)/$@

%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@

.PHONY: clean run

run:
	bsub -I -b -q q_sw_expr -n 1 -cgsp 64 -share_size 4096 ./$(BIN_DIR)/stencil 512 512 64 100 -i ./datasets/default/input/512x512x64x100.bin -o ./$(BIN_DIR)/output

clean:
	rm -rf $(SRC_DIR)/*.o $(BIN_DIR)/stencil $(BIN_DIR)/output
