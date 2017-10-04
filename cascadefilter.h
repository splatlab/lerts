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

#include "threadsafe-gqf/gqf.h"

#define NUM_MAX_FILTERS 10

class CascadeFilter {
	public:
		CascadeFilter(uint32_t nhashbits, uint32_t filter_thlds[],
									uint64_t filter_sizes[], uint32_t num_filters);

		const QF* get_filter(uint32_t level) const;

		/* Increment the counter for this key/value pair by count. */
		bool insert(QF *qf, uint64_t key, uint64_t value, uint64_t count, bool
								lock, bool spin);

		/* Remove count instances of this key/value combination. */
		void remove(QF *qf, uint64_t key, uint64_t value, uint64_t count, bool
								lock);

		/* Replace the association (key, oldvalue, count) with the association
			 (key, newvalue, count). If there is already an association (key,
			 newvalue, count'), then the two associations will be merged and
			 their counters will be summed, resulting in association (key,
			 newvalue, count' + count). */
		void replace(QF *qf, uint64_t key, uint64_t oldvalue, uint64_t newvalue);

		/* Return the number of times key has been inserted, with the given
			 value, into qf. */
		uint64_t count_key_value(const QF *qf, uint64_t key, uint64_t value) const;

	private:
		/**
		 * Perform a shuffle-merge among @nqf cqfs from @qf_arr and put elements in
		 * new cqfs in @qf_arr_new.
		 *
		 * After the shuffle-merge the cqfs in @qf_arr will be destroyed and memory
		 * will be freed.
		 */
		void shuffle_merge(uint32_t num_levels);

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
