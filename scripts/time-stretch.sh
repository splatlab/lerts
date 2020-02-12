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
# This script generate validation results for time-stretch.
#

f=1
q=$(echo "scale=0;x=$log_num;y=$l;x-2*(y-1)" | bc -l)
echo "Memory CQF: $q"
t=1
a=1
c=0
stretch_out=raw/Stretch-$f-$q-$l-$g-$a-$c.data

echo "Running time stretch validation experiments for $numobs from $file."

#echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file"
#./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file
#echo "structure,stretch" > raw/tf1-$numobs
#cat $stretch_out | awk '{print "tf1,"$7}' | tail -n +2 >> raw/tf1-$numobs
#echo "Time stretch with age bits 1 finished!"
#a=2
#stretch_out=raw/Stretch-$f-$q-$l-$g-$a-$c.data

#echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file"
#./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file
#echo "structure,stretch" > raw/tf2-$numobs
#cat $stretch_out | awk '{print "tf2,"$7}' | tail -n +2 >> raw/tf2-$numobs
#echo "Time stretch with age bits 2 finished!"

#a=3
#stretch_out=raw/Stretch-$f-$q-$l-$g-$a-$c.data

#echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file"
#./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file
#echo "structure,stretch" > raw/tf3-$numobs
#cat $stretch_out | awk '{print "tf3,"$7}' | tail -n +2 >> raw/tf3-$numobs
#echo "Time stretch with age bits 3 finished!"

#a=4
#stretch_out=raw/Stretch-$f-$q-$l-$g-$a-$c.data

#echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file"
#./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file
#echo "structure,stretch" > raw/tf4-$numobs
#cat $stretch_out | awk '{print "tf4,"$7}' | tail -n +2 >> raw/tf4-$numobs
#echo "Time stretch with age bits 4 finished!"


#
# time stretch with cones and threads.
#
a=1
f=8
q=$(echo "$q-3" | bc -l)

#stretch_out=raw/Stretch-$f-$q-$l-$g-$a-$c.data

#echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file"
#./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file
#echo "structure,stretch" > raw/tf1-c-$numobs
#cat $stretch_out | awk '{print "tf1-c,"$7}' | tail -n +2 >> raw/tf1-c-$numobs
#echo "Time stretch with age bits 1 and cones finished!"

t=8
stretch_out=raw/Stretch-$f-$q-$l-$g-$a-$c.data

echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file
echo "structure,stretch" > raw/tf1-ct-$numobs
cat $stretch_out | awk '{print "tf1-ct,"$7}' | tail -n +2 >> raw/tf1-ct-$numobs
echo "Time stretch with age bits 1, cones and threads finished!"

