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

#define BUFFER_SIZE (1ULL << 16)

void *thread_insert(void *a) {
	ThreadArgs<KeyObject> *args = (ThreadArgs<KeyObject>*)a;

	uint32_t hash_bits = log2(args->pf->get_range());
	CQF<KeyObject> buffer(BUFFER_SIZE, hash_bits, 0, /*mem*/ true, "",
												args->pf->get_seed());

	/* First try and insert the key-value pair in the cascade filter. If the
	 * insert fails then insert in the buffer. Later dump the buffer in the
	 * cascade filter.
	 */
	for (uint64_t i = args->start; i < args->end; i++) {
		if (args->vals[i] >= args->pf->get_range()) {
			PRINT_CF("Index: " << i << " val: " << args->vals[i]);
		}
		if (!args->pf->insert(KeyObject(args->vals[i], 0, 1, 0), LOCK_NO_SPIN)) {
			buffer.insert(KeyObject(args->vals[i], 0, 1, 0), NO_LOCK);
			double load_factor = buffer.occupied_slots() /
				(double)buffer.total_slots();
			if (load_factor > 0.75) {
				DEBUG_CF("Dumping buffer.");
				typename CQF<KeyObject>::Iterator it = buffer.begin();
				do {
					KeyObject key = *it;
					if (!args->pf->insert(key, LOCK_AND_SPIN)) {
						std::cerr << "Failed insertion for " << (uint64_t)key.key <<
							std::endl;
						abort();
					}
					++it;
				} while(!it.done());
				buffer.reset();
			}
		}
	}
	/* Finally dump the anything left in the buffer. */
	if (buffer.total_elements() > 0) {
		DEBUG_CF("Dumping buffer final time.");
		typename CQF<KeyObject>::Iterator it = buffer.begin();
		do {
			KeyObject key = *it;
			if (!args->pf->insert(key, LOCK_AND_SPIN)) {
				std::cerr << "Failed insertion for " << (uint64_t)key.key << std::endl;
				abort();
			}
			++it;
		} while(!it.done());
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

	if (argc >= 8) {
		nagebits = atoi(argv[6]);
		do_odp = atoi(argv[7]);
	}

	PopcornFilter<KeyObject> pf(nfilters, qbits, nlevels, gfactor, nagebits,
															do_odp);

	uint64_t nvals = 750 * pf.get_max_size() / 1000;
	nvals *= 2;

	uint64_t *vals;
	std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>> keylifetimes;
	if (argc == 9) {
		PRINT_CF("Reading input stream and logs from disk");
		std::string streamfile(argv[8]);
		std::string streamlogfile(streamfile + ".log");
		vals = read_stream_from_disk(streamfile);
		nvals = 50000000;
		keylifetimes = read_stream_log_from_disk(streamlogfile);
	} else {
		vals = (uint64_t*)calloc(nvals, sizeof(vals[0]));
		memset(vals, 0, nvals * sizeof(vals[0]));
		uint64_t quarter = nvals / 4;
		uint64_t half = nvals / 2;

		/* Generate random keys from a Zipfian distribution. */
		PRINT_CF("Generating " << nvals << " random numbers.");
		RAND_pseudo_bytes((unsigned char *)vals, sizeof(*vals) * (quarter));
		for (uint64_t i = 0; i < quarter; i++) {
			vals[i] = vals[i] % pf.get_range();
			vals[i + quarter] = vals[i] % pf.get_range();
		}
		RAND_pseudo_bytes((unsigned char *)(vals + half), sizeof(*vals) * (half));
		for (uint64_t i = half; i < nvals; i++)
			vals[i] = vals[i] % pf.get_range();

		//generate_random_keys(vals, nvals, nvals, 1.5);
		//for (uint64_t i = 0; i < nvals; i++) {
			//vals[i] = HashUtil::AES_HASH(vals[i]) % pf.get_range();
		//}
		keylifetimes = analyze_stream(vals, nvals, THRESHOLD_VALUE);
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

	if (nthreads == 1) {
		PRINT_CF("Performing validation");
		if (pf.validate_anomalies(keylifetimes))
			PRINT_CF("Validation successful!");
		else
			PRINT_CF("Validation failed!");
	}

	PRINT_CF("Total number of keys above thrshold: " <<
					 pf.get_total_keys_above_threshold());

	//pf.print_stats();

	return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */
