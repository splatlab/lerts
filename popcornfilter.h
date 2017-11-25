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
									nlevels, uint32_t gfactor, uint32_t nagebits = 0, bool
									do_odp = true);

		bool insert(const key_object& k, enum lock flag);

		uint64_t query(const key_object& k, enum lock flag);

		uint64_t get_total_keys_above_threshold(void) const;
		uint32_t get_num_hash_bits(void) const;
		uint32_t get_seed(void) const;
		uint64_t get_range(void) const;
		uint64_t get_max_size(void) const;
		uint64_t get_total_elements(void) const;
		uint64_t get_total_dist_elements(void) const;

		void print_stats(void) const;
		bool validate_anomalies(std::unordered_map<uint64_t,
															 std::pair<uint64_t, uint64_t>> key_lifetime);

	private:
		uint64_t nfilters;
		uint32_t qbits;
		uint32_t nlevels;
		uint32_t gfactor;
		uint32_t nagebits;
		bool odp;
		uint32_t fbits;
		uint32_t nhashbits;
		uint32_t nvaluebits;
		uint64_t num_obs_seen;
		LightweightLock pf_lw_lock;
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

template <class key_object>
PopcornFilter<key_object>::PopcornFilter(uint64_t nfilters, uint32_t qbits,
																				 uint32_t nlevels, uint32_t gfactor,
																				 uint32_t nagebits, bool do_odp) :
	nfilters(nfilters), qbits(qbits), nlevels(nlevels),
	gfactor(gfactor), nagebits(nagebits), odp(do_odp) {
		fbits = log2(nfilters); 	// assuming nfilters is a power of 2.
		nhashbits = NUM_HASH_BITS;
		nvaluebits = NUM_VALUE_BITS;
		num_obs_seen = 0;
		uint64_t sizes[nlevels];
		uint32_t thlds[nlevels];

		/* level sizes grow by a factor "r". */
		sizes[0] = (1ULL << qbits);
		for (uint32_t i = 1; i < nlevels; i++)
			sizes[i] = pow(gfactor, i) * sizes[0];

		// if there are age bits then taus are infinity.
		if (nagebits) {
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
			PRINT_CF("Creating a cascade filter with " << nhashbits <<
							 "-bit hashes, " << nlevels << " levels, and " << gfactor <<
							 " as growth factor.");
			std::string prefix = "raw/" + std::to_string(i) + "_";
			cf[i] = new CascadeFilter<KeyObject>(nhashbits, nvaluebits, nagebits,
																					 odp, thlds, sizes, nlevels, prefix);
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
uint64_t PopcornFilter<key_object>::get_total_keys_above_threshold(void) const
{
	uint64_t total = 0;
	for (uint32_t i = 0; i < nfilters; i++)
		total += cf[i]->get_num_keys_above_threshold();
	return total;
}

template <class key_object>
bool PopcornFilter<key_object>::insert(const key_object& k, enum lock flag) {
	bool ret = true;
	KeyObject dup_k(k);

	uint32_t filter_idx = dup_k.key >> nhashbits;
	dup_k.key = dup_k.key & BITMASK(nhashbits);
	ret = cf[filter_idx]->insert(dup_k, num_obs_seen, flag);

	if (ret) {
		pf_lw_lock.lock(LOCK_AND_SPIN);
		num_obs_seen += dup_k.count;
		pf_lw_lock.unlock();

	}

	return ret;
}

template <class key_object>
uint64_t PopcornFilter<key_object>::query(const key_object& k, enum lock flag)
{
	uint64_t count = 0;
	KeyObject dup_k(k);

	uint32_t filter_idx = dup_k.key >> nhashbits;
	dup_k.key = dup_k.key & BITMASK(nhashbits);
	count = cf[filter_idx]->count_key_value(dup_k, flag);

	return count;
}

template <class key_object>
void PopcornFilter<key_object>::print_stats(void) const {
	for (uint32_t i = 0; i < nfilters; i++) {
		PRINT_CF("CascadeFilter " << i);
		cf[i]->print_anomaly_stats();
	}
}

template <class key_object>
bool PopcornFilter<key_object>::validate_anomalies(
								std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>>
								key_lifetime) {
	std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>>
		per_filter[nfilters];
	for (auto it : key_lifetime) {
		if (it.second.first < it.second.second) {
			uint32_t filter_idx = it.first >> nhashbits;
			uint64_t key = it.first & BITMASK(nhashbits);
			per_filter[filter_idx][key] = it.second;
		}
	}
	for (uint32_t i = 0; i < nfilters; i++)
		if (!cf[i]->validate_key_lifetimes(per_filter[i]))
			return false;
	return true;
}

#endif
