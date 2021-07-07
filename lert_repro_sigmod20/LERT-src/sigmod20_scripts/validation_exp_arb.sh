#!/bin/bash

logdir=logs
make clean; make V=1 main; mkdir -p $logdir

numobs=$(($1/8))
log_num=$(echo "x=$numobs;num=l(x);den=l(2);scale=0;num/den" | bc -l)
echo "Num observations: $log_num"
l=4
g=4
f=1
q=$(echo "scale=0;x=$log_num;y=$l;x-2*(y-1)" | bc -l)
echo "Memory CQF: $q"
t=1
a=1
c=0

for i in {1..4}; do
	stretch_out=$2/Stretch-$f-$q-$l-$g-$a-$c-$t-arb$i.data
  file=$2/streamdump_mmap_arb_new_$numobs\_$i
	echo "Running time stretch validation experiments for $numobs from $file."
	echo "./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file -d $stretch_out"
	./main popcornfilter -f $f -q $q -l $l -g $g -t $t -a $a -o -v 24 -i $file -d $stretch_out
	echo "structure,stretch" > $2/tf1-arb-$numobs-$i
	cat $stretch_out | awk -v arb=arb$i '{print arb","$7}' | tail -n +2 >> $2/tf1-arb-$numobs-$i
	echo "Time stretch with age bits 1 finished!"
	echo "Time stretch finished! Output in: $2/tf1-arb-$numobs-$i"
done
