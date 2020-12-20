#!/bin/bash

logdir=logs
make clean; make main; mkdir -p $logdir

numobs=$(($1/8))
file=$2/streamdump_mmap_active_new_$numobs
log_num=$(echo "x=$numobs;num=l(x);den=l(2);scale=0;num/den" | bc -l)
echo "Num observations: $log_num"

l=1
g=4
f=1
q=$log_num
echo "Memory CQF: $q"
t=1

#
# Misra-Gries 
#

out="$2/MG-throughput.data"
echo "Running MG experiments for $numobs from $file."
command="./myio.sh ./main popcornfilter -f $f -q $q -l $l -g $g -t $t -c -v 24 -e -i $file"
echo $command
echo "\n Command: $command" > $out
`$command > $out`

# Reset Cgroups
RAM=67108864
echo "\n Setting RAM size to: $RAM" 
echo  $RAM > /var/cgroups/lert/memory.limit_in_bytes

#
# Count-stretch LERT
#

l=3
q=$(echo "scale=0;x=$log_num;y=$l;x-2*(y-1)" | bc -l)
echo "Memory CQF: $q"

out="$2/CSL-throughput.data"
echo "Running CSL experiments for $numobs from $file."
command="./myio.sh cgexec -g memory:lert ./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file"
echo $command
echo "\n Command: $command" > $out
`$command > $out`

#
# Immediate-report LERT
#

out="$2/IRL-throughput.data"
echo "Running IRL experiments for $numobs from $file."
command="./myio.sh cgexec -g memory:lert ./main popcornfilter -f $f -q $q -l $l -g $g -t $t -v 24 -i $file"
echo $command
echo "\n Command: $command" > $out
`$command > $out`

#
# Time-stretch LERT
#
a=1
out="$2/TSL1-throughput.data"
echo "Running TSL1 experiments for $numobs from $file."
command="./myio.sh cgexec -g memory:lert ./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file"
echo $command
echo "\n Command: $command" > $out
`$command > $out`

a=2
out="$2/TSL2-throughput.data"
echo "Running TSL2 experiments for $numobs from $file."
command="./myio.sh cgexec -g memory:lert ./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file"
echo $command
echo "\n Command: $command" > $out
`$command > $out`

a=3
out="$2/TSL3-throughput.data"
echo "Running TSL3 experiments for $numobs from $file."
command="./myio.sh cgexec -g memory:lert ./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file"
echo $command
echo "\n Command: $command" > $out
`$command > $out`

a=4
out="$2/TSL4-throughput.data"
echo "Running TSL4 experiments for $numobs from $file."
command="./myio.sh cgexec -g memory:lert ./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file"
echo $command
echo "\n Command: $command" > $out
`$command > $out`

