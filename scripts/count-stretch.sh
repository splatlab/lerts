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
l=4
g=4

#
# This script generate validation results for count-stretch.
#

f=1
q=$(echo "scale=0;x=$log_num;y=$l;x-2*(y-1)" | bc -l)
echo "Memory CQF: $q"
t=1
a=0
c=0
stretch_out=raw/Stretch-$f-$q-$l-$g-$a-$c.data

#echo "Running count stretch validation experiments for $numobs from $file."
#echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file"
#./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file
#echo "structure,stretch" > raw/pf-$numobs
#cat $stretch_out | awk '{print "pf,"$6}' | tail -n +2 >> raw/pf-$numobs
#echo "Count stretch finished! Output in: raw/pf-$numobs"

#
# count stretch with cones and threads.
#

f=8
q=$(echo "$q-3" | bc -l)
stretch_out=raw/Stretch-$f-$q-$l-$g-$a-$c.data

#echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file"
#./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file
#echo "structure,stretch" > raw/pf-c-$numobs
#cat $stretch_out | awk '{print "pf-c,"$6}' | tail -n +2 >> raw/pf-c-$numobs
#echo "Count stretch with cones finished! Output in: raw/pf-c-$numobs"

t=8
stretch_out=raw/Stretch-$f-$q-$l-$g-$a-$c.data

echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file
echo "structure,stretch" > raw/pf-ct-$numobs
cat $stretch_out | awk '{print "pf-ct,"$6}' | tail -n +2 >> raw/pf-ct-$numobs
echo "Count stretch with cones and threads finished! Output in: raw/pf-ct-$numobs"

