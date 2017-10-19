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

		bool insert(key_object k);

		uint64_t query(key_object k);

		uint32_t get_num_hash_bits(void);

		uint32_t get_seed(void);

	private:
		uint32_t qbits;
		uint32_t nlevels;
		uint32_t gfactor;
		uint64_t nfilters;
		uint32_t nthreads;
		uint32_t nhashbits;
		CascadeFilter<key_object> *cf[NUM_MAX_FILTERS];
};

template <class key_object>
class ThreadArgs {
	public:
		key_object *vals;
		uint64_t start;
		uint64_t end;

		ThreadArgs(key_object *vals, uint64_t start,
							 uint64_t end) : vals(vals), start(start), end(end) {};
};

#endif
