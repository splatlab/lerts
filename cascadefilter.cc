/*
 * ============================================================================
 *
 *       Filename:  cascadefilter.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017-10-01 11:38:44 PM
 *       Revision:  none
 *       Compiler:  gcc
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

#include "cascadefilter.h"

#include "hashutil.h"
#include "util.h"


CascadeFilter::CascadeFilter(uint32_t num_hash_bits, uint32_t filter_thlds[],
									uint64_t filter_sizes[], uint32_t num_filters) {
	num_levels = num_filters;
	memcpy(thresholds, filter_thlds, num_filters * sizeof(thresholds[0]));
	memcpy(sizes, filter_sizes, num_filters * sizeof(sizes[0]));

	/* Initialize all the filters. */
	//TODO: (prashant) Make it laze initialization.
	uint32_t seed = time(NULL);
	for (uint32_t i = 0; i < num_levels; i++) {
		std::string file("_cqf.ser");
		file = std::to_string(i) + file;
		qf_init(&filters[i], sizes[i], num_hash_bits, 0, false, file.c_str(), seed);
	}
}

const QF* CascadeFilter::get_filter(uint32_t level) const {
	return &filters[level];
}

CascadeFilterIterator::CascadeFilterIterator(
								const CascadeFilter *cascade_filter,
								uint32_t num_levels) {
	cf = cascade_filter;
	cur_num_levels = num_levels;

	/* Initialize the iterator for all the levels. */
	for (int i = 0; i < cur_num_levels; i++)
		qf_iterator(cf->get_filter(i), &qfi_arr[i], 0);

	/* Find the smallest key across levels. */
	uint64_t key, value, count;
	uint64_t smallest_key = UINT64_MAX;
	for (int i = 0; i < cur_num_levels; i++) {
		qfi_get(&qfi_arr[i], &key, &value, &count);
		if (key < smallest_key)
			cur_level = i;
	}
}

int CascadeFilterIterator::get(uint64_t *key, uint64_t *value, uint64_t
															 *count, uint32_t *level) {
	*level = cur_level;
	return qfi_get(&qfi_arr[cur_level], key, value, count);
}

int CascadeFilterIterator::next() {
	assert(cur_level < cur_num_levels);

	/* Move the iterator for "cur_level". */
	qfi_next(&qfi_arr[cur_level]);

	/* End of the cascade filter. */
	if (cur_num_levels == 1 && qfi_end(&qfi_arr[cur_level]))
		return -1;

	/* remove the qf iterator that is exhausted from the array if it is not
	 * the last level. */
	if (cur_num_levels > 1 && qfi_end(&qfi_arr[cur_level])) {
		if (cur_level < cur_num_levels - 1)
			memmove(&qfi_arr[cur_level], &qfi_arr[cur_level + 1],
							(cur_num_levels - cur_level - 1)*sizeof(qfi_arr[0]));
		cur_num_levels--;
	}

	/* Find the smallest key across levels. */
	uint64_t key, value, count;
	uint64_t smallest_key = UINT64_MAX;
	for (int i = 0; i < cur_num_levels; i++) {
		qfi_get(&qfi_arr[i], &key, &value, &count);
		if (key < smallest_key)
			cur_level = i;
	}

	return 0;
}

int CascadeFilterIterator::end() {
	assert(cur_level < cur_num_levels);
	if (cur_num_levels == 1 && qfi_end(&qfi_arr[cur_level]))
		return 1;
	return 0;
}

#if 0
bool CascadeFilter::insert(QF *qf, uint64_t key, uint64_t value, uint64_t
													 count, bool lock, bool spin);

void CascadeFilter::remove(QF *qf, uint64_t key, uint64_t value, uint64_t
													 count, bool lock);

void CascadeFilter::replace(QF *qf, uint64_t key, uint64_t oldvalue, uint64_t
														newvalue);

uint64_t CascadeFilter::count_key_value(const QF *qf, uint64_t key, uint64_t
																				value) const;

void CascadeFilter::shuffle_merge(uint8_t nqf);





/**
 * This function will return the smallest key greater than @prev in the QF.
 * Returns -1 if @prev is not found. 
 * Returns 0 if @prev is found and sets @key, @value, and @count.
 */
int qf_succ(const QF *qf, uint64_t prev, uint64_t *key, uint64_t *value,
						uint64_t *count)
{
	__uint128_t hash = prev;
	uint64_t hash_remainder   = hash & BITMASK(qf->metadata->bits_per_slot);
	int64_t hash_bucket_index = hash >> qf->metadata->bits_per_slot;

	if (!is_occupied(qf, hash_bucket_index))
		return -1;

	int64_t runstart_index = hash_bucket_index == 0 ? 0 : run_end(qf,
																																hash_bucket_index-1) + 1;
	if (runstart_index < hash_bucket_index)
		runstart_index = hash_bucket_index;

	/* printf("MC RUNSTART: %02lx RUNEND: %02lx\n", runstart_index, runend_index); */

	uint64_t current_remainder, current_count, current_end;
	do {
		current_end = decode_counter(qf, runstart_index, &current_remainder,
																 &current_count);
		if (current_remainder == hash_remainder)
			return current_count;
		runstart_index = current_end + 1;
	} while (!is_runend(qf, current_end));

	return 0;
}

uint64_t qf_shuffle_merge_step(QF *qf_arr[], uint8_t thlds[], uint8_t nqf,
															 uint64_t prev, bool lock, bool spin)
{
	uint64_t min = UINT64_MAX;

	uint64_t keys[nqf];
	uint64_t values[nqf];
	uint64_t counts[nqf];

	/*Find the smallest key greater than @prev across each level.*/
	for (uint32_t i = 0; i <= nqf; i++) {
		qf_succ(qf_arr[i], prev, &keys[i], &values[i], &counts[i]);
		if (min > keys[i]) {
			min = keys[i];
		}
	}

	if (min == UINT64_MAX)
		return UINT64_MAX;

	/*Aggregate count of @min across levels and remove all it's occurrences from
	 * all levels.*/
	uint64_t count = 0;
	uint64_t value = 0;
	for (uint32_t i = 0; i <= nqf; i++) {
		if (min == keys[i]) {
			count += counts[i];
			value = values[i];
			qf_remove(qf_arr[i], keys[i], values[i],
								0, lock);
		}
	}

	/* Smear aggregated count of @min starting from the lowest level. */
	for (uint32_t i = nqf; i <= nqf; i++) {
		if (count >= thlds[i]) {
			qf_insert(qf_arr[i], min, value, thlds[i], lock, spin);
			count -= thlds[i];
		} else if (count > 0) {
			qf_insert(qf_arr[i], min, value, count, lock, spin);
		} else
			break;
	}

	return min;
}

/**
 * Perform a shuffle-merge among qfs in qf_arr.
 * Only hashes [hash_begin, hash_end] are involved in the shuffle-merge.
 * This function assumes that the qfs in qf_arr are sufficient to handle all
 * the elements <key, count> without violating the no-clogging constraint.
 * 
 * This shuffle merge depends on a stateless iterator, since we will be
 * performing the shuffle-merge in-place.
 */
void qf_shuffle_merge_inplace(QF *qf_arr[], uint8_t thlds[], uint8_t nqf, uint64_t
															hash_begin, uint64_t hash_end, bool lock, bool spin)
{
	uint64_t cur = hash_begin-1; 		//Need to handle the case when hash_begin is 0.

	while (cur <= hash_end) {
		cur = qf_shuffle_merge_step(qf_arr, thlds, nqf, cur, lock, spin);
		if (cur == UINT64_MAX)
			break;
	}
}

/**
 * Perform a shuffle-merge among @nqf cqfs from @qf_arr and put elements in
 * new cqfs in @qf_arr_new.
 *
 * After the shuffle-merge the cqfs in @qf_arr will be destroyed and memory
 * will be freed.
 *
 * This function assumes that the cqfs in @qf_arr_new have already been
 * initialized by the caller.
 */
void qf_shuffle_merge(QF *qf_arr[], uint8_t thlds[], uint8_t nqf, QF
											*qf_arr_new[], bool lock, bool spin) {
	QFi qfi_arr[nqf];
	int flag = 0;

	/* Initialize iterators. */
	for (int i=0; i<nqf; i++) {
		qf_iterator(qf_arr[i], &qfi_arr[i], 0);
	}

	while (!flag) {
		uint64_t keys[nqf];
		uint64_t values[nqf];
		uint64_t counts[nqf];
		int smallest_idx = 0;
		uint64_t smallest_key = UINT64_MAX;

		do {
			/* Get the current key from each level and find the smallest one among
			 * them and aggregate its count.
			 * */
			smallest_key = UINT64_MAX;
			for (int i=0; i<nqf; i++) {
				qfi_get(&qfi_arr[i], &keys[i], &values[i], &counts[i]);
				if (keys[i] < smallest_key) {
					smallest_key = keys[i];
					smallest_idx = i;
				} else if (keys[i] == smallest_key) {
					counts[smallest_idx] += counts[i];
				}
			}
			/* Smear aggregated count of smallest_key starting from the lowest
			 * level. */
			for (uint32_t i = nqf; i <= nqf; i++) {
				if (counts[smallest_idx] >= thlds[i]) {
					qf_insert(qf_arr_new[i], keys[smallest_idx], values[smallest_idx],
										thlds[i], lock, spin);
					counts[smallest_idx] -= thlds[i];
				} else if (counts[smallest_idx] > 0) {
					qf_insert(qf_arr_new[i], keys[smallest_idx], values[smallest_idx],
										counts[smallest_idx], lock, spin);
				} else
					break;
			}
			/* Increment the iterator on all the levels that had the smallest_key. */
			for (int i=0; i<nqf; i++) {
				if (keys[i] == smallest_key) {
					qfi_next(&qfi_arr[smallest_idx]);
					qfi_get(&qfi_arr[smallest_idx], &keys[smallest_idx],
									&values[smallest_idx], &counts[smallest_idx]);
				}
			}
		} while(!qfi_end(&qfi_arr[smallest_idx]));
					/* remove the cqf that is exhausted from the array. */
					if (smallest_idx < nqf-1)
						memmove(&qfi_arr[smallest_idx], &qfi_arr[smallest_idx+1],
										(nqf-smallest_idx-1)*sizeof(qfi_arr[0]));
					nqf--;
					if (nqf == 1)
						flag = 1;
	}

	if (!qfi_end(&qfi_arr[0])) {
		do {
			uint64_t key, value, count;
			qfi_get(&qfi_arr[0], &key, &value, &count);
			/* Smear aggregated count of smallest_key starting from the lowest
			 * level. */
			for (uint32_t i = nqf; i <= nqf; i++) {
				if (count >= thlds[i]) {
					qf_insert(qf_arr_new[i], key, value, thlds[i], lock, spin);
					count -= thlds[i];
				} else if (count > 0) {
					qf_insert(qf_arr_new[i], key, value, count, lock, spin);
				} else
					break;
			}
		} while(!qfi_next(&qfi_arr[0]));
	}
}
#endif


/* 
 * ===  FUNCTION  =============================================================
 *         Name:  main
 *  Description:  
 * ===========================================================================
 */
	int
main ( int argc, char *argv[] )
{
	return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */
