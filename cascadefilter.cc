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
#include "hashutil.h"
#include "zipf.h"
#include "util.h"

template <class key_object>
CascadeFilter<key_object>::CascadeFilter(uint32_t nhashbits, uint32_t
																				 filter_thlds[], uint64_t
																				 filter_sizes[], uint32_t num_filters)
{
	total_num_levels = num_filters;
	num_hash_bits = nhashbits;
	num_flush = 0;
	seed = time(NULL);
	memcpy(thresholds, filter_thlds, num_filters * sizeof(thresholds[0]));
	memcpy(sizes, filter_sizes, num_filters * sizeof(sizes[0]));

	/* Initialize all the filters. */
	//TODO: (prashant) Maybe make this a lazy initilization.
	for (uint32_t i = 0; i < total_num_levels; i++) {
		DEBUG_CF("Creating level: " << i << " of " << sizes[i] <<
						 " slots and threshold " << thresholds[i]);
		std::string file("_cqf.ser");
		file = "raw/" + std::to_string(i) + file;
		qf_init(&filters[i], sizes[i], num_hash_bits, 0, /*mem*/ false,
						file.c_str(), seed);
	}
}

template <class key_object>
const QF* CascadeFilter<key_object>::get_filter(uint32_t level) const {
	return &filters[level];
}

template <class key_object>
uint64_t CascadeFilter<key_object>::get_num_elements(void) const {
	uint64_t total_count = 0;
	for (uint32_t i = 0; i < total_num_levels; i++)
		total_count += get_filter(i)->metadata->nelts;
	return total_count;
}

template <class key_object>
bool CascadeFilter<key_object>::is_full(uint32_t level) const {
	double load_factor = get_filter(level)->metadata->noccupied_slots /
											 (double)get_filter(level)->metadata->nslots;
	if (load_factor > 0.75) {
		DEBUG_CF("Load factor: " << load_factor);
		return true;
	}
	return false;
}

template <class key_object>
uint32_t CascadeFilter<key_object>::find_first_empty_level() {
	uint32_t empty_level;
	uint64_t total_occupied_slots = get_filter(0)->metadata->noccupied_slots;
	for (empty_level = 1; empty_level < total_num_levels; empty_level++) {
		/* (prashant): This is a upper-bound on the number of slots that are
		 * needed in the empty level for the shuffle-merge to finish successfully.
		 * We can probably give a little slack in the constraints here.
		 */
		uint64_t available_slots = 	get_filter(empty_level)->metadata->nslots -
			get_filter(empty_level)->metadata->noccupied_slots;
		if (!is_full(empty_level) && total_occupied_slots <= available_slots)
			break;
		total_occupied_slots += get_filter(empty_level)->metadata->noccupied_slots;
	}
	/* If found an empty level. Merge all levels before the empty level into the
	 * empty level. Else create a new level and merge all the levels. */
	uint32_t nlevels;
	if (empty_level < total_num_levels)
		nlevels = empty_level;
	else {
		/* This is for lazy initialization of CQFs in the levels.
		 * Assuing we are creating all the levels in the constructor, code below
		 * will never be called. */
		std::string file("_cqf.ser");
		file = "raw/" + std::to_string(total_num_levels) + file;
		qf_init(&filters[total_num_levels], sizes[total_num_levels],
						num_hash_bits, 0, /*mem*/ false, file.c_str(), seed);
		nlevels = total_num_levels;
		total_num_levels++;
	}

	return nlevels;
}

template <class key_object>
void CascadeFilter<key_object>::smear_element(QF qf_arr[], key_object k,
																							uint32_t nlevels) {
	for (int32_t i = nlevels - 1; i > 0; i--) {
		if (k.count > thresholds[i]) {
			qf_insert(&qf_arr[i], k.key, k.value, thresholds[i],
								LOCK_AND_SPIN);
			k.count -= thresholds[i];
		} else {
			qf_insert(&qf_arr[i], k.key, k.value, k.count, LOCK_AND_SPIN);
			k.count = 0;
			break;
		}
	}
	/* If some observations are left then insert them in the first level. */
	if (k.count > 0)
		qf_insert(&qf_arr[0], k.key, k.value, k.count, LOCK_AND_SPIN);
}

template <class key_object>
CascadeFilter<key_object>::Iterator::Iterator(QFi arr[], uint32_t num_levels,
																							uint32_t cur_level) :
	iter_num_levels(num_levels), iter_cur_level(cur_level) {
		memcpy(qfi_arr, arr,  num_levels * sizeof(QFi));
	};

template <class key_object>
typename CascadeFilter<key_object>::Iterator
CascadeFilter<key_object>::begin(uint32_t num_levels) const {
	QFi qfi_arr[NUM_MAX_FILTERS];

	/* Initialize the iterator for all the levels. */
	for (uint32_t i = 0; i < num_levels; i++)
		qf_iterator(get_filter(i), &qfi_arr[i], 0);

	/* Find the level with the smallest key. */
	uint32_t cur_level = 0;
	uint64_t key, value, count;
	uint64_t smallest_key = UINT64_MAX;
	for (uint32_t i = 0; i < num_levels; i++)
		if (!qfi_end(&qfi_arr[i])) {
			qfi_get(&qfi_arr[i], &key, &value, &count);
			if (key < smallest_key)
				cur_level = i;
		} else {
			/* remove the qf iterator that has already reached the end. */
			memmove(&qfi_arr[i], &qfi_arr[i + 1], (num_levels - i -
																						 1) * sizeof(qfi_arr[0]));
			num_levels--;
		}

	return Iterator(qfi_arr, num_levels, cur_level);
}

template <class key_object>
typename CascadeFilter<key_object>::Iterator
CascadeFilter<key_object>::end() const {
	QFi qfi_arr[1];

	/* Initialize the iterator for all the levels. */
	qf_iterator(get_filter(0), &qfi_arr[0],
							0xffffffffffffffff);

	return Iterator(qfi_arr, 1, 0);
}

template <class key_object>
key_object CascadeFilter<key_object>::Iterator::operator*(void) const {
	uint64_t key, value, count;
	qfi_get(&qfi_arr[iter_cur_level], &key, &value, &count);
	key_object k(key, value, count, iter_cur_level);
	return k;
}

template <class key_object>
void CascadeFilter<key_object>::Iterator::operator++(void) {
	assert(iter_cur_level < iter_num_levels);

	/* Move the iterator for "iter_cur_level". */
	qfi_next(&qfi_arr[iter_cur_level]);

	// End of the cascade filter. 
	if (iter_num_levels == 1 && qfi_end(&qfi_arr[iter_cur_level]))
		return;

	/* remove the qf iterator that is exhausted from the array if it is not
	 * the last level. */
	if (iter_num_levels > 1 && qfi_end(&qfi_arr[iter_cur_level])) {
		if (iter_cur_level < iter_num_levels - 1)
			memmove(&qfi_arr[iter_cur_level], &qfi_arr[iter_cur_level + 1],
							(iter_num_levels - iter_cur_level - 1)*sizeof(qfi_arr[0]));
		iter_num_levels--;
	}

	/* Find the smallest key across levels and update "iter_cur_level". */
	uint64_t key, value, count;
	uint64_t smallest_key = UINT64_MAX;
	for (uint32_t i = 0; i < iter_num_levels; i++)
		if (!qfi_end(&qfi_arr[i])) {
			qfi_get(&qfi_arr[i], &key, &value, &count);
			if (key < smallest_key)
				iter_cur_level = i;
		}
}

template <class key_object>
bool CascadeFilter<key_object>::Iterator::done() const {
	assert(iter_cur_level < iter_num_levels);
	if (iter_num_levels == 1 && qfi_end(&qfi_arr[iter_cur_level]))
		return true;
	return false;
}

template <class key_object>
bool operator!=(const typename CascadeFilter<key_object>::Iterator& a, const
								typename CascadeFilter<key_object>::Iterator& b) {
	return !a.done() || !b.done();
}

template <class key_object>
void CascadeFilter<key_object>::merge() {
	uint32_t nlevels = find_first_empty_level();
	assert(nlevels <= total_num_levels);

	QF *to_merge[nlevels];
	for (uint32_t i = 0; i < nlevels; i++)
		to_merge[i] = get_filter(i);
	DEBUG_CF("Merging CQFs 0 to " << nlevels - 1 << " into the CQF "
					 << nlevels);
	/* If the in-mem filter needs to be merged to the first level on disk. */
	if (filters[nlevels].metadata->size ==
			to_merge[0]->metadata->size && nlevels == 1)
		qf_copy(&filters[nlevels], to_merge[0]);
	else
		qf_multi_merge(to_merge, nlevels, &filters[nlevels],
									 LOCK_AND_SPIN);

	/* Reset the filter that were merged. */
	for (uint32_t i = 0; i < nlevels; i++)
		qf_reset(&filters[i]);
}

template <class key_object>
void CascadeFilter<key_object>::shuffle_merge() {
	/* The empty level is also involved in the shuffle merge. */
	uint32_t nlevels = find_first_empty_level() + 1;
	assert(nlevels <= total_num_levels);
	DEBUG_CF("Shuffle merging CQFs 0 to " << nlevels - 1);

	KeyObject cur_key, next_key;
	QF new_filters[nlevels];

	/* Initialize new filters. */
	for (uint32_t i = 0; i < nlevels; i++) {
		std::string file("_cqf.ser");
		file = "raw/" + std::to_string(num_flush) + "_" + std::to_string(i) + file;
		qf_init(&new_filters[i], sizes[i], num_hash_bits, 0, /*mem*/ false,
						file.c_str(), seed);
	}

	DEBUG_CF("Old CQFs");
	for (uint32_t i = 0; i < nlevels; i++) {
		DEBUG_CF("CQF " << i << " threshold " << thresholds[i]);
		DEBUG_DUMP(&filters[i]);
	}

	/* Initialize cascade filter iterator. */
	CascadeFilter<key_object>::Iterator it = begin(nlevels);
	cur_key = *it;
	++it;

	do {
		next_key = *it;
		/* If next_key is same as cur_key then aggregate counts.
		 * Else, smear the count across levels starting from the bottom one.
		 * */
		if (cur_key == next_key)
			cur_key.count += next_key.count;
		else {
			smear_element(new_filters, cur_key, nlevels);
			/* Update cur_key. */
			cur_key = next_key;
		}
		/* Increment the iterator. */
		++it;
	} while(!it.done());

	/* Insert the last key in the cascade filter. */
	smear_element(new_filters, cur_key, nlevels);

	DEBUG_CF("New CQFs");
	for (uint32_t i = 0; i < nlevels; i++) {
		DEBUG_CF("CQF " << i);
		DEBUG_DUMP(&new_filters[i]);
	}

	/* Destroy the existing filters and replace them with the new filters. */
	for (uint32_t i = 0; i < nlevels; i++) {
		qf_destroy(&filters[i], /*mem*/ false);
		memcpy(&filters[i], &new_filters[i], sizeof(QF));
	}
}

template <class key_object>
bool CascadeFilter<key_object>::insert(const key_object& k, enum lock flag) {
	if (is_full(0)) {
		num_flush++;
		//merge();
		shuffle_merge();
	}

	qf_insert(&filters[0], k.key, k.value, k.count, flag);
	return true;
}

template <class key_object>
void CascadeFilter<key_object>::remove(const key_object& k, enum lock flag) {
	for (uint32_t i = 0; i < total_num_levels; i++)
		qf_remove(&filters[i], k.key, k.value, k.count, flag);
}

template <class key_object>
uint64_t CascadeFilter<key_object>::count_key_value(const key_object& k) const {
	uint64_t count = 0;
	for (uint32_t i = 0; i < total_num_levels; i++)
		count += qf_count_key_value(get_filter(i), k.key, k.value);
	return count;
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
	uint32_t randominput = 1;	// Default value is uniform-random distribution.
	if (argc > 4)
		randominput = atoi(argv[4]);

	uint64_t sizes[nfilters];
	uint32_t thlds[nfilters];

	struct timeval start, end;
	struct timezone tzp;

	/* level sizes grow by a factor "r". */
	sizes[0] = (1ULL << qbits);
	for (uint32_t i = 1; i < nfilters; i++)
		sizes[i] = pow(gfactor, i) * sizes[0];

	uint64_t nvals = 750 * (sizes[nfilters - 1]) / 1000;

	thlds[nfilters - 1] = 1;
	uint32_t j = 1;
	/* taus grow with r^0.5. */
	uint32_t tau_ratio = sqrt(gfactor);
	for (int32_t i = nfilters - 2; i >= 0; i--, j++)
		thlds[i] = pow(tau_ratio, j) * thlds[nfilters - 1];

	/* Create a cascade filter. */
	std::cout << "Create a cascade filter with " << nhashbits << "-bit hashes, "
		<< nfilters << " levels, and " << gfactor << " as growth factor." <<
		std::endl;
	CascadeFilter<KeyObject> cf(nhashbits, thlds, sizes, nfilters);

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
	//qf_init(&new_cf, sizes[nfilters - 1], nhashbits, 0, [>mem<] true,
					//"", 0);
	//for (auto it = cf.begin(nfilters); it != cf.end(); ++it) {
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

