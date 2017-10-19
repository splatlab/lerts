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
PopcornFilter<key_object>::PopcornFilter(uint64_t nfilters, uint32_t
																				 nthreads, uint32_t qbits, uint32_t
																				 nlevels, uint32_t gfactor) :
	nfilters(nfilters), nthreads(nthreads), qbits(qbits), nlevels(nlevels),
	gfactor(gfactor) {
		nhashbits = qbits + 10;
		uint64_t sizes[nlevels];
		uint32_t thlds[nlevels];

		/* level sizes grow by a factor "r". */
		sizes[0] = (1ULL << qbits);
		for (uint32_t i = 1; i < nlevels; i++)
			sizes[i] = pow(gfactor, i) * sizes[0];


		thlds[nlevels - 1] = 1;
		uint32_t j = 1;
		/* taus grow with r^0.5. */
		uint32_t tau_ratio = sqrt(gfactor);
		for (int32_t i = nlevels - 2; i >= 0; i--, j++)
			thlds[i] = pow(tau_ratio, j) * thlds[nlevels - 1];

		/* Create a cascade filter. */
		for (uint32_t i = 0; i < nfilters; i++) {
			std::cout << "Create a cascade filter with " << nhashbits << "-bit hashes, "
				<< nlevels << " levels, and " << gfactor << " as growth factor." <<
				std::endl;
			cf[i] = new CascadeFilter<KeyObject>(nhashbits, thlds, sizes, nlevels);
		}
	}

template <class key_object>
uint32_t PopcornFilter<key_object>::get_num_hash_bits(void) {
	return cf[0]->get_num_hash_bits();
}

template <class key_object>
uint32_t PopcornFilter<key_object>::get_seed(void) {
	return cf[0]->get_seed();
}

template <class key_object>
bool PopcornFilter<key_object>::insert(key_object k) {

}

template <class key_object>
uint64_t PopcornFilter<key_object>::query(key_object k) {

}


template <class key_object>
void *insert(void *a) {
	QF buffer;
	QFi it_buffer;
	ThreadArgs<KeyObject> *args = (ThreadArgs<KeyObject>*)a;

	qf_init(&buffer, 16, cf[0].get_num_hash_bits(), 0, /*mem*/ true,
					"", cf[0].get_seed());

	/* First try and insert the key-value pair in the cascade filter. If the
	 * insert fails then insert in the buffer. Later dump the buffer in the
	 * cascade filter.
	 */
	for (uint64_t i = args->start; i < args->end; i++) {
		if (!args->cf.insert(args->vals[i], LOCK_NO_SPIN)) {
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
					args->cf.insert(k, LOCK_AND_SPIN);
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
			args->cf.insert(k, LOCK_AND_SPIN);
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
	uint32_t nhashbits = qbits + 10;
	uint32_t randominput = 1;	// Default value is uniform-random distribution.
	if (argc > 4)
		randominput = atoi(argv[4]);

	uint64_t sizes[nlevels];
	uint32_t thlds[nlevels];

	struct timeval start, end;
	struct timezone tzp;

	/* level sizes grow by a factor "r". */
	sizes[0] = (1ULL << qbits);
	for (uint32_t i = 1; i < nlevels; i++)
		sizes[i] = pow(gfactor, i) * sizes[0];

	uint64_t nvals = 750 * (sizes[nlevels - 1]) / 1000;

	thlds[nlevels - 1] = 1;
	uint32_t j = 1;
	/* taus grow with r^0.5. */
	uint32_t tau_ratio = sqrt(gfactor);
	for (int32_t i = nlevels - 2; i >= 0; i--, j++)
		thlds[i] = pow(tau_ratio, j) * thlds[nlevels - 1];

	/* Create a cascade filter. */
	std::cout << "Create a cascade filter with " << nhashbits << "-bit hashes, "
		<< nlevels << " levels, and " << gfactor << " as growth factor." <<
		std::endl;
	CascadeFilter<KeyObject> cf(nhashbits, thlds, sizes, nlevels);

	uint64_t *vals;
	vals = (uint64_t*)malloc(nvals*sizeof(vals[0]));
	std::cout << "Generating " << nvals << " random numbers." << std::endl;
	memset(vals, 0, nvals*sizeof(vals[0]));

	if (randominput) {
		/* Generate random keys from a uniform-random distribution. */
		RAND_pseudo_bytes((unsigned char *)vals, sizeof(*vals) * nvals);
		for (uint64_t k = 0; k < nvals; k++)
			vals[k] = (1 * vals[k]) % cf.get_filter(0)->metadata->range;
	} else {
		/* Generate random keys from a Zipfian distribution. */
		generate_random_keys(vals, nvals, nvals, 1.5);
		for (uint64_t i = 0; i < nvals; i++) {
			vals[i] = HashUtil::AES_HASH(vals[i]) % cf.get_filter(0)->metadata->range;
		}
	}
	std::cout << "Inserting elements." << std::endl;
	gettimeofday(&start, &tzp);
	for (uint64_t k = 0; k < nvals; k++)
		if (!cf.insert(KeyObject(vals[k], 0, 1, 0), LOCK_AND_SPIN)) {
			std::cerr << "Failed insertion for " <<
				(uint64_t)vals[k] << std::endl;
			abort();
		}
	gettimeofday(&end, &tzp);
	print_time_elapsed("", &start, &end);
	std::cout << "Finished insertions." << std::endl;

	DEBUG_CF("Number of elements in the CascadeFilter " << cf.get_num_elements());

	//QF new_cf;
	//qf_init(&new_cf, sizes[nlevels - 1], nhashbits, 0, [>mem<] true,
	//"", 0);
	//for (auto it = cf.begin(nlevels); it != cf.end(); ++it) {
	//KeyObject k; 
	//k = *it;
	//qf_insert(&new_cf, k.key, k.value, k.count, LOCK_AND_SPIN);
	//}
	//for (uint64_t k = 0; k < nvals; k++)
	//if (qf_count_key_value(&new_cf, vals[k], 0) < 1) {
	//std::cerr << "Failed lookup for " <<
	//(uint64_t)vals[k] << " " << k << " " << nvals << std::endl;
	//abort();
	//}
	//DEBUG_CF("Iterator verified!");

	std::cout << "Querying elements." << std::endl;
	gettimeofday(&start, &tzp);
	for (uint64_t k = 0; k < nvals; k++)
		if (cf.count_key_value(KeyObject(vals[k], 0, 0, 0)) < 1) {
			std::cerr << "Failed lookup for " <<
				(uint64_t)vals[k] << " " << k << " " << nvals << std::endl;
			abort();
		}
	gettimeofday(&end, &tzp);
	print_time_elapsed("", &start, &end);
	std::cout << "Finished lookups." << std::endl;

	return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */
