#!/bin/bash

#
#  These test try to run against a 2B data set across several parameters
#

logdir=logs
make clean; make main; make -p $logdir

numobs=$(($1/8))
file=$2/streamdump_mmap_active_new_$numobs

echo "Sending output to $out"

#-----------  New  Params  ---------------
#
#   4B -- 2G RAM -- 2048 Cones -- q=15
RAM=2147483648
echo "\n Setting RAM size to: $RAM" >> $out
echo  $RAM > /var/cgroups/lert/memory.limit_in_bytes

c=2048
q=15

for t in 1 2 4 8 16 32 64; do
    out=$2/popcorn-output-v8-2G-$t.output
    command="cgexec -g memory:lert ./main popcornfilter -f $c -q $q -l 4 -g 4 -t $t -o -e -v 24 -i $stream"
    echo $command
    echo "\n Command: $command" > $out
    `$command > $out`
done

#-----------  New  Params  ---------------
#
#   4B -- 1G RAM -- 2048 Cones -- q=15
RAM=1073741824
echo "\n Setting RAM size to: $RAM" >> $out
echo  $RAM > /var/cgroups/lert/memory.limit_in_bytes

for t in 1 2 4 8 16 32 64; do
    out=$2/popcorn-output-v8-1G-$t.output
    command="cgexec -g memory:lert ./main popcornfilter -f $c -q $q -l 4 -g 4 -t $t -o -e -v 24 -i $stream"
    echo $command
    echo "\n Command: $command" > $out
    `$command > $out`
done

#-----------  New  Params  ---------------
#
#   4B -- 512M RAM -- 2048 Cones -- q=15
RAM=536870912
echo "\n Setting RAM size to: $RAM" >> $out
echo  $RAM > /var/cgroups/lert/memory.limit_in_bytes

for t in 1 2 4 8 16 32 64; do
    out=$2/popcorn-output-v8-512M-$t.output
    command="cgexec -g memory:lert ./main popcornfilter -f $c -q $q -l 4 -g 4 -t $t -o -e -v 24 -i $stream"
    echo $command
    echo "\n Command: $command" > $out
    `$command > $out`
done

