#!/bin/sh
set -x
echo 134217728 > /var/cgroups/popcorning/memory.limit_in_bytes
cgexec -g memory:popcorning ./main popcornfilter -f 256 -q 16 -l 4 -g 4 -t 1 -o -e -v 24 -i raw/streamdump_mmap_active_new_500M
cgexec -g memory:popcorning ./main popcornfilter -f 256 -q 16 -l 4 -g 4 -t 2 -o -e -v 24 -i raw/streamdump_mmap_active_new_500M
cgexec -g memory:popcorning ./main popcornfilter -f 256 -q 16 -l 4 -g 4 -t 3 -o -e -v 24 -i raw/streamdump_mmap_active_new_500M
cgexec -g memory:popcorning ./main popcornfilter -f 256 -q 16 -l 4 -g 4 -t 4 -o -e -v 24 -i raw/streamdump_mmap_active_new_500M
cgexec -g memory:popcorning ./main popcornfilter -f 256 -q 16 -l 4 -g 4 -t 8 -o -e -v 24 -i raw/streamdump_mmap_active_new_500M
cgexec -g memory:popcorning ./main popcornfilter -f 256 -q 16 -l 4 -g 4 -t 16 -o -e -v 24 -i raw/streamdump_mmap_active_new_500M


