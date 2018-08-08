# popcorning

To run the program.

```bash
$ make
$ ./main popcornfilter -f 1 -q 12 -l 3 -g 2 -t 1 -a 1 -o -v 24
```

```bash
SYNOPSIS
        popcornfilter -f <num_filters> -q <quotient_bits> -l <num_levels> -g <growth_factor> -t <num_threads> [-a <num_age_bits>] [-o] [-v <threshold_value>] [-i <input_file>]

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

        <input_file>
                    input file containing keys and values. (default is generate keys from a Zipfian distribution.)
```
