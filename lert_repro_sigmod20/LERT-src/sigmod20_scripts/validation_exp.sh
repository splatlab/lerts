#!/bin/bash

logdir=logs
make clean; make V=1 main; mkdir -p $logdir

numobs=$(($1/8))
file=$2/streamdump_mmap_active_new_$numobs
log_num=$(echo "x=$numobs;num=l(x);den=l(2);scale=0;num/den" | bc -l)
echo "Num observations: $log_num"

l=4
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
stretch_out=$2/Stretch-$f-$q-$l-$g-$a-$c-$t.data

echo "Running count stretch validation experiments for $numobs from $file."
echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -c -o -v 24 -i $file -d $stretch_out"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -c -o -v 24 -i $file -d $stretch_out
echo "structure,stretch" > $2/cf-count-$numobs
echo "structure,stretch" > $2/cf-time-$numobs
cat $stretch_out | awk '{print "cf,"$6}' | tail -n +2 >> $2/cf-count-$numobs
cat $stretch_out | awk '{print "cf,"$7}' | tail -n +2 >> $2/cf-time-$numobs
echo "Count/Time stretch finished for cascade filter! Output in: $2/cf-$numobs"

#
# This script generate validation results for count-stretch.
#

c=0
stretch_out=$2/Stretch-$f-$q-$l-$g-$a-$c-$t.data

echo "Running count stretch validation experiments for $numobs from $file."
echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file -d $stretch_out"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file -d $stretch_out
echo "structure,stretch" > $2/pf-$numobs
cat $stretch_out | awk '{print "pf,"$6}' | tail -n +2 >> $2/pf-$numobs
echo "Count stretch finished! Output in: $2/pf-$numobs"

#
# count stretch with cones and threads.
#

f=8
q=$(echo "$q-3" | bc -l)
stretch_out=$2/Stretch-$f-$q-$l-$g-$a-$c-$t.data

echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file -d $stretch_out"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file -d $stretch_out
echo "structure,stretch" > $2/pf-c-$numobs
cat $stretch_out | awk '{print "pf-c,"$6}' | tail -n +2 >> $2/pf-c-$numobs
echo "Count stretch with cones finished! Output in: $2/pf-c-$numobs"

t=8
stretch_out=$2/Stretch-$f-$q-$l-$g-$a-$c-$t.data

echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file -d $stretch_out"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file -d $stretch_out
echo "structure,stretch" > $2/pf-ct-$numobs
cat $stretch_out | awk '{print "pf-ct,"$6}' | tail -n +2 >> $2/pf-ct-$numobs
echo "Count stretch with cones and threads finished! Output in: $2/pf-ct-$numobs"


#
# This script generate validation results for time-stretch.
#

f=1
q=$(echo "scale=0;x=$log_num;y=$l;x-2*(y-1)" | bc -l)
echo "Memory CQF: $q"
t=1
a=1
c=0
stretch_out=$2/Stretch-$f-$q-$l-$g-$a-$c-$t.data

echo "Running time stretch validation experiments for $numobs from $file."
echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file -d $stretch_out"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file -d $stretch_out
echo "structure,stretch" > $2/tf1-$numobs
cat $stretch_out | awk '{print "tf1,"$7}' | tail -n +2 >> $2/tf1-$numobs
echo "Time stretch with age bits 1 finished!"

a=2
stretch_out=$2/Stretch-$f-$q-$l-$g-$a-$c-$t.data

echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file -d $stretch_out"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file -d $stretch_out
echo "structure,stretch" > $2/tf2-$numobs
cat $stretch_out | awk '{print "tf2,"$7}' | tail -n +2 >> $2/tf2-$numobs
echo "Time stretch with age bits 2 finished!"

a=3
stretch_out=$2/Stretch-$f-$q-$l-$g-$a-$c-$t.data

echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file -d $stretch_out"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file -d $stretch_out
echo "structure,stretch" > $2/tf3-$numobs
cat $stretch_out | awk '{print "tf3,"$7}' | tail -n +2 >> $2/tf3-$numobs
echo "Time stretch with age bits 3 finished!"

a=4
stretch_out=$2/Stretch-$f-$q-$l-$g-$a-$c-$t.data

echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file -d $stretch_out"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file -d $stretch_out
echo "structure,stretch" > $2/tf4-$numobs
cat $stretch_out | awk '{print "tf4,"$7}' | tail -n +2 >> $2/tf4-$numobs
echo "Time stretch with age bits 4 finished!"

#
# time stretch with cones and threads.
#
a=1
f=8
q=$(echo "$q-3" | bc -l)
stretch_out=$2/Stretch-$f-$q-$l-$g-$a-$c-$t.data

echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file -d $stretch_out"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file -d $stretch_out
echo "structure,stretch" > $2/tf1-c-$numobs
cat $stretch_out | awk '{print "tf1-c,"$7}' | tail -n +2 >> $2/tf1-c-$numobs
echo "Time stretch with age bits 1 and cones finished!"

t=8
stretch_out=$2/Stretch-$f-$q-$l-$g-$a-$c-$t.data

echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file -d $stretch_out"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file -d $stretch_out
echo "structure,stretch" > $2/tf1-ct-$numobs
cat $stretch_out | awk '{print "tf1-ct,"$7}' | tail -n +2 >> $2/tf1-ct-$numobs
echo "Time stretch with age bits 1, cones and threads finished!"

