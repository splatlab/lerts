#!/bin/bash

#
#  These test try to run against a 500M data set across several parameters
#

#stream=raw/streamdump_mmap_active_new_500M
stream=raw/streamdump_mmap_active_new_2B 

out=popcorn-output-2B-$$.txt
echo "Sending output to $out"

#RAM=134217728
#RAM=268435456
RAM=536870912


Set -x
echo  $RAM > /var/cgroups/popcorning/memory.limit_in_bytes

#------------ 512 cones
c=512
q=15


#
#  32 threads w/ more cones
t=32
command="cgexec -g memory:popcorning ./main popcornfilter -f $c -q $q -l 4 -g 4 -t $t -o -e -v 24 -i $stream"
echo $command
echo "\n Command: $command" >> $out
`$command >> $out`

#
# 16 threads
t=16
command="cgexec -g memory:popcorning ./main popcornfilter -f $c -q $q -l 4 -g 4 -t $t -o -e -v 24 -i $stream"
echo $command
echo "\n Command: $command" >> $out
`$command >> $out`

#
#  8 threads
t=8
command="cgexec -g memory:popcorning ./main popcornfilter -f $c -q $q -l 4 -g 4 -t $t -o -e -v 24 -i $stream"
echo $command
echo "\n Command: $command" >> $out
`$command >> $out`



#------------ 1024 cones
c=1024
q=14


#
#  32 threads w/ more cones
t=32
command="cgexec -g memory:popcorning ./main popcornfilter -f $c -q $q -l 4 -g 4 -t $t -o -e -v 24 -i $stream"
echo $command
echo "\n Command: $command" >> $out
`$command >> $out`

#
# 16 threads
t=16
command="cgexec -g memory:popcorning ./main popcornfilter -f $c -q $q -l 4 -g 4 -t $t -o -e -v 24 -i $stream"
echo $command
echo "\n Command: $command" >> $out
`$command >> $out`

#
#  8 threads
t=8
command="cgexec -g memory:popcorning ./main popcornfilter -f $c -q $q -l 4 -g 4 -t $t -o -e -v 24 -i $stream"
echo $command
echo "\n Command: $command" >> $out
`$command >> $out`
