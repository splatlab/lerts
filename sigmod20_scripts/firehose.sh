#!/bin/bash

make clean; make streamdump;

#
# setup firehose
#

#git clone https://github.com/splatlab/firehose.git
cd firehose/generators/active/; make; cd -

#
#  This script generates data from Firehose active generator and dumps that
#  into a file.
#
#  **The input to this script is the size of the final output file.**
#

numobs=$(($1/8))
packets=$(($numobs/50))
file=$2/streamdump_mmap_active_new_$numobs
echo "Generating $numobs observation from Firehose active (packets: $packets, active set: 1048576) and dumping into file $file"

./streamdump -s $numobs -f $file 12345 & \
./firehose/generators/active/active -n $packets -r 500000 -a 1048576 127.0.0.1@12345

log_num=$(echo "x=$numobs;num=l(x);den=l(2);scale=0;num/den" | bc -l)
echo "Num observations: $log_num"

