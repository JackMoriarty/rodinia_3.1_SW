# rodinia_3.1_SW
rodinia_3.1 benchmark for Sunway TaihuLight.

## Compile

Enter the subfolder and run the following command:

```bash
make
```

## Run

After compile, run following command in subfolder:

```bash
make run
```

Then, the program will be submited to Sunway TaihuLight and wait for execution in queue.

Besides, we also provide shell script `compile_run.sh`, this script will run all benchmark and save the output in `result.txt`

## Generate picture of result

We provide the python script to generate the result picture, You need to download the `result.txt` and  `result_plot.py` located in `plot` directory to  the same folder on your PC, then run the script `result_plot.py` , and the result picture will be saved into PDF file `result.pdf`

## Example

Run athread version of BFS for example.

1. Enter BFS source folder.

   ```bash
   cd /path/to/rodinia_3.1_SW/athread/bfs
   ```

2. compile the source code.

   ```bash
   make
   ```

3. submit to Sunway TaihuLight.

   ```bash
   make run
   ```

Run all benchmarks.

1. Enter root folder.

   ```bash
   cd /path/to/rodinia_3.1_SW/
   ```

2. Execute the bash script, then the output file `result.txt` will be generated.

   ```bash
   ./compile_run.sh
   ```

3. Download `result.txt`and `result_plot.py` located in `plot` directory to  the same folder on your PC.

4. Generate result figure.

   ```bash
   python result_plot.py
   ```

