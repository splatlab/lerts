#!/bin/bash

#
# This scripts compiles the code in the validation mode and creates the @raw
# dir.
#

logdir=logs
make clean; make V=1 main; make streamdump; mkdir -p $logdir

#
# setup firehose
#

git clone https://github.com/splatlab/firehose.git
cd firehose/generators/active/; make; cd -

#
#  This script generates data from Firehose active generator and dumps that
#  into a file.
#
#  **The input to this script is the size of the final output file.**
#

numobs=$(($1/8))
packets=$(($numobs/50))
file=raw/streamdump_mmap_active_new_$numobs
echo "Generating $numobs observation from Firehose active (packets: $packets, active set: 1048576) and dumping into file $file"

./streamdump -s $numobs -f $file 12345 & \
./firehose/generators/active/active -n $packets -r 500000 -a 1048576 127.0.0.1@12345

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
# This script generate validation results for the cascade filter.
#

f=1
q=$(echo "scale=0;x=$log_num;y=$l;x-2*(y-1)" | bc -l)
echo "Memory CQF: $q"
t=1
a=0
c=1
stretch_out=raw/Stretch-$f-$q-$l-$g-$a-$c.data

#: <<'END'

echo "Running count stretch validation experiments for $numobs from $file."
echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -c -o -v 24 -i $file"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -c -o -v 24 -i $file
echo "structure,stretch" > raw/cf-count-$numobs
echo "structure,stretch" > raw/cf-count-$numobs
cat $stretch_out | awk '{print "cf,"$6}' | tail -n +2 >> raw/cf-count-$numobs
cat $stretch_out | awk '{print "cf,"$7}' | tail -n +2 >> raw/cf-time-$numobs
echo "Count stretch finished! Output in: raw/cf-$numobs"

#
# This script generate validation results for count-stretch.
#

c=0
stretch_out=raw/Stretch-$f-$q-$l-$g-$a-$c.data

echo "Running count stretch validation experiments for $numobs from $file."
echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file
echo "structure,stretch" > raw/pf-$numobs
cat $stretch_out | awk '{print "pf,"$6}' | tail -n +2 >> raw/pf-$numobs
echo "Count stretch finished! Output in: raw/pf-$numobs"

#
# count stretch with cones and threads.
#

f=8
q=$q-3
stretch_out=raw/Stretch-$f-$q-$l-$g-$a-$c.data

echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file
echo "structure,stretch" > raw/pf-c-$numobs
cat $stretch_out | awk '{print "pf-c,"$6}' | tail -n +2 >> raw/pf-c-$numobs
echo "Count stretch with cones finished! Output in: raw/pf-c-$numobs"

t=8
stretch_out=raw/Stretch-$f-$q-$l-$g-$a-$c.data

echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file
echo "structure,stretch" > raw/pf-ct-$numobs
cat $stretch_out | awk '{print "pf-ct,"$6}' | tail -n +2 >> raw/pf-ct-$numobs
echo "Count stretch with cones and threads finished! Output in: raw/pf-ct-$numobs"


#
# This script generate validation results for time-stretch.
#

f=1
q=$(echo "x=$log_num;y=$l;x-2(y-1)" | bc -l)
t=1
a=1
c=0
stretch_out=raw/Stretch-$f-$q-$l-$g-$a-$c.data

echo "Running time stretch validation experiments for $numobs from $file."
echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file
echo "structure,stretch" > raw/tf1-$numobs
cat $stretch_out | awk '{print "tf1,"$7}' | tail -n +2 >> raw/tf1-$numobs
echo "Time stretch with age bits 1 finished!"

a=2
stretch_out=raw/Stretch-$f-$q-$l-$g-$a-$c.data

echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file
echo "structure,stretch" > raw/tf2-$numobs
cat $stretch_out | awk '{print "tf2,"$7}' | tail -n +2 >> raw/tf2-$numobs
echo "Time stretch with age bits 2 finished!"

a=3
stretch_out=raw/Stretch-$f-$q-$l-$g-$a-$c.data

echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file
echo "structure,stretch" > raw/tf3-$numobs
cat $stretch_out | awk '{print "tf3,"$7}' | tail -n +2 >> raw/tf3-$numobs
echo "Time stretch with age bits 3 finished!"

a=4
stretch_out=raw/Stretch-$f-$q-$l-$g-$a-$c.data

echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file
echo "structure,stretch" > raw/tf4-$numobs
cat $stretch_out | awk '{print "tf4,"$7}' | tail -n +2 >> raw/tf4-$numobs
echo "Time stretch with age bits 4 finished!"

#
# time stretch with cones and threads.
#
a=1
f=8
q=$q-3
stretch_out=raw/Stretch-$f-$q-$l-$g-$a-$c.data

echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file
echo "structure,stretch" > raw/tf1-c-$numobs
cat $stretch_out | awk '{print "tf1-c,"$7}' | tail -n +2 >> raw/tf1-c-$numobs
echo "Time stretch with age bits 1 and cones finished!"

t=8
stretch_out=raw/Stretch-$f-$q-$l-$g-$a-$c.data

echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file
echo "structure,stretch" > raw/tf1-ct-$numobs
cat $stretch_out | awk '{print "tf1-ct,"$7}' | tail -n +2 >> raw/tf1-ct-$numobs
echo "Time stretch with age bits 1, cones and threads finished!"

#END
