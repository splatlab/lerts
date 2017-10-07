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
#include "util.h"

CascadeFilter::CascadeFilter(uint32_t nhashbits, uint32_t filter_thlds[],
									uint64_t filter_sizes[], uint32_t num_filters) {
	total_num_levels = num_filters;
	num_hash_bits = nhashbits;
	seed = time(NULL);
	memcpy(thresholds, filter_thlds, num_filters * sizeof(thresholds[0]));
	memcpy(sizes, filter_sizes, num_filters * sizeof(sizes[0]));

	/* Initialize all the filters. */
	//TODO: (prashant) Maybe make is a lazy initilization.
	for (uint32_t i = 0; i < total_num_levels; i++) {
		DEBUG_CF("Creating level: " << i << " of " << sizes[i] << " slots.");
		std::string file("_cqf.ser");
		file = "raw/" + std::to_string(i) + file;
		qf_init(&filters[i], sizes[i], num_hash_bits, 0, false, file.c_str(), seed);
	}
}

const QF* CascadeFilter::get_filter(uint32_t level) const {
	return &filters[level];
}

bool CascadeFilter::is_full(uint32_t level) {
	double load_factor = filters[level].metadata->noccupied_slots /
											 (double)filters[level].metadata->nslots;
	if (load_factor > 0.75) {
		DEBUG_CF("Load factor: " << load_factor);
		return true;
	}
	return false;
}

void CascadeFilter::merge() {
	uint32_t empty_level;
	for (empty_level = 1; empty_level < total_num_levels; empty_level++) {
		if (!is_full(empty_level))
			break;
	}
	/* If found an empty level. Merge all levels before the empty level into the
	 * empty level. Else create a new level and merge all the levels. */
	uint32_t num_levels_to_merge;
	if (empty_level < total_num_levels)
		num_levels_to_merge = empty_level;
	else {
		/* Assuing we are creating all the levels in the constructor. */
		//std::string file("_cqf.ser");
		//file = std::to_string(total_num_levels) + file;
		//qf_init(&filters[total_num_levels], sizes[total_num_levels],
						//num_hash_bits, 0, false, file.c_str(), seed);
		num_levels_to_merge = total_num_levels;
		total_num_levels++;
	}

	QF *to_merge[num_levels_to_merge];
	for (uint32_t i = 0; i < num_levels_to_merge; i++)
		to_merge[i] = &filters[i];
	DEBUG_CF("Merging CQFs 0 to " << num_levels_to_merge - 1 << " into the CQF "
					 << num_levels_to_merge);
	qf_multi_merge(to_merge, num_levels_to_merge, &filters[num_levels_to_merge],
								 true, true);

	/* Reset the filter that were merged. */
	for (uint32_t i = 0; i < num_levels_to_merge; i++)
		qf_reset(&filters[i]);
}

void CascadeFilter::shuffle_merge(uint32_t num_levels) {
	assert(num_levels < total_num_levels);

	uint64_t cur_key, cur_value, cur_count;
	uint64_t next_key, next_value, next_count;
	uint32_t cur_level, next_level;
	QF new_filters[num_levels];

	/* Initialize new filters. */
	//TODO: (prashant) Add a version number to file names.
	for (uint32_t i = 0; i < num_levels; i++) {
		std::string file("_cqf.ser");
		file = std::to_string(i) + file;
		qf_init(&new_filters[i], sizes[i], num_hash_bits, 0, false, file.c_str(),
						seed);
	}

	/* Initialize cascade filter iterator. */
	CascadeFilterIterator cfi(this, num_levels);
	cfi.get(&cur_key, &cur_value, &cur_count, &cur_level);
	
	while(!cfi.end()) {
		cfi.get(&next_key, &next_value, &next_count, &next_level);
		/* If next_key is same as cur_key then aggregate counts.
		 * Else, smear the count across levels starting from the bottom one.
		 * */
		if (next_key == cur_key)
			cur_count += next_count;
		else {
			for (int32_t i = num_levels; i >= 0; i--) {
				if (cur_count >= thresholds[i]) {
					qf_insert(&new_filters[i], cur_key, cur_value, thresholds[i], true,
										true);
					cur_count -= thresholds[i];
				} else if (cur_count > 0) {
					qf_insert(&new_filters[i], cur_key, cur_value, cur_count, true,
										true);
				} else
					break;
			}
		}
	}
}

bool CascadeFilter::insert(uint64_t key, uint64_t value, uint64_t count,
													 bool lock, bool spin) {
	if (is_full(0))
		merge();

	qf_insert(&filters[0], key, value, count, lock, spin);
	return true;
}

void CascadeFilter::remove(uint64_t key, uint64_t value, uint64_t count, bool
													 lock) {
	for (uint32_t i = 0; i < total_num_levels; i++)
		qf_remove(&filters[i], key, value, count, lock);
}

uint64_t CascadeFilter::count_key_value(uint64_t key, uint64_t value) const {
	uint64_t count = 0;
	for (uint32_t i = 0; i < total_num_levels; i++)
		count += qf_count_key_value(&filters[i], key, value);
	return count;
}

CascadeFilterIterator::CascadeFilterIterator(
								const CascadeFilter *cascade_filter,
								uint32_t num_levels) {
	cf = cascade_filter;
	cur_num_levels = num_levels;

	/* Initialize the iterator for all the levels. */
	for (uint32_t i = 0; i < cur_num_levels; i++)
		qf_iterator(cf->get_filter(i), &qfi_arr[i], 0);

	/* Find the smallest key across levels. */
	uint64_t key, value, count;
	uint64_t smallest_key = UINT64_MAX;
	for (uint32_t i = 0; i < cur_num_levels; i++) {
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

	/* Find the smallest key across levels and update "cur_level". */
	uint64_t key, value, count;
	uint64_t smallest_key = UINT64_MAX;
	for (uint32_t i = 0; i < cur_num_levels; i++) {
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

/* 
 * ===  FUNCTION  =============================================================
 *         Name:  main
 *  Description:  
 * ===========================================================================
 */
	int
main ( int argc, char *argv[] )
{
	if (argc < 4) {
		std::cout << "Not suffcient args." << std::endl;
		abort();
	}

	uint64_t qbits = atoi(argv[1]);
	uint32_t nfilters = atoi(argv[2]);
	uint32_t gfactor = atoi(argv[3]);
	uint32_t nhashbits = qbits + 10;

	uint64_t sizes[nfilters];
	uint32_t thlds[nfilters];

	struct timeval start, end;
	struct timezone tzp;
	
	sizes[0] = (1ULL << qbits);
	for (uint32_t i = 1; i < nfilters; i++) {
		sizes[i] = pow(gfactor, i - 1) * sizes[0];
	}
	uint64_t nvals = 750 * (sizes[nfilters - 1]) / 1000;

	/* Create a cascade filter. */
	std::cout << "Create a cascade filter with " << nhashbits << "-bit hashes, "
		<< nfilters << " levels, and " << gfactor << " as growth factor." <<
		std::endl;
	CascadeFilter cf(nhashbits, thlds, sizes, nfilters);

	__uint128_t *vals;
	vals = (__uint128_t*)malloc(nvals*sizeof(vals[0]));
	std::cout << "Generating " << nvals << " random numbers." << std::endl;
	memset(vals, 0, nvals*sizeof(vals[0]));
	RAND_pseudo_bytes((unsigned char *)vals, sizeof(*vals) * nvals);
	for (uint64_t k = 0; k < nvals; k++)
		vals[k] = (1 * vals[k]) % cf.get_filter(0)->metadata->range;

	std::cout << "Inserting elements." << std::endl;
	gettimeofday(&start, &tzp);
	for (uint64_t k = 0; k < nvals; k++)
		if (!cf.insert(vals[k], 0, 1, true, true)) {
			std::cerr << "Failed insertion for " <<
				(uint64_t)vals[k] << std::endl;
			abort();
		}
	gettimeofday(&end, &tzp);
	print_time_elapsed("", &start, &end);
	std::cout << "Finished insertions." << std::endl;

	std::cout << "Querying elements." << std::endl;
	gettimeofday(&start, &tzp);
	for (uint64_t k = 0; k < nvals; k++)
		if (cf.count_key_value(vals[k], 0) < 1) {
			std::cerr << "Failed lookup for " <<
				(uint64_t)vals[k] << std::endl;
			abort();
		}
	gettimeofday(&end, &tzp);
	print_time_elapsed("", &start, &end);
	std::cout << "Finished lookups." << std::endl;

	return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */

#if 0
//TODO: (prashant) delete code after this line when you know this won't be
//needed anymore.

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


