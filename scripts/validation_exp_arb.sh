#!/bin/bash

#
# This scripts compiles the code in the validation mode and creates the @raw
# dir.
#

logdir=logs
make clean; make V=1 main; make generate_stream; mkdir -p $logdir

#
#  **The input to this script is the size of the final output file.**
#

numobs=$(($1/8))

log_num=$(echo "x=$numobs;num=l(x);den=l(2);scale=0;num/den" | bc -l)
echo "Num observations: $log_num"
#l=$(echo "scale=0;x=$log_num;((x-22)/2 + 1)" | bc -l)
#echo $l
#if [ $l -lt 3 ]; then 
  #l=3;
#fi
l=4
g=4
q=$(echo "scale=0;x=$log_num;y=$l;x-2*(y-1)" | bc -l)
echo "Memory CQF: $q"

file=raw/streamdump_mmap_arb_new_$numobs
echo "Generating $numobs observation from random generator and dumping into file $file"
./generate_stream $log_num $q $file

#
# This script generate validation results for the cascade filter.
#

f=1
q=$(echo "scale=0;x=$log_num;y=$l;x-2*(y-1)" | bc -l)
echo "Memory CQF: $q"
t=1
a=0
c=1

#stretch_out=raw/Stretch-$f-$q-$l-$g-$a-$c.data

#echo "Running count stretch validation experiments for $numobs from $file."
#echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -c -o -v 24 -i $file"
#./main popcornfilter -f $f -q $q -l $l -g $g -t $t -c -o -v 24 -i $file
#echo "structure,stretch" > raw/cf-arb-count-$numobs
#echo "structure,stretch" > raw/cf-arb-time-$numobs
#cat $stretch_out | awk '{print "cf,"$6}' | tail -n +2 >> raw/cf-arb-count-$numobs
#cat $stretch_out | awk '{print "cf,"$7}' | tail -n +2 >> raw/cf-arb-time-$numobs
#echo "Count/Time stretch finished for cascade filter! Output in: raw/cf-arb-$numobs"

#
# This script generate validation results for count-stretch.
#

c=0
stretch_out=raw/Stretch-$f-$q-$l-$g-$a-$c.data

echo "Running count stretch validation experiments for $numobs from $file."
echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -o -v 24 -i $file
echo "structure,stretch" > raw/pf-arb-$numobs
cat $stretch_out | awk '{print "pf,"$6}' | tail -n +2 >> raw/pf-arb-$numobs
echo "Count stretch finished! Output in: raw/pf-arb-$numobs"

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
echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file"
./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file
echo "structure,stretch" > raw/tf1-arb-$numobs
cat $stretch_out | awk '{print "tf1,"$7}' | tail -n +2 >> raw/tf1-arb-$numobs
echo "Time stretch with age bits 1 finished!"


#END
