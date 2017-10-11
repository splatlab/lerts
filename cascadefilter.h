/*
 * ============================================================================
 *
 *       Filename:  cascadefilter.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017-10-01 11:40:01 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Prashant Pandey (), ppandey@cs.stonybrook.edu
 *   Organization:  Stony Brook University
 *
 * ============================================================================
 */

#ifndef _CASCADEFILTER_H_
#define _CASCADEFILTER_H_

#include <sys/types.h>

#include "cqf/gqf.h"

#define NUM_MAX_FILTERS 10

class CascadeFilter {
	public:
		CascadeFilter(uint32_t nhashbits, uint32_t filter_thlds[],
									uint64_t filter_sizes[], uint32_t num_filters);

		const QF* get_filter(uint32_t level) const;

		/* Increment the counter for this key/value pair by count. */
		bool insert(uint64_t key, uint64_t value, uint64_t count, enum lock flag);

		/* Remove count instances of this key/value combination. */
		void remove(uint64_t key, uint64_t value, uint64_t count, enum lock flag);

		/* Replace the association (key, oldvalue, count) with the association
			 (key, newvalue, count). If there is already an association (key,
			 newvalue, count'), then the two associations will be merged and
			 their counters will be summed, resulting in association (key,
			 newvalue, count' + count). */
		void replace(uint64_t key, uint64_t oldvalue, uint64_t newvalue);

		/* Return the number of times key has been inserted, with the given
			 value, into the cascade qf. */
		uint64_t count_key_value(uint64_t key, uint64_t value) const;

	private:
		/**
		 * Check if the filter at "level" has exceeded the threshold load factor.
		 */
		bool is_full(uint32_t level) const;
	
		/**
		 * Returns the number of levels to merge based on the first empty level.
		 */
		uint32_t num_levels_to_merge();

		/** Perform a standard cascade filter merge. It merges "num_levels" levels
		 * and inserts all the elements into a new level.  Merge will be called
		 * whenever in-mem level is full.
     *
		 * Approach: Make the in-memory level as large as possible w/o swapping.
		 * Each on-disk level should be twice as large as the previous level.  The
		 * number of levels should be unconstrained, i.e. make a new level
		 * whenever all the existing levels become full.
		 *
		 * For the cascade filter, the size of the first level, i.e., the level in
		 * RAM and the second level, i.e., the first level on-disk should same.
		 * The rest of the levels on-disk grow exponentially in size.
     */
		void merge();

		/**
		 * Perform a shuffle-merge among @nqf cqfs from @qf_arr and put elements in
		 * new cqfs in @qf_arr_new.
		 *
		 * After the shuffle-merge the cqfs in @qf_arr will be destroyed and memory
		 * will be freed.
		 */
		void shuffle_merge();

		QF filters[NUM_MAX_FILTERS];
		uint32_t thresholds[NUM_MAX_FILTERS];
		uint64_t sizes[NUM_MAX_FILTERS];
		uint32_t total_num_levels;
		uint32_t num_hash_bits;
		uint32_t seed;
};

class CascadeFilterIterator {
	public:
		CascadeFilterIterator(const CascadeFilter *cascade_filter,
													uint32_t num_levels);

		/* Returns 0 if the iterator is still valid (i.e. has not reached the
			 end of the QF. */
		int get(uint64_t *key, uint64_t *value, uint64_t *count, uint32_t *level);

		/* Advance to next entry.  Returns whether or not another entry is
			 found.  */
		int next();

		/* Check to see if the if the end of the QF */
		int end();

	private:
		const CascadeFilter *cf;
		QFi qfi_arr[NUM_MAX_FILTERS];
		uint32_t cur_num_levels;
		uint32_t cur_level;
};

#endif
