#!/bin/bash

#
# This scripts compiles the code in the validation mode and creates the @raw
# dir.
#

make clean; make generate_stream;

#
#  **The input to this script is the size of the final output file.**
#

numobs=$(($1/8))

log_num=$(echo "x=$numobs;num=l(x);den=l(2);scale=0;num/den" | bc -l)
echo "Num observations: $log_num"

l=4
for i in {1..4}; do
	q=$(echo "scale=0;x=$log_num;y=$l;x-2*(y-1)" | bc -l)
	echo "Memory CQF: $q"
	file=$2/streamdump_mmap_arb_new_$numobs\_$i;
	echo "Generating $numobs observation from random generator and dumping into file $file"
	./generate_stream $log_num $q $i $file
done

