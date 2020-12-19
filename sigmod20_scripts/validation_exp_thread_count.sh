#!/bin/bash

make clean; make V=1 main;


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

stretch_out=$2/Stretch-$f-$q-$l-$g-$a-$c-ct.data

echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -b 0 -i $file -d $stretch_out"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -b 0 -i $file -d $stretch_out
echo "structure,stretch" > $2/pf-ct-no-buffer-$numobs
cat $stretch_out | awk '{print "pf-ct,"$6}' | tail -n +2 >> $2/pf-ct-no-buffer-$numobs
echo "Count stretch with cones and threads finished! Output in: $2/pf-ct-no-buffer-$numobs"

echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -b 3 -i $file -d $stretch_out"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -b 3 -i $file -d $stretch_out
echo "structure,stretch" > $2/pf-ct-buffer-count-$numobs
cat $stretch_out | awk '{print "pf-ct,"$6}' | tail -n +2 >> $2/pf-ct-buffer-count-$numobs
echo "Count stretch with cones and threads finished! Output in: $2/pf-ct-buffer-count-$numobs"

echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file -d $stretch_out"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file -d $stretch_out
echo "structure,stretch" > $2/pf-ct-buffer-$numobs
cat $stretch_out | awk '{print "pf-ct,"$6}' | tail -n +2 >> $2/pf-ct-buffer-$numobs
echo "Count stretch with cones and threads finished! Output in: $2/pf-ct-buffer-$numobs"

