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

