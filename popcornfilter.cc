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
#include "cascadefilter.h"
#include "hashutil.h"
#include "zipf.h"
#include "util.h"

template <class key_object>
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
		if (!args->pf->insert(args->vals[i], LOCK_NO_SPIN)) {
			qf_insert(&buffer, args->vals[i].key, args->vals[i].value,
								args->vals[i].count, LOCK_AND_SPIN);

			double load_factor = buffer.metadata->noccupied_slots /
				(double)buffer.metadata->nslots;
			if (load_factor > 0.75) {
				qf_iterator(&buffer, &it_buffer, 0);
				do {
					uint64_t key, value, count;
					qfi_get(&it_buffer, &key, &value, &count);
					key_object k(key, value, count, 0);
					args->pf->insert(k, LOCK_AND_SPIN);
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
			key_object k(key, value, count, 0);
			args->pf->insert(k, LOCK_AND_SPIN);
			qfi_next(&it_buffer);
		} while(!qfi_end(&it_buffer));
	}

	return NULL;
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
		std::cout << "Not suffcient args." << std::endl;
		abort();
	}

	uint64_t qbits = atoi(argv[1]);
	uint32_t nlevels = atoi(argv[2]);
	uint32_t gfactor = atoi(argv[3]);
	uint64_t nfilters = atoi(argv[4]);
	uint64_t nthreads = atoi(argv[5]);

	PopcornFilter<KeyObject> pf(nfilters, nthreads, qbits, nlevels, gfactor);

	uint64_t nvals = 750 * pf.get_max_size() / 1000;

	uint64_t *vals;
	vals = (uint64_t*)malloc(nvals*sizeof(vals[0]));
	std::cout << "Generating " << nvals << " random numbers." << std::endl;
	memset(vals, 0, nvals*sizeof(vals[0]));

	/* Generate random keys from a Zipfian distribution. */
	generate_random_keys(vals, nvals, nvals, 1.5);
	for (uint64_t i = 0; i < nvals; i++) {
		vals[i] = HashUtil::AES_HASH(vals[i]) % pf.get_range();
	}

	struct timeval start, end;
	struct timezone tzp;

	std::cout << "Inserting elements." << std::endl;
	gettimeofday(&start, &tzp);
	for (uint64_t k = 0; k < nvals; k++)
		if (!pf.insert(KeyObject(vals[k], 0, 1, 0), LOCK_AND_SPIN)) {
			std::cerr << "Failed insertion for " <<
				(uint64_t)vals[k] << std::endl;
			abort();
		}
	gettimeofday(&end, &tzp);
	print_time_elapsed("", &start, &end);
	std::cout << "Finished insertions." << std::endl;

	DEBUG_CF("Number of elements in the CascadeFilter " <<
					 pf.get_total_elements());

	std::cout << "Querying elements." << std::endl;
	gettimeofday(&start, &tzp);
	for (uint64_t k = 0; k < nvals; k++)
		if (pf.query(KeyObject(vals[k], 0, 0, 0)) < 1) {
			std::cerr << "Failed lookup for " <<
				(uint64_t)vals[k] << " " << k << " " << nvals << std::endl;
			abort();
		}
	gettimeofday(&end, &tzp);
	print_time_elapsed("", &start, &end);
	std::cout << "Finished lookups." << std::endl;

	return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */
