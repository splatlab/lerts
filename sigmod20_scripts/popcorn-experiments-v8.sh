#!/bin/bash

#
#  These test try to run against a 2B data set across several parameters
#


out=popcorn-output-v8-$$.txt
echo "Sending output to $out"

#-----------  New  Params  ---------------
#
#   2B -- 1G RAM -- 2048 Cones -- q=15
stream=raw/streamdump_mmap_active_new_2B
RAM=1073741824
echo "\n Setting RAM size to: $RAM" >> $out
echo  $RAM > /var/cgroups/popcorning/memory.limit_in_bytes

c=2048
q=15

for t in 64; do
    command="cgexec -g memory:popcorning ./main popcornfilter -f $c -q $q -l 4 -g 4 -t $t -o -e -v 24 -i $stream"
    echo $command
    echo "\n Command: $command" >> $out
    `$command >> $out`
done



#-----------  New  Params  ---------------
#
#   2B -- 1G RAM -- 2048 Cones -- q=15
stream=raw/streamdump_mmap_active_new_2B 
RAM=536870912
echo "\n Setting RAM size to: $RAM" >> $out
echo  $RAM > /var/cgroups/popcorning/memory.limit_in_bytes

c=2048
q=15

for t in 64; do
    command="cgexec -g memory:popcorning ./main popcornfilter -f $c -q $q -l 4 -g 4 -t $t -o -e -v 24 -i $stream"
    echo $command
    echo "\n Command: $command" >> $out
    `$command >> $out`
done

