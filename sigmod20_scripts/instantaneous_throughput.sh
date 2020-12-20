#!/bin/bash

logdir=logs
make clean; make main; mkdir -p $logdir

numobs=$(($1/8))
file=$2/streamdump_mmap_active_new_$numobs
log_num=$(echo "x=$numobs;num=l(x);den=l(2);scale=0;num/den" | bc -l)
echo "Num observations: $log_num"

l=4
g=4
f=1
q=$(echo "scale=0;x=$log_num;y=$l;x-2*(y-1)" | bc -l)
echo "Memory CQF: $q"
t=1
a=0
c=0

#
# This script generate validation results for count-stretch.
#

echo "Running instantaneous throughput count stretch experiments for $numobs from $file."
echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file
echo "Count stretch 1 cone 1 thread done"
mv Instantaneous_throughput.txt Instantaneous_throughput_1C_1T.txt

t=4
f=1024
q=$(($q-10))

echo "Running instantaneous throughput count stretch experiments for $numobs from $file."
echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file
echo "Count stretch 1024 cones 4 thread done"
mv Instantaneous_throughput.txt Instantaneous_throughput_1024C_4T.txt

