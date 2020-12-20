#!/bin/bash

t=$(grep "Insertion throughput" ../count.log | cut -d " " -f 3 | sed -E \
's/([+-]?[0-9.]+)[eE]\+?(-?)([0-9]+)/(\1*10^\2\3)/g')

tm=$(echo "x=$t;x/10^6" | bc -l)
echo $tm

r16=$tm

echo $r16
