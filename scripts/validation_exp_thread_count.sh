#!/bin/bash

#
# This scripts compiles the code in the validation mode and creates the @raw
# dir.
#

logdir=logs
make V=1 main; make streamdump; mkdir -p $logdir


numobs=$(($1/8))
file=raw/streamdump_mmap_active_new_$numobs

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


packets=$(($numobs/50))
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


#
# This script generate validation results for the cascade filter.
#


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

stretch_out=raw/Stretch-$f-$q-$l-$g-$a-$c-ct.data

echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -b 0 -i $file -d $stretch_out"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -b 0 -i $file -d $stretch_out
echo "structure,stretch" > raw/pf-ct-no-buffer-$numobs
cat $stretch_out | awk '{print "pf-ct,"$6}' | tail -n +2 >> raw/pf-ct-no-buffer-$numobs
echo "Count stretch with cones and threads finished! Output in: raw/pf-ct-no-buffer-$numobs"

echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -b 3 -i $file -d $stretch_out"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -b 3 -i $file -d $stretch_out
echo "structure,stretch" > raw/pf-ct-buffer-count-$numobs
cat $stretch_out | awk '{print "pf-ct,"$6}' | tail -n +2 >> raw/pf-ct-buffer-count-$numobs
echo "Count stretch with cones and threads finished! Output in: raw/pf-ct-buffer-count-$numobs"

echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file -d $stretch_out"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file -d $stretch_out
echo "structure,stretch" > raw/pf-ct-buffer-$numobs
cat $stretch_out | awk '{print "pf-ct,"$6}' | tail -n +2 >> raw/pf-ct-buffer-$numobs
echo "Count stretch with cones and threads finished! Output in: raw/pf-ct-buffer-$numobs"

#END
