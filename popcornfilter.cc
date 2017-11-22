/*
 * =============================================================================
 *
 *       Filename:  popcornfilter.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017-10-19 10:22:02 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Prashant Pandey (), ppandey@cs.stonybrook.edu
 *   Organization:  Stony Brook University
 *
 * =============================================================================
 */

#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>
#include <set>
#include <unordered_set>
#include <bitset>
#include <cassert>
#include <fstream>

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <openssl/rand.h>

#include "popcornfilter.h"

void *thread_insert(void *a) {
	QF buffer;
	QFi it_buffer;
	ThreadArgs<KeyObject> *args = (ThreadArgs<KeyObject>*)a;

	qf_init(&buffer, 16, args->pf->get_num_hash_bits(), 0, /*mem*/ true,
					"", args->pf->get_seed());

	/* First try and insert the key-value pair in the cascade filter. If the
	 * insert fails then insert in the buffer. Later dump the buffer in the
	 * cascade filter.
	 */
	for (uint64_t i = args->start; i < args->end; i++) {
		if (!args->pf->insert(KeyObject(args->vals[i], 0, 1, 0), LOCK_NO_SPIN)) {
			qf_insert(&buffer, args->vals[i], 0, 1, NO_LOCK);
			double load_factor = buffer.metadata->noccupied_slots /
				(double)buffer.metadata->nslots;
			if (load_factor > 0.75) {
				qf_iterator(&buffer, &it_buffer, 0);
				do {
					uint64_t key, value, count;
					qfi_get(&it_buffer, &key, &value, &count);
					if (!args->pf->insert(KeyObject(key, value, count, 0),
																LOCK_AND_SPIN)) {
						std::cerr << "Failed insertion for " << (uint64_t)key << std::endl;
						abort();
					}
					qfi_next(&it_buffer);
				} while(!qfi_end(&it_buffer));
				qf_reset(&buffer);
			}
		}
	}
	/* Finally dump the anything left in the buffer. */
	if (buffer.metadata->nelts > 0) {
		qf_iterator(&buffer, &it_buffer, 0);
		do {
			uint64_t key, value, count;
			qfi_get(&it_buffer, &key, &value, &count);
			if (!args->pf->insert(KeyObject(key, value, count, 0), LOCK_AND_SPIN)) {
				std::cerr << "Failed insertion for " << (uint64_t)key << std::endl;
				abort();
			}
			qfi_next(&it_buffer);
		} while(!qfi_end(&it_buffer));
	}

	return NULL;
}

void perform_insertion(ThreadArgs<KeyObject> args[], uint32_t nthreads) {
	pthread_t threads[nthreads];

	for (uint32_t i = 0; i < nthreads; i++) {
		DEBUG_CF("Starting thread " << i << " from " << args[i].start << " to " <<
						 args[i].end);
		if (pthread_create(&threads[i], NULL, &thread_insert, &args[i])) {
			std::cerr << "Error creating thread " << i << std::endl;
			abort();
		}
	}

	for (uint32_t i = 0; i < nthreads; i++)
		if (pthread_join(threads[i], NULL)) {
			std::cerr << "Error joining thread " << i << std::endl;
			abort();
		}
}

/* 
 * ===  FUNCTION  =============================================================
 *         Name:  main
 *  Description:  
 * ===========================================================================
 */
	int
main ( int argc, char *argv[] )
{
	if (argc < 6) {
		PRINT_CF("Not suffcient args.");
		abort();
	}

	uint64_t qbits = atoi(argv[1]);
	uint32_t nlevels = atoi(argv[2]);
	uint32_t gfactor = atoi(argv[3]);
	uint64_t nfilters = atoi(argv[4]);
	uint64_t nthreads = atoi(argv[5]);
	uint32_t nagebits = 0, do_odp = 1;

	if (argc == 8) {
		nagebits = atoi(argv[6]);
		do_odp = atoi(argv[7]);
	}

	if (nagebits)
		PRINT_CF("Creating a popcorn filter with time-stretch");
	else
		PRINT_CF("Creating a popcorn filter with immediate reporting.");

	PopcornFilter<KeyObject> pf(nfilters, qbits, nlevels, gfactor, nagebits,
															do_odp);

	uint64_t nvals = 750 * pf.get_max_size() / 1000;

	uint64_t *vals;
	vals = (uint64_t*)calloc(nvals, sizeof(vals[0]));
	PRINT_CF("Generating " << nvals << " random numbers.");
	memset(vals, 0, nvals*sizeof(vals[0]));

	/* Generate random keys from a Zipfian distribution. */
	generate_random_keys(vals, nvals, nvals, 1.5);
	for (uint64_t i = 0; i < nvals; i++) {
		vals[i] = HashUtil::AES_HASH(vals[i]) % pf.get_range();
	}

	struct timeval start, end;
	struct timezone tzp;
	ThreadArgs<KeyObject> args[NUM_MAX_THREADS];

	for (uint64_t i = 0; i < nthreads; i++) {
		args[i].pf = &pf;
		args[i].vals = vals;
		args[i].start = i * (nvals / nthreads);
		args[i].end = (i + 1) * (nvals / nthreads);
	}

	PRINT_CF("Inserting elements.");
	gettimeofday(&start, &tzp);
	perform_insertion(args, nthreads);
	gettimeofday(&end, &tzp);
	print_time_elapsed("", &start, &end);
	PRINT_CF("Finished insertions.");

	PRINT_CF("Total elements inserted: " << pf.get_total_elements());
	PRINT_CF("Total distinct elements inserted: " <<
					 pf.get_total_dist_elements());

	PRINT_CF("Querying elements.");
	gettimeofday(&start, &tzp);
	for (uint64_t k = 0; k < nvals; k++)
		if (pf.query(KeyObject(vals[k], 0, 0, 0), LOCK_AND_SPIN) < 1) {
			std::cerr << "Failed lookup for " <<
				(uint64_t)vals[k] << " " << k << " " << nvals << std::endl;
			abort();
		}
	gettimeofday(&end, &tzp);
	print_time_elapsed("", &start, &end);
	PRINT_CF("Finished lookups.");

	//pf.print_stats();

	std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>> keylifetimes =
		analyze_stream(vals, nvals);

	PRINT_CF("Performing validation");
	if (pf.validate_anomalies(keylifetimes))
		PRINT_CF("Validation successful!");
	else
		PRINT_CF("Validation failed!");

	//pf.print_stats();

	return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */
