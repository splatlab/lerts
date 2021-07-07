# lert

Timely Reporting of Heavy Hitters using External Memory
This work appeared at SIGMOD 2020 and the extended version of the paper appeared
at TODS 2021. If you use this software please cite us:

```
@inproceedings{DBLP:conf/sigmod/Pandey0BBFJKP20,
  author    = {Prashant Pandey and
               Shikha Singh and
               Michael A. Bender and
               Jonathan W. Berry and
               Martin Farach{-}Colton and
               Rob Johnson and
               Thomas M. Kroeger and
               Cynthia A. Phillips},
  editor    = {David Maier and
               Rachel Pottinger and
               AnHai Doan and
               Wang{-}Chiew Tan and
               Abdussalam Alawini and
               Hung Q. Ngo},
  title     = {Timely Reporting of Heavy Hitters using External Memory},
  booktitle = {Proceedings of the 2020 International Conference on Management of
               Data, {SIGMOD} Conference 2020, online conference [Portland, OR, USA],
               June 14-19, 2020},
  pages     = {1431--1446},
  publisher = {{ACM}},
  year      = {2020},
  url       = {https://doi.org/10.1145/3318464.3380598},
  doi       = {10.1145/3318464.3380598},
  timestamp = {Fri, 09 Apr 2021 18:43:50 +0200},
  biburl    = {https://dblp.org/rec/conf/sigmod/Pandey0BBFJKP20.bib},
  bibsource = {dblp computer science bibliography, https://dblp.org}
}
```

Overview
-------
LERTs are write-optimized data structures and perform timely event detection by
efficiently scaling to SSDs. LERTs achieve up to 11 Million events/sec insertion
throughput while timely reporting malicious events. This work shows that we can
match the performance of cache-latency-bound in-memory data structures without
any false-negatives or -positives. It further shows how to extend
write-optimized data structures, which have fast updates but relatively slow
queries, to support fast standing queries for timely event detection.

Build
-------

To build and test a sample program.

```bash
$ make main
```

To run the program.

```bash
$ make
$ ./main popcornfilter -f 1 -q 16 -l 3 -g 2 -t 1 -a 1 -o -v 24
```

```bash
SYNOPSIS
        popcornfilter -f <num_filters> -q <quotient_bits> -l <num_levels> -g <growth_factor> -t <num_threads> [-a <num_age_bits>] [-o] [-v <threshold_value>] [-e] [-p] [-i <input_file>] [-u <udp_port>]

OPTIONS
        <num_filters>
                    number of cascade filters in the popcorn filter. (default is 1)

        <quotient_bits>
                    log of number of slots in the in-memory level in each cascade filter. (default is 16)

        <num_levels>
                    number of levels in each cascade filter. (default is 4)

        <growth_factor>
                    growth factor in each cascade filter. (default is 4)

        <num_threads>
                    number of threads (default is 1)

        <num_age_bits>
                    number of aging bits. (default is 0)

        -o, --no-odp
                    do not perform on-demand popcorning. (default is true.)

        <threshold_value>
                    threshold_value  to report. (default is 24)

        -e, --greedy-flushing
                    greedy flushing optimization. (default is 0)

        -p, --pinning
                    pinning optimizations. (default is 0)

        <input_file>
                    input file containing keys and values. (default is generate keys from a Zipfian distribution.)

        <udp_port>  port at which generator will send the packets.)
```

Reproducibility
------------
This repo also includes the necessary scripts and infrastructure code to
reproduce the experiments and results from the SIGMOD2020 paper. Please take a
look at the README in the `lert_repro_sigmod20` directory for more
instructions.

Contributing
------------
Contributions via GitHub pull requests are welcome.


Authors
-------
- Prashant Pandey <ppandey@cs.stonybrook.edu>
