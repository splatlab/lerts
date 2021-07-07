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
q=$(echo "scale=0;x=$log_num;y=$l;x-2*(y-1)" | bc -l)
echo "Memory CQF: $q"
a=0
c=0

f=8
q=$(echo "$q-3" | bc -l)
t=8


b=0
stretch_out=$2/Stretch-$f-$q-$l-$g-$a-$c-$t-$b-ct.data
logfile=$2/csl_no_buffer.output

echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -b 0 -i $file -d $stretch_out"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -b $b -i $file -d $stretch_out > $logfile
echo "structure,stretch" > $2/pf-ct-no-buffer-$numobs
cat $stretch_out | awk '{print "pf-ct"$b","$6}' | tail -n +2 >> $2/pf-ct-no-buffer-$numobs
echo "Count stretch with cones and threads finished! Output in: $2/pf-ct-no-buffer-$numobs"

b=3
stretch_out=$2/Stretch-$f-$q-$l-$g-$a-$c-$t-$b-ct.data
logfile=$2/csl_buffer_count.output
echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -b 3 -i $file -d $stretch_out"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -b $b -i $file -d $stretch_out > $logfile
echo "structure,stretch" > $2/pf-ct-buffer-count-$numobs
cat $stretch_out | awk '{print "pf-ct"$b","$6}' | tail -n +2 >> $2/pf-ct-buffer-count-$numobs
echo "Count stretch with cones and threads finished! Output in: $2/pf-ct-buffer-count-$numobs"

stretch_out=$2/Stretch-$f-$q-$l-$g-$a-$c-$t-ct.data
logfile=$2/csl_buffer.output
echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file -d $stretch_out"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file -d $stretch_out > $logfile
echo "structure,stretch" > $2/pf-ct-buffer-$numobs
cat $stretch_out | awk '{print "pf-ct,"$6}' | tail -n +2 >> $2/pf-ct-buffer-$numobs
echo "Count stretch with cones and threads finished! Output in: $2/pf-ct-buffer-$numobs"

