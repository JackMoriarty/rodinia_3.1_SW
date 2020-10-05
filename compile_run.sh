#!/bin/bash
root_dir=$(pwd)
# clean result
rm -f result.txt

# compile and run
cd $root_dir/serial
dir=$(ls -l|awk '/^d/ {print $NF}')
for i in $dir
do
    cd $i
    make clean
    make
    for j in $(seq 5)
    do
        Time=$(make run|grep -i time|grep -o '[0-9]*\.[0-9]*')
        result_string="$i:Serial:$Time"
        echo $result_string >> $root_dir/result.txt
    done
    cd ..
done

cd $root_dir/openacc
dir=$(ls -l|awk '/^d/ {print $NF}')
for i in $dir
do
    cd $i
    make clean
    make
    for j in $(seq 5)
    do
        Time=$(make run|grep -i time|grep -o '[0-9]*\.[0-9]*')
        result_string="$i:openACC:$Time"
        echo $result_string >> $root_dir/result.txt
    done
    cd ..
done

cd $root_dir/athread
dir=$(ls -l|awk '/^d/ {print $NF}')
for i in $dir
do
    cd $i
    make clean
    make
    for j in $(seq 5)
    do
        Time=$(make run|grep -i time|grep -o '[0-9]*\.[0-9]*')
        result_string="$i:Athread:$Time"
        echo $result_string >> $root_dir/result.txt
    done
    cd ..
done

echo 'Finish'
