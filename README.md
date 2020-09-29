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

## Generate picture of result

We provide the python script to generate the result picture, You need to put the result into `source/result.csv`, then run the 

script `result_plot.py` in `source` directory, and the result picture will be saved into PDF file `result.pdf`

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
