#!/bin/bash

#
# function definitions
#

#
# Data generation code
#
# Expected files after this function:
# sigmod20_raw/streamdump_mmap_active_new_67108864
# sigmod20_raw/streamdump_mmap_active_new_536870912
# sigmod20_raw/streamdump_mmap_active_new_4294967296
# sigmod20_raw/streamdump_mmap_arb_new_1
# sigmod20_raw/streamdump_mmap_arb_new_2
# sigmod20_raw/streamdump_mmap_arb_new_3
# sigmod20_raw/streamdump_mmap_arb_new_4
generate_data() {
  # 2^26 obs
  datasize=536870912
  ./$scriptdir/firehose.sh $datasize $rawdir >> $logfile

  # Arbitrary obs
  # 2^29 obs
  datasize=4294967296
  ./$scriptdir/arb_gen_data.sh $datasize $rawdir >> $logfile

  # 2^29 obs
  datasize=4294967296
  ./$scriptdir/firehose.sh $datasize $rawdir >> $logfile

  # 2^32 obs
  datasize=34359738368
  ./$scriptdir/firehose-gen-data.sh $datasize >> $logfile
}

#############################################################

#
# Validation experiments for Cascade filter, time-stretch and count-stretch LERT
#

# Expected files after this function:
# Stretch-1-20-4-4-0-1-1.data 
# Stretch-1-20-4-4-0-0-1.data
# Stretch-8-17-4-4-0-0-1.data
# Stretch-8-17-4-4-0-0-8.data
# Stretch-1-20-4-4-1-0-1.data
# Stretch-1-20-4-4-2-0-1.data
# Stretch-1-20-4-4-3-0-1.data
# Stretch-1-20-4-4-4-0-1.data
# Stretch-8-17-4-4-1-0-1.data
# Stretch-8-17-4-4-1-0-8.data
# cf-count-67108864
# cf-time-67108864
# pf-67108864 
# pf-c-67108864 
# pf-ct-67108864
# tf1-67108864
# tf2-67108864
# tf3-67108864
# tf4-67108864
# tf1-c-67108864
# tf1-ct-67108864
validate_stretch () {
  # 2^29 obs
  datasize=4294967296
  ./$scriptdir/validation_exp.sh $datasize $rawdir >> $logfile
}

#############################################################

#
# Validation experiments for time-stretch LERT for arbitrary datasets
#

# Expected files after this function:
# Stretch-1-20-4-4-1-0-1-arb1.data
# Stretch-1-20-4-4-1-0-1-arb2.data
# Stretch-1-20-4-4-1-0-1-arb3.data
# Stretch-1-20-4-4-1-0-1-arb4.data
# tf1-arb-67108864-1
# tf1-arb-67108864-2
# tf1-arb-67108864-3
# tf1-arb-67108864-4
validate_arb_stretch() {
  # 2^29 obs
  datasize=4294967296
  ./$scriptdir/validation_exp_arb.sh $datasize $rawdir >> $logfile
}

#############################################################

#
# Validation experiments for count-stretch LERT with 8 threads/cones for
# different buffering approaches
#

# Expected files after this function:
# Stretch-8-17-4-4-0-0-8-0-ct.data
# Stretch-8-17-4-4-0-0-8-3-ct.data 
# Stretch-8-17-4-4-1-0-8-ct.data 
# pf-ct-buffer-67108864
# pf-ct-buffer-count-67108864
# pf-ct-no-buffer-67108864
# csl_buffer.output
# csl_buffer_count.output
# csl_no_buffer.output
validate_buffer_stretch() {
  # 2^29 obs
  datasize=4294967296
  ./$scriptdir/validation_exp_thread_count.sh $datasize $rawdir >> $logfile
}

#############################################################

#
# Plot lifetime-vs  count-  and time-stretch  8 threads/cones 
#

# Expected files after this function:
# Stretch-8-17-4-4-0-0-8-ct.data
# Stretch-8-17-4-4-1-0-8-ct.data 
# pf-ct-buffer-lt-67108864
# tf-ct-buffer-lt-67108864 
validate_lifetime_stretch() {
  # 2^29 obs
  datasize=4294967296
  ./$scriptdir/validation_exp_thread_count_lifetime.sh $datasize $rawdir >> $logfile
}

#############################################################

#
# Throughput experiments with a single thread 
#

# Expected files after this function:
# MG-throughput.data
# CSL-throughput.data
# IRL-throughput.data
# TSL1-throughput.data
# TSL2-throughput.data
# TSL3-throughput.data
# TSL4-throughput.data
throughput_exp () {
  # 2^26 obs
  datasize=536870912
  ./$scriptdir/throughput_exp.sh $datasize $rawdir >> $logfile
}

#############################################################

#
# Scalability throughput experiments 
#

# Expected files after this function:
# popcorn-output-v8-2G-$t.output
# popcorn-output-v8-1G-$t.output
# popcorn-output-v8-512M-$t.output
# for $t in 1 2 4 8 16 32 64
scalability_exp () {
  # 2^32 obs
  datasize=34359738368
  ./$scriptdir/popcorn-experiments-v8.sh $datasize $rawdir >> $logfile
}

#############################################################

#
# Instaneous throughput experiments 
#

# Expected files after this function:
# Instantaneous_throughput_1C_1T.txt
# Instantaneous_throughput_1024C_4T.txt
instantaneous_throughput () {
  # 2^29 obs
  datasize=536870912
  #datasize=4294967296
  ./$scriptdir/instantaneous_throughput.sh $datasize $rawdir >> $logfile
}

#############################################################

#
# Plot stretch distributions
#

plot_stretch () {
  # 2^29 obs
  datasize=536870912 # delete after testing
  #datasize=4294967296
  numobs=$((datasize/8))

  ./$scriptdir/boxplot_cs_dist.py $rawdir/cf-count-$numobs $rawdir/pf-$numobs \
    $rawdir/pf-c-$numobs $rawdir/pf-ct-$numobs $figdir/countstretch-dist.png

  ./$scriptdir/boxplot_ts_dist.py $rawdir/cf-time-$numobs $rawdir/tf1-$numobs \
    $rawdir/tf1-c-$numobs $rawdir/tf1-ct-$numobs $figdir/timestretch-dist.png

  ./$scriptdir/boxplot_ts_a_dist.py $rawdir/tf1-$numobs $rawdir/tf2-$numobs \
    $rawdir/tf3-$numobs $rawdir/tf4-$numobs $figdir/timestretch-a-dist.png

  ./$scriptdir/boxplot_ts_arb_dist.py $rawdir/tf1-arb-$numobs-1 \
    $rawdir/tf1-arb-$numobs-2 $rawdir/tf1-arb-$numobs-3 $rawdir/tf1-arb-$numobs-4 \
    $figdir/timestretch-arb-dist.png
}


#
# Retrieve throughput for buffer experiments
#

plot_buffer () {
  # 2^29 obs
  datasize=4294967296
  numobs=$((datasize/8))
  buffer=$(grep "Insertion throughput" $rawdir/csl_buffer.output | cut -d " " -f 3 | sed -E 's/([+-]?[0-9.]+)[eE]\+?(-?)([0-9]+)/(\1*10^\2\3)/g')
  buffer_tp=$(echo "x=$buffer;x/10^6" | bc -l)

  buffer_count=$(grep "Insertion throughput" $rawdir/csl_buffer_count.output | cut -d " " -f 3 | sed -E 's/([+-]?[0-9.]+)[eE]\+?(-?)([0-9]+)/(\1*10^\2\3)/g')
  buffer_count_tp=$(echo "x=$buffer_count;x/10^6" | bc -l)

  buffer_no=$(grep "Insertion throughput" $rawdir/csl_no_buffer.output | cut -d " " -f 3 | sed -E 's/([+-]?[0-9.]+)[eE]\+?(-?)([0-9]+)/(\1*10^\2\3)/g')
  buffer_no_tp=$(echo "x=$buffer_no;x/10^6" | bc -l)

  ./$scriptdir/boxplot_cs_buffer_dist.py $rawdir/pf-ct-buffer-$numobs \
    $rawdir/pf-ct-buffer-count-$numobs $rawdir/pf-ct-no-buffer-$numobs $buffer_tp \
    $buffer_count_tp $buffer_no_tp $figdir/countstretch-buffer-dist.png

  ./$scriptdir/plot_cs.py $rawdir/pf-ct-buffer-lt-$numobs \
    $figdir/countstretch-lifetime.png 

  ./$scriptdir/plot_ts.py $rawdir/tf-ct-buffer-lt-$numobs \
    $figdir/timestretch-lifetime.png
}

#
# Retrieve throughput for throughput experiments 
#

plot_throughput () {
  throughputfile=$rawdir/throughput.output

  mg=$(grep "Insertion throughput" $rawdir/MG-throughput.data | cut -d " " -f 3 | sed -E 's/([+-]?[0-9.]+)[eE]\+?(-?)([0-9]+)/(\1*10^\2\3)/g')
  mg_tp=$(echo "x=$mg;x/10^6" | bc -l)
  echo "MG $mg_tp" > $throughputfile

  csl=$(grep "Insertion throughput" $rawdir/CSL-throughput.data | cut -d " " -f 3 | sed -E 's/([+-]?[0-9.]+)[eE]\+?(-?)([0-9]+)/(\1*10^\2\3)/g')
  csl_tp=$(echo "x=$csl;x/10^6" | bc -l)
  echo "CSL $csl_tp" >> $throughputfile

  irl=$(grep "Insertion throughput" $rawdir/IRL-throughput.data | cut -d " " -f 3 | sed -E 's/([+-]?[0-9.]+)[eE]\+?(-?)([0-9]+)/(\1*10^\2\3)/g')
  irl_tp=$(echo "x=$irl;x/10^6" | bc -l)
  echo "IRL $irl_tp" >> $throughputfile

  tsl1=$(grep "Insertion throughput" $rawdir/TSL1-throughput.data | cut -d " " -f 3 | sed -E 's/([+-]?[0-9.]+)[eE]\+?(-?)([0-9]+)/(\1*10^\2\3)/g')
  tsl1_tp=$(echo "x=$tsl1;x/10^6" | bc -l)
  echo "TSL1 $tsl1_tp" >> $throughputfile

  tsl2=$(grep "Insertion throughput" $rawdir/TSL2-throughput.data | cut -d " " -f 3 | sed -E 's/([+-]?[0-9.]+)[eE]\+?(-?)([0-9]+)/(\1*10^\2\3)/g')
  tsl2_tp=$(echo "x=$tsl2;x/10^6" | bc -l)
  echo "TSL2 $tsl2_tp" >> $throughputfile

  tsl3=$(grep "Insertion throughput" $rawdir/TSL3-throughput.data | cut -d " " -f 3 | sed -E 's/([+-]?[0-9.]+)[eE]\+?(-?)([0-9]+)/(\1*10^\2\3)/g')
  tsl3_tp=$(echo "x=$tsl3;x/10^6" | bc -l)
  echo "TSL3 $tsl3_tp" >> $throughputfile

  tsl4=$(grep "Insertion throughput" $rawdir/TSL4-throughput.data | cut -d " " -f 3 | sed -E 's/([+-]?[0-9.]+)[eE]\+?(-?)([0-9]+)/(\1*10^\2\3)/g')
  tsl4_tp=$(echo "x=$tsl4;x/10^6" | bc -l)
  echo "TSL4 $tsl4_tp" >> $throughputfile

  ./$scriptdir/barplot_throughput.py $mg_tp $csl_tp $irl_tp $tsl1_tp $tsl2_tp $tsl3_tp $tsl4_tp $figdir/throughput.png
}

plot_io () {
  # 2^26 obs
  datasize=536870912
  datasize=$(($datasize/1024/1024))
  file=$rawdir/CSL-throughput.data
  read_mbps=$(grep "Total  read_bytes:" $file | awk '{print $3}')
  write_mbps=$(grep "Total  write_bytes:" $file | awk '{print $3}')
  ttime=$(grep "Performace after" $file | awk '{print $3}')
  tr=$((($read_mbps*$ttime-$datasize)))
  tw=$((($write_mbps*$ttime-$datasize)))
  csl_rg=$(echo "a=$tr;a/1024" | bc -l)
  csl_wg=$(echo "a=$tw;a/1024" | bc -l)

  file=$rawdir/IRL-throughput.data
  read_mbps=$(grep "Total  read_bytes:" $file | awk '{print $3}')
  write_mbps=$(grep "Total  write_bytes:" $file | awk '{print $3}')
  ttime=$(grep "Performace after" $file | awk '{print $3}')
  tr=$((($read_mbps*$ttime-$datasize)))
  tw=$((($write_mbps*$ttime-$datasize)))
  irl_rg=$(echo "a=$tr;a/1024" | bc -l)
  irl_wg=$(echo "a=$tw;a/1024" | bc -l)

  file=$rawdir/TSL1-throughput.data
  read_mbps=$(grep "Total  read_bytes:" $file | awk '{print $3}')
  write_mbps=$(grep "Total  write_bytes:" $file | awk '{print $3}')
  ttime=$(grep "Performace after" $file | awk '{print $3}')
  tr=$((($read_mbps*$ttime-$datasize)))
  tw=$((($write_mbps*$ttime-$datasize)))
  tsl1_rg=$(echo "a=$tr;a/1024" | bc -l)
  tsl1_wg=$(echo "a=$tw;a/1024" | bc -l)

  file=$rawdir/TSL2-throughput.data
  read_mbps=$(grep "Total  read_bytes:" $file | awk '{print $3}')
  write_mbps=$(grep "Total  write_bytes:" $file | awk '{print $3}')
  ttime=$(grep "Performace after" $file | awk '{print $3}')
  tr=$((($read_mbps*$ttime-$datasize)))
  tw=$((($write_mbps*$ttime-$datasize)))
  tsl2_rg=$(echo "a=$tr;a/1024" | bc -l)
  tsl2_wg=$(echo "a=$tw;a/1024" | bc -l)

  file=$rawdir/TSL3-throughput.data
  read_mbps=$(grep "Total  read_bytes:" $file | awk '{print $3}')
  write_mbps=$(grep "Total  write_bytes:" $file | awk '{print $3}')
  ttime=$(grep "Performace after" $file | awk '{print $3}')
  tr=$((($read_mbps*$ttime-$datasize)))
  tw=$((($write_mbps*$ttime-$datasize)))
  tsl3_rg=$(echo "a=$tr;a/1024" | bc -l)
  tsl3_wg=$(echo "a=$tw;a/1024" | bc -l)

  file=$rawdir/TSL4-throughput.data
  read_mbps=$(grep "Total  read_bytes:" $file | awk '{print $3}')
  write_mbps=$(grep "Total  write_bytes:" $file | awk '{print $3}')
  ttime=$(grep "Performace after" $file | awk '{print $3}')
  tr=$((($read_mbps*$ttime-$datasize)))
  tw=$((($write_mbps*$ttime-$datasize)))
  tsl4_rg=$(echo "a=$tr;a/1024" | bc -l)
  tsl4_wg=$(echo "a=$tw;a/1024" | bc -l)

  ./$scriptdir/barplot_read_io.py $csl_rg $irl_rg $tsl1_rg $tsl2_rg $tsl3_rg $tsl4_rg $figdir/io_read.png
  ./$scriptdir/barplot_write_io.py $csl_wg $irl_wg $tsl1_wg $tsl2_wg $tsl3_wg $tsl4_wg $figdir/io_write.png
}

#
# Retrieve throughput for scalability numbers
#

retrieve_scalability () {
  scalethroughputfile=$rawdir/scalability_throughput_16.output
  for t in 1 2 4 8 16 32 64; do
    out=$rawdir/popcorn-output-v8-2G-$t.output
    echo "x_0 y_0" >> $scalethroughputfile
    r16=$(grep "Insertion throughput" $out | \
      cut -d " " -f 3 | sed -E 's/([+-]?[0-9.]+)[eE]\+?(-?)([0-9]+)/(\1*10^\2\3)/g')
    r16_tp=$(echo "x=$r16;x/10^6" | bc -l)
    echo "$(t) $(r16_tp)" >> $scalethroughputfile
  done

  scalethroughputfile=$rawdir/scalability_throughput_32.output
  for t in 1 2 4 8 16 32 64; do
    out=$rawdir/popcorn-output-v8-1G-$t.output
    echo "x_0 y_0" >> $scalethroughputfile
    r32=$(grep "Insertion throughput" $out | \
      cut -d " " -f 3 | sed -E 's/([+-]?[0-9.]+)[eE]\+?(-?)([0-9]+)/(\1*10^\2\3)/g')
    r32_tp=$(echo "x=$r32;x/10^6" | bc -l)
    echo "$(t) $(r32_tp)" >> $scalethroughputfile
  done

  scalethroughputfile=$rawdir/scalability_throughput_64.output
  for t in 1 2 4 8 16 32 64; do
    out=$rawdir/popcorn-output-v8-512M-$t.output
    echo "x_0 y_0" >> $scalethroughputfile
    r64=$(grep "Insertion throughput" $out | \
      cut -d " " -f 3 | sed -E 's/([+-]?[0-9.]+)[eE]\+?(-?)([0-9]+)/(\1*10^\2\3)/g')
    r64_tp=$(echo "x=$r64;x/10^6" | bc -l)
    echo "$(t) $(r64_tp)" >> $scalethroughputfile
  done
}


timestamp () {
  date +"%T" # current time
}

#
# This is the script to reproduce SIGMOD2020 paper experiments
#

#
# Setting local vars
#
activedir="firehose/generators/active/"
rawdir="sigmod20_raw"
scriptdir="sigmod20_scripts"
figdir="sigmod20_figs"
logfile="sigmod20_exp.log"

echo $(timestamp)
echo "Logs are diverted to $logfile"

echo $(timestamp) > $logfile

#############################################################

# Create dir
mkdir -p $rawdir
mkdir -p $figdir

#
# function calls
#

echo "$(timestamp): Generating data" > $logfile
generate_data
echo "$(timestamp): Running stretch validation" > $logfile
validate_stretch
echo "$(timestamp): Running arb stretch validation" > $logfile
validate_arb_stretch
echo "$(timestamp): Running buffer stretch validation" > $logfile
validate_buffer_stretch
echo "$(timestamp): Running lifetime stretch validation" > $logfile
validate_lifetime_stretch
echo "$(timestamp): Running throughput exp" > $logfile
throughput_exp
echo "$(timestamp): Running scalability throughput exp" > $logfile
scalability_exp
echo "$(timestamp): Running instantaneous throughput exp" > $logfile
instantaneous_throughput
echo "$(timestamp): Plotting stretch" > $logfile
plot_stretch
echo "$(timestamp): Plotting buffer/lifetime stretch" > $logfile
plot_buffer
echo "$(timestamp): Plotting I/O" > $logfile
plot_io
echo "$(timestamp): Plotting throughput" > $logfile
plot_throughput
echo "$(timestamp): Retrieving scalability data" > $logfile
retrieve_scalability
