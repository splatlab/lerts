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
#include "util.h"

#include "cqf/gqf.h"

#define NUM_MAX_LEVELS 10

template <class key_object>
class CascadeFilter {
	public:
		CascadeFilter(uint32_t nhashbits, uint32_t filter_thlds[],
									uint64_t filter_sizes[], uint32_t num_filters);

		const QF* get_filter(uint32_t level) const;

		uint64_t get_num_elements(void) const;
		uint32_t get_num_hash_bits(void) const;
		uint32_t get_seed(void) const;
		uint64_t get_max_size(void) const;

		/* Increment the counter for this key/value pair by count. */
		bool insert(const key_object& key_val_cnt, enum lock flag);

		/* Remove count instances of this key/value combination. */
		bool remove(const key_object& key_val_cnt, enum lock flag);

		/* Replace the association (key, oldvalue, count) with the association
			 (key, newvalue, count). If there is already an association (key,
			 newvalue, count'), then the two associations will be merged and
			 their counters will be summed, resulting in association (key,
			 newvalue, count' + count). */
		void replace(const key_object& key_val_cnt, uint64_t newvalue);

		/* Return the number of times key has been inserted, with the given
			 value, into the cascade qf. */
		uint64_t count_key_value(const key_object& key_val_cnt);

		class Iterator {
			public:
				Iterator(QFi arr[], uint32_t num_levels, uint32_t cur_level);

				/* Returns 0 if the iterator is still valid (i.e. has not reached the
					 end of the QF. */
				key_object operator*(void) const;

				/* Advance to next entry.  Returns whether or not another entry is
					 found.  */
				void operator++(void);

				/* Return true if its the end of the iterator. */
				bool done() const;

			private:

				QFi qfi_arr[NUM_MAX_LEVELS];
				uint32_t iter_num_levels;
				uint32_t iter_cur_level;
		};

		Iterator begin(uint32_t num_levels) const;
		Iterator end(void) const;

	private:
		/**
		 * Check if the filter at "level" has exceeded the threshold load factor.
		 */
		bool is_full(uint32_t level) const;

		/**
		 * Returns the index of the first empty level.
		 */
		uint32_t find_first_empty_level();

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

		/**
		 * Spread the count of the key in the cascade filter.
		 */
		void smear_element(QF qf_arr[], key_object k, uint32_t nlevels);

		/**
		 * Try to acquire a lock once and return even if the lock is busy.
		 * If spin flag is set, then spin until the lock is available.
		 */
		bool lock(enum lock flag);

		void unlock(void);

		QF filters[NUM_MAX_LEVELS];
		uint32_t thresholds[NUM_MAX_LEVELS];
		uint64_t sizes[NUM_MAX_LEVELS];
		uint32_t total_num_levels;
		uint32_t num_flush;
		uint32_t num_hash_bits;
		uint32_t seed;
		volatile int locked;
};

class KeyObject {
	public:
		KeyObject() : key(0), value(0), count(0), level(0) {};

		KeyObject(uint64_t k, uint64_t v, uint64_t c, uint32_t l) : key(k),
		value(v), count(c), level(l) {};

		KeyObject(const KeyObject& k) : key(k.key), value(k.value), count(k.count),
		level(k.level) {};

		bool operator==(KeyObject k) { return key == k.key; }

		uint64_t key;
		uint64_t value;
		uint64_t count;
		uint32_t level;
};

template <class key_object = KeyObject>
bool operator!=(const typename CascadeFilter<key_object>::Iterator& a, const
								typename CascadeFilter<key_object>::Iterator& b);

template <class key_object>
CascadeFilter<key_object>::CascadeFilter(uint32_t nhashbits, uint32_t
																				 filter_thlds[], uint64_t
																				 filter_sizes[], uint32_t num_filters)
{
	total_num_levels = num_filters;
	num_hash_bits = nhashbits;
	num_flush = 0;
	seed = time(NULL);
	locked = 0;
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
uint32_t CascadeFilter<key_object>::get_num_hash_bits(void) const {
	return num_hash_bits;
}

template <class key_object>
uint32_t CascadeFilter<key_object>::get_seed(void) const {
	return seed;
}

template <class key_object>
uint64_t CascadeFilter<key_object>::get_max_size(void) const {
	return sizes[total_num_levels - 1];
}

template <class key_object>
uint64_t CascadeFilter<key_object>::get_num_elements(void) const {
	uint64_t total_count = 0;
	for (uint32_t i = 0; i < total_num_levels; i++)
		total_count += get_filter(i)->metadata->nelts;
	return total_count;
}

template <class key_object>
bool CascadeFilter<key_object>::lock(enum lock flag)
{
	if (flag != LOCK_AND_SPIN) {
		return !__sync_lock_test_and_set(&locked, 1);
	} else {
		while (__sync_lock_test_and_set(&locked, 1))
			while (locked);
		return true;
	}

	return false;
}

template <class key_object>
void CascadeFilter<key_object>::unlock(void)
{
	__sync_lock_release(&locked);
	return;
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
	QFi qfi_arr[NUM_MAX_LEVELS];

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
	if (flag != NO_LOCK)
		if (!lock(flag))
			return false;

	if (is_full(0)) {
		num_flush++;
		DEBUG_CF("Flusing " << num_flush);
		//merge();
		shuffle_merge();
	}

	/* we don't need a lock on the in-memory CQF while inserting. However, I
	 * still have the flag "LOCK_AND_SPIN" here just to be extra sure that we
	 * will not corrupt the data structure.
	 */
	bool ret = qf_insert(&filters[0], k.key, k.value, k.count, LOCK_AND_SPIN);

	if (flag != NO_LOCK)
		unlock();

	return ret;
}

template <class key_object>
bool CascadeFilter<key_object>::remove(const key_object& k, enum lock flag) {
	if (flag != NO_LOCK)
		if (!lock(flag))
			return false;

	for (uint32_t i = 0; i < total_num_levels; i++)
		qf_remove(&filters[i], k.key, k.value, k.count, LOCK_AND_SPIN);

	if (flag != NO_LOCK)
		unlock();

	return true;
}

template <class key_object>
uint64_t CascadeFilter<key_object>::count_key_value(const key_object& k) {
	lock(LOCK_AND_SPIN);

	uint64_t count = 0;
	for (uint32_t i = 0; i < total_num_levels; i++)
		count += qf_count_key_value(get_filter(i), k.key, k.value);

	unlock();
	return count;
}

#endif
