#!/bin/bash

numobs=$(($1/8))
file=raw/streamdump_mmap_active_new_$numobs

log_num=$(echo "x=$numobs;num=l(x);den=l(2);scale=0;num/den" | bc -l)
echo "Num observations: $log_num"

#l=$(echo "scale=0;x=$log_num;((x-22)/2 + 1)" | bc -l)
#echo $l
#if [ $l -lt 3 ]; then 
  #l=3;
#fi
l=3
g=4


#
# This script generate validation results for the cascade filter.
#

f=1
q=$(echo "scale=0;x=$log_num;y=$l;x-2*(y-1)" | bc -l)
echo "Memory CQF: $q"
t=1
a=0
c=1
stretch_out=raw/Stretch-$f-$q-$l-$g-$a-$c.data

echo "Running count stretch validation experiments for $numobs from $file."
echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -c -o -v 24 -i $file"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -c -o -v 24 -i $file
echo "structure,stretch" > raw/cf-count-$numobs
echo "structure,stretch" > raw/cf-time-$numobs
cat $stretch_out | awk '{print "cf,"$6}' | tail -n +2 >> raw/cf-count-$numobs
cat $stretch_out | awk '{print "cf,"$7}' | tail -n +2 >> raw/cf-time-$numobs
echo "Count/Time stretch finished for cascade filter! Output in: raw/cf-$numobs"

