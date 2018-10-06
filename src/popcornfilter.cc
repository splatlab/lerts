/*
 * ============================================================================
 *
 *         Author:  Prashant Pandey (), ppandey@cs.stonybrook.edu
 *   Organization:  Stony Brook University
 *
 * ============================================================================
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

#include "clipp.h"
#include "ProgOpts.h"
#include "popcornfilter.h"

#define BUFFER_SIZE (1ULL << 16)

void *thread_insert(void *a) {
	ThreadArgs<KeyObject> *args = (ThreadArgs<KeyObject>*)a;

	CQF<KeyObject> buffer(BUFFER_SIZE, args->pf->get_num_key_bits(),
												args->pf->get_num_value_bits(), QF_HASH_DEFAULT,
												args->pf->get_seed());

	/* First try and insert the key-value pair in the cascade filter. If the
	 * insert fails then insert in the buffer. Later dump the buffer in the
	 * cascade filter.
	 */
	for (uint64_t i = args->start; i < args->end; i++) {
		uint64_t key = args->vals[i];
		if (!args->pf->insert(KeyObject(key, 0, 1, 0),
													PF_TRY_ONCE_LOCK)) {
			buffer.insert(KeyObject(key, 0, 1, 0), PF_NO_LOCK);
			double load_factor = buffer.occupied_slots() /
				(double)buffer.total_slots();
			if (load_factor > 0.75) {
				DEBUG("Dumping buffer.");
				typename CQF<KeyObject>::Iterator it = buffer.begin();
				do {
					KeyObject key = *it;
					if (!args->pf->insert(key, PF_WAIT_FOR_LOCK | QF_KEY_IS_HASH)) {
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
	if (buffer.total_elts() > 0) {
		DEBUG("Dumping buffer final time.");
		typename CQF<KeyObject>::Iterator it = buffer.begin();
		do {
			KeyObject key = *it;
			if (!args->pf->insert(key, PF_WAIT_FOR_LOCK | QF_KEY_IS_HASH)) {
				std::cerr << "Failed insertion for " << (uint64_t)key.key << std::endl;
				abort();
			}
			++it;
		} while(!it.done());
	}

	return NULL;
}

void perform_insertion(std::vector<ThreadArgs<KeyObject>> args, uint32_t
											 nthreads) {
	pthread_t threads[nthreads];

	for (uint32_t i = 0; i < nthreads; i++) {
		DEBUG("Starting thread " << i << " from " << args[i].start << " to " <<
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

// random generator function:
uint64_t myrandom (uint64_t i) { return std::rand()%i;}

/* 
 * ===  FUNCTION  =============================================================
 *         Name:  main
 *  Description:  
 * ===========================================================================
 */
int popcornfilter_main (PopcornFilterOpts opts)
{
	uint64_t qbits = opts.qbits;
	uint32_t nlevels = opts.nlevels;
	uint32_t gfactor = opts.gfactor;
	uint64_t nfilters = opts.nfilters;
	uint64_t nthreads = opts.nthreads;
	uint32_t nagebits = opts.nagebits;
	uint32_t do_odp = opts.do_odp;
	uint32_t greedy = opts.greedy;
	uint32_t pinning = opts.pinning;
	uint32_t threshold_value = opts.threshold_value;

	PopcornFilter<KeyObject> pf(nfilters, qbits, nlevels, gfactor, nagebits,
															do_odp, greedy, pinning, threshold_value);

	uint64_t nvals = 0;

	uint64_t *vals;
	std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>> keylifetimes;
	if (opts.ip_file.size() > 1) {
		PRINT("Reading input stream and logs from disk");
		std::string streamlogfile(opts.ip_file + ".log");
		vals = popcornfilter::read_stream_from_disk(opts.ip_file, &nvals);
#ifdef VALIDATE
		keylifetimes = popcornfilter::analyze_stream(vals, nvals, threshold_value);
#endif
	} else {
	nvals = pf.get_max_size();
#if 0
		// This is a specific generator to produce Jon's use-case.
		uint64_t quarter = nvals;
		uint64_t half = 2*quarter;
		nvals = nvals + nvals + 7 * (nvals/4);
		uint64_t rest = nvals - half;
		vals = (uint64_t*)calloc(nvals, sizeof(vals[0]));
		memset(vals, 0, nvals * sizeof(vals[0]));
		/* Generate random keys from a Zipfian distribution. */
		PRINT("Generating " << nvals << " random numbers.");

		RAND_bytes((unsigned char *)vals, sizeof(*vals) * (quarter));
		for (uint64_t i = 0; i < quarter; i++) {
			vals[i] = vals[i] % pf.get_range();
			vals[i + quarter] = vals[i] % pf.get_range();
		}
		RAND_bytes((unsigned char *)(vals + half), sizeof(*vals) * (rest));
		for (uint64_t i = half; i < nvals; i++)
			vals[i] = vals[i] % pf.get_range();

		//RAND_pseudo_bytes((unsigned char *)vals, sizeof(*vals) * (nvals));
		//for (uint64_t i = 0; i < nvals; i++)
		//vals[i] = vals[i] % pf.get_range();
#endif
		vals = (uint64_t*)calloc(nvals, sizeof(vals[0]));
		memset(vals, 0, nvals * sizeof(vals[0]));
		/* Generate random keys from a Zipfian distribution. */
		PRINT("Generating " << nvals << " random numbers.");
		generate_random_keys(vals, nvals, nvals, 1.5);
		//std::random_shuffle(&vals[0], &vals[nvals-1], myrandom);
#ifdef VALIDATE
		keylifetimes = popcornfilter::analyze_stream(vals, nvals, threshold_value);
		//popcornfilter::induce_special_case(vals, threshold_value, 29000, 40000, 5);
		//keylifetimes = popcornfilter::analyze_stream(vals, nvals, threshold_value);
#endif
	}
	PRINT("Total observations: " << nvals);

	struct timeval start, end;
	struct timezone tzp;
	std::vector<ThreadArgs<KeyObject>> args;
	args.reserve(nthreads);

	for (uint64_t i = 0; i < nthreads; i++) {
		ThreadArgs<KeyObject> obj;
		obj.pf = &pf;
		obj.vals = vals;
		obj.start = i * (nvals / nthreads);
		obj.end = (i + 1) * (nvals / nthreads);
		args.emplace_back(obj);
	}

	PRINT("Inserting elements.");
	gettimeofday(&start, &tzp);
	perform_insertion(args, nthreads);
	gettimeofday(&end, &tzp);
	popcornfilter::print_time_elapsed("", &start, &end);
	PRINT("Finished insertions.");

	//PRINT("Total distinct elements inserted: " <<
				//pf.get_total_dist_elements());

#ifdef VALIDATE
	PRINT("Querying elements.");
	gettimeofday(&start, &tzp);
	for (uint64_t k = 0; k < nvals; k++) {
		uint64_t key = vals[k];
		if (pf.query(KeyObject(key, 0, 0, 0), PF_WAIT_FOR_LOCK) < 1) {
			std::cerr << "Failed lookup for " <<
				(uint64_t)vals[k] << " " << k << " " << nvals << std::endl;
			abort();
		}
	}
	gettimeofday(&end, &tzp);
	popcornfilter::print_time_elapsed("", &start, &end);
	PRINT("Finished lookups.");
#endif

#ifdef VALIDATE
	if (nthreads == 1) {
		PRINT("Performing validation");
		if (pf.validate_anomalies(keylifetimes, vals))
			PRINT("Validation successful!");
		else
			PRINT("Validation failed!");
	}
#else
	if (!opts.do_odp)
		pf.find_anomalies();
#endif

	PRINT("Total number of keys above threshold: " <<
				pf.get_total_keys_above_threshold());

	pf.print_stats();

	return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */
