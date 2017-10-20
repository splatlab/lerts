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

#include "cascadefilter.h"

#define NUM_MAX_FILTERS 10

template <class key_object>
class PopcornFilter {
	public:
		PopcornFilter(uint64_t nfilters, uint32_t nthreads, uint32_t qbits, uint32_t
									nlevels, uint32_t gfactor);

		bool insert(const key_object& k, enum lock flag);

		uint64_t query(const key_object& k) const;

		uint32_t get_num_hash_bits(void) const;

		uint32_t get_seed(void) const;

		uint64_t get_range(void) const;

		uint64_t get_max_size(void) const;

		uint64_t get_total_elements(void) const;

	private:
		uint64_t nfilters;
		uint32_t nthreads;
		uint32_t qbits;
		uint32_t nlevels;
		uint32_t gfactor;
		uint32_t fbits;
		uint32_t nhashbits;
		CascadeFilter<key_object> *cf[NUM_MAX_FILTERS];
};

template <class key_object>
class ThreadArgs {
	public:
		key_object *vals;
		uint64_t start;
		uint64_t end;
		PopcornFilter<key_object> *pf;

		ThreadArgs(PopcornFilter<key_object> *pf, key_object *vals, uint64_t start,
							 uint64_t end) : pf(pf), vals(vals), start(start), end(end) {};
};

#endif
