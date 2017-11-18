/*
 * =============================================================================
 *
 *       Filename:  popcornfilter.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017-10-19 10:19:18 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Prashant Pandey (), ppandey@cs.stonybrook.edu
 *   Organization:  Stony Brook University
 *
 * =============================================================================
 */


#ifndef _POPCORNFILTER_H_
#define _POPCORNFILTER_H_

#include <sys/types.h>

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

#include "cascadefilter.h"
#include "hashutil.h"
#include "zipf.h"

#include "cascadefilter.h"

#define NUM_MAX_FILTERS 10
#define NUM_MAX_THREADS 20

template <class key_object>
class PopcornFilter {
	public:
		PopcornFilter(uint64_t nfilters, uint32_t qbits, uint32_t
									nlevels, uint32_t gfactor);

		bool insert(const key_object& k, enum lock flag);

		uint64_t query(const key_object& k, enum lock flag) const;

		uint32_t get_num_hash_bits(void) const;
		uint32_t get_seed(void) const;
		uint64_t get_range(void) const;
		uint64_t get_max_size(void) const;
		uint64_t get_total_elements(void) const;
		uint64_t get_total_dist_elements(void) const;

	private:
		uint64_t nfilters;
		uint32_t qbits;
		uint32_t nlevels;
		uint32_t gfactor;
		uint32_t fbits;
		uint32_t nhashbits;
		uint32_t nvaluebits;
		uint32_t nagebits;
		CascadeFilter<key_object> *cf[NUM_MAX_FILTERS];
};

template <class key_object>
class ThreadArgs {
	public:
		PopcornFilter<key_object> *pf;
		uint64_t *vals;
		uint64_t start;
		uint64_t end;

		ThreadArgs() : pf(NULL), vals(NULL), start(0), end(0) {};
		ThreadArgs(PopcornFilter<key_object> *pf, key_object *vals, uint64_t start,
							 uint64_t end) : pf(pf), vals(vals), start(start), end(end) {};
};

#define NUM_HASH_BITS 32
/* We use value bits to store the value of the key from FireHose. */
#define NUM_VALUE_BITS 1
/* We also use the value bits to store the age of the key. */
#define NUM_AGE_BITS 1

template <class key_object>
PopcornFilter<key_object>::PopcornFilter(uint64_t nfilters, uint32_t qbits,
																				 uint32_t nlevels, uint32_t gfactor) :
	nfilters(nfilters), qbits(qbits), nlevels(nlevels),
	gfactor(gfactor) {
		fbits = log2(nfilters); 	// assuming nfilters is a power of 2.
		nhashbits = NUM_HASH_BITS;
		nvaluebits = NUM_VALUE_BITS;
		nagebits = NUM_AGE_BITS;
		uint64_t sizes[nlevels];
		uint32_t thlds[nlevels];

		/* level sizes grow by a factor "r". */
		sizes[0] = (1ULL << qbits);
		for (uint32_t i = 1; i < nlevels; i++)
			sizes[i] = pow(gfactor, i) * sizes[0];

		// if there are age bits then taus are infinity.
		if (NUM_AGE_BITS) {
			for (int32_t i = 0; i < nlevels; i++)
				thlds[i] = UINT32_MAX;
		} else {
			thlds[nlevels - 1] = 1;
			uint32_t j = 1;
			/* taus grow with r^0.5. */
			uint32_t tau_ratio = sqrt(gfactor);
			for (int32_t i = nlevels - 2; i >= 0; i--, j++)
				thlds[i] = pow(tau_ratio, j) * thlds[nlevels - 1];
		}
		/* Create a cascade filter. */
		for (uint32_t i = 0; i < nfilters; i++) {
			PRINT_CF("Create a cascade filter with " << nhashbits << "-bit hashes, "
				<< nlevels << " levels, and " << gfactor << " as growth factor.");
			std::string prefix = "raw/" + std::to_string(i) + "_";
			cf[i] = new CascadeFilter<KeyObject>(nhashbits, nvaluebits, nagebits,
																					 thlds, sizes, nlevels, prefix);
		}
	}

template <class key_object>
uint32_t PopcornFilter<key_object>::get_num_hash_bits(void) const {
	return cf[0]->get_num_hash_bits();
}

template <class key_object>
uint32_t PopcornFilter<key_object>::get_seed(void) const {
	return cf[0]->get_seed();
}

template <class key_object>
uint64_t PopcornFilter<key_object>::get_range(void) const {
	uint64_t range = cf[0]->get_filter(0)->range();
	range <<= fbits;
	return range;
}

template <class key_object>
uint64_t PopcornFilter<key_object>::get_max_size(void) const {
	return cf[0]->get_max_size() * nfilters;
}

template <class key_object>
uint64_t PopcornFilter<key_object>::get_total_elements(void) const {
	uint64_t total = 0;
	for (uint32_t i = 0; i < nfilters; i++)
		total += cf[i]->get_num_elements();
	return total;
}

template <class key_object>
uint64_t PopcornFilter<key_object>::get_total_dist_elements(void) const {
	uint64_t total = 0;
	for (uint32_t i = 0; i < nfilters; i++)
		total += cf[i]->get_num_dist_elements();
	return total;
}

template <class key_object>
bool PopcornFilter<key_object>::insert(const key_object& k, enum lock flag) {
	KeyObject dup_k(k);
	uint32_t filter_idx = dup_k.key >> nhashbits;
	dup_k.key = dup_k.key & BITMASK(nhashbits);
	return cf[filter_idx]->insert(dup_k, flag);
}

template <class key_object>
uint64_t PopcornFilter<key_object>::query(const key_object& k, enum lock flag)
	const {
	KeyObject dup_k(k);
	uint32_t filter_idx = dup_k.key >> nhashbits;
	dup_k.key = dup_k.key & BITMASK(nhashbits);
	return cf[filter_idx]->count_key_value(dup_k, flag);
}

#endif
