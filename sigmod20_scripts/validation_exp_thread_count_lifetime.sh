#!/bin/bash

logdir=logs
make clean; make V=1 main; mkdir -p $logdir


numobs=$(($1/8))
file=$2/streamdump_mmap_active_new_$numobs

log_num=$(echo "x=$numobs;num=l(x);den=l(2);scale=0;num/den" | bc -l)
echo "Num observations: $log_num"

#
# count stretch with cones and threads.
#

l=4
g=4
f=1
q=$(echo "scale=0;x=$log_num;y=$l;x-2*(y-1)" | bc -l)
echo "Memory CQF: $q"
a=0
c=0

f=8
q=$(echo "$q-3" | bc -l)
t=8

stretch_out=$2/Stretch-$f-$q-$l-$g-$a-$c-$t-ct.data

echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file -d $stretch_out"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file -d $stretch_out
cat $stretch_out | awk '{print $4","$6}' | tail -n +2 >> $2/pf-ct-buffer-lt-$numobs
echo "Count stretch with cones and threads finished! Output in: $2/pf-ct-buffer-lt-$numobs"

a=1
stretch_out=$2/Stretch-$f-$q-$l-$g-$a-$c-$t-ct.data
echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file -d $stretch_out"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file -d $stretch_out
cat $stretch_out | awk '{print $4","$7}' | tail -n +2 >> $2/tf-ct-buffer-lt-$numobs
echo "Time stretch with cones and threads finished! Output in: $2/tf-ct-buffer-lt-$numobs"
