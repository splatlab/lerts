#!/bin/bash

#
#  This script generates data from Firehose active generator and dumps that
#  into a file.
#
#  **The input to this script is the size of the final output file.**
#

numobs=$(($1/8))
packets=$(($numobs/50))
file=raw/streamdump_mmap_active_new_$numobs
echo "Generating $numobs observation from Firehose active (packets: $packets) and dumping into file $file"

./streamdump -s $numobs -f $file 12345 & \
./firehose/generators/active/active -n $packets -r 500000 -a 1048576 127.0.0.1@12345


