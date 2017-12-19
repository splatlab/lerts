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
#include <queue>
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

#include "cqf.h"

#define NUM_MAX_LEVELS 10
#define THRESHOLD_VALUE 2

template <class key_object>
class CascadeFilter {
	public:
		CascadeFilter(uint32_t nhashbits, uint32_t nvaluebits, uint32_t
									nagebits, bool odp, uint32_t filter_thlds[], uint64_t
									filter_sizes[], uint32_t num_levels, std::string& prefix);

		const CQF<key_object>* get_filter(uint32_t level) const;

		void print_anomaly_stats(void);
		uint64_t get_num_keys_above_threshold(void) const;
		uint64_t get_num_elements(void) const;
		uint64_t get_num_dist_elements(void) const;
		uint32_t get_num_hash_bits(void) const;
		uint32_t get_seed(void) const;
		uint64_t get_max_size(void) const;

		/* Increment the counter for this key/value pair by count. */
		bool insert(const key_object& key_val_cnt, uint64_t obs_cnt,
								enum lock flag);

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
		uint64_t count_key_value(const key_object& key_val_cnt, enum lock flag);

		bool validate_key_lifetimes(std::unordered_map<uint64_t,
																std::pair<uint64_t, uint64_t>> key_lifetime);

		class Iterator {
			public:
				Iterator(typename CQF<key_object>::Iterator arr[], uint32_t
								 num_levels);

				/* Returns 0 if the iterator is still valid (i.e. has not reached the
					 end of the QF. */
				key_object operator*(void) const;

				/* Advance to next entry.  Returns whether or not another entry is
					 found.  */
				void operator++(void);

				/* Return true if its the end of the iterator. */
				bool done() const;

			private:

				typename CQF<key_object>::Iterator *qfi_arr;
				std::priority_queue<key_object, std::vector<key_object>,
					compare<key_object>> min_heap;
				uint32_t iter_num_levels;
		};

		Iterator begin(uint32_t num_levels) const;
		Iterator end(void) const;

	private:
		/**
		 * Check if the filter at "level" has exceeded the threshold load factor.
		 */
		bool is_full(uint32_t level) const;

		/**
		 * Returns true if we need shuffle merge in the time stretch case.
		 */
		bool need_shuffle_merge_time_stretch(void) const;

		/**
		 * Returns true if the key is aged at the level it is in.
		 */
		bool is_aged(const key_object k) const;

		void increment_age(uint32_t level);

		/**
		 * Returns the index of the first empty level.
		 */
		uint32_t find_first_empty_level(void) const;

		/**
		 * Return the number of levels to merge into given "num_flush" . 
		 */
		uint32_t find_levels_to_flush() const;

		/**
		 * Update the flush counters.
		 */
		void update_flush_counters(uint32_t nlevels);

		/**
		 * report anomalies currently in the system.
		 */
		void find_anomalies(void);

		void perform_shuffle_merge_if_needed(void);

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
		 *
		 * It uses a fix scheule for merges. There are "r" merges to the first
		 * level on disk and "r^2" merges to the second level and so on.
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
		void smear_element(CQF<key_object> *qf_arr, key_object k, uint32_t
											 nlevels);

		/**
		 * Insert count in the last level.
		 */
		void insert_element(CQF<key_object> *qf_arr, key_object cur, uint32_t
												level);

		uint64_t ondisk_count(key_object k) const;


		CQF<key_object> *filters;
		CQF<key_object> anomalies;
		uint32_t total_num_levels;
		uint32_t num_hash_bits;
		uint32_t num_value_bits;
		uint32_t num_age_bits;
		bool odp;
		bool count_stretch;
		std::string prefix;
		uint32_t thresholds[NUM_MAX_LEVELS];
		uint64_t sizes[NUM_MAX_LEVELS];
		uint16_t flushes[NUM_MAX_LEVELS];
		uint8_t ages[NUM_MAX_LEVELS];
		uint32_t num_flush;
		uint32_t gfactor;
		uint32_t popcorn_threshold;
		uint32_t max_age;
		uint32_t num_obs_seen;
		uint32_t seed;
		LightweightLock cf_lw_lock;
};

template <class key_object = KeyObject>
bool operator!=(const typename CascadeFilter<key_object>::Iterator& a, const
								typename CascadeFilter<key_object>::Iterator& b);

template <class key_object>
CascadeFilter<key_object>::CascadeFilter(uint32_t nhashbits, uint32_t
																				 nvaluebits, uint32_t nagebits, bool
																				 odp, uint32_t filter_thlds[],
																				 uint64_t filter_sizes[], uint32_t
																				 num_levels, std::string& prefix) :
	total_num_levels(num_levels), num_hash_bits(nhashbits),
	num_age_bits(nagebits), odp(odp), prefix(prefix)
{
	num_value_bits = nvaluebits + nagebits;

	if (nagebits)
		max_age = 1 << nagebits;
	else
		max_age = 0;

	if (!odp && num_age_bits == 0)
		count_stretch = true;
	else
		count_stretch = false;

	num_flush = 0;
	gfactor = filter_sizes[1] / filter_sizes[0];
	popcorn_threshold = 0;
	num_obs_seen = 0;
	seed = 2038074761;
	memcpy(thresholds, filter_thlds, num_levels * sizeof(thresholds[0]));
	memcpy(sizes, filter_sizes, num_levels * sizeof(sizes[0]));
	memset(flushes, 0, num_levels *  sizeof(flushes[0]));
	memset(ages, 0, num_levels *  sizeof(ages[0]));

	// creating an exact CQF to store anomalies.
	uint64_t anomaly_filter_size = sizes[0] * 16;
	DEBUG_CF("Creating anomaly filter of " << anomaly_filter_size <<
					 " slots and THRESHOLD " << THRESHOLD_VALUE);
	anomalies = CQF<key_object>(anomaly_filter_size, num_hash_bits,
															num_value_bits, /*mem*/ true, "", seed);

	filters = (CQF<key_object>*)calloc(NUM_MAX_LEVELS, sizeof(CQF<key_object>));

	if (max_age == 0) {
		uint32_t sum_disk_threshold = 0;
		for (uint32_t i = 1; i < total_num_levels; i++)
			sum_disk_threshold += thresholds[i];
		popcorn_threshold = THRESHOLD_VALUE - sum_disk_threshold;
	}

	/* Initialize all the filters. */
	//TODO: (prashant) Maybe make this a lazy initilization.
	for (uint32_t i = 0; i < total_num_levels; i++) {
		DEBUG_CF("Creating level: " << i << " of " << sizes[i] <<
						 " slots and threshold " << thresholds[i]);
		std::string file_ext("_cqf.ser");
		std::string file = prefix + std::to_string(i) + file_ext;
		filters[i] = CQF<key_object>(sizes[i], num_hash_bits, num_value_bits,
																 /*mem*/ false, file.c_str(), seed);
	}
}

template <class key_object>
const CQF<key_object>* CascadeFilter<key_object>::get_filter(uint32_t level)
	const {
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
		total_count += get_filter(i)->total_elements();
	return total_count;
}

template <class key_object>
uint64_t CascadeFilter<key_object>::get_num_dist_elements(void) const {
	uint64_t total_count = 0;
	for (uint32_t i = 0; i < total_num_levels; i++)
		total_count += get_filter(i)->distinct_elements();
	return total_count;
}

template <class key_object>
void CascadeFilter<key_object>::print_anomaly_stats(void) {
	// find anomalies that are not reported yet.
	find_anomalies();
	uint64_t num_shuffle_merge = 0, num_odp = 0;
	typename CQF<key_object>::Iterator it = anomalies.begin();
	while (!it.done()) {
		key_object k = *it;
		if (k.value == 0) {
			num_shuffle_merge++;
			if (count_stretch)
				PRINT_CF("Shuffle-merge: " << k.key << " count: " << k.count);
			else
				PRINT_CF("Shuffle-merge: " << k.key << " index: " << k.count);
		} else if (k.value == 1) {
			num_odp++;
			PRINT_CF("ODP: " << k.key << " index: " << k.count);
		} else {
			PRINT_CF("Wrong value in the anomaly CQF.");
			abort();
		}
		++it;
	}

	PRINT_CF("Number of keys above the THRESHOLD value " <<
					 anomalies.distinct_elements());
	PRINT_CF("Number of keys reported through shuffle-merges " <<
					 num_shuffle_merge);
	PRINT_CF("Number of keys reported through odp " << num_odp);
}

template <class key_object>
uint64_t CascadeFilter<key_object>::get_num_keys_above_threshold(void) const {
	return anomalies.distinct_elements();
}

template <class key_object>
bool CascadeFilter<key_object>::validate_key_lifetimes(
								std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>>
								key_lifetime) {
	uint32_t failures = 0;
	std::ofstream result;

	// find anomalies that are not reported yet.
	if (!odp)
		shuffle_merge();
		//find_anomalies();

	DEBUG_CF("Anomaly CQF ");
	anomalies.dump_metadata();

	//PRINT_CF("Number of keys above threshold: " <<
	//get_num_keys_above_threshold());
	uint64_t idx = 0;
	if (max_age) {
		result.open("raw/time-stretch.data");
		//result << "x_0 y_0 y_1 y_2" << std::endl;
		result << "Key Inex-0 Index-2 ReportIndex Stretch" << std::endl;
		double stretch = 1 + 1 / num_age_bits;
		for (auto it : key_lifetime) {
			if (it.second.first < it.second.second) {
				uint64_t value;
				key_object k(it.first, 0, 0, 0);
				uint64_t lifetime = it.second.second - it.second.first;
				uint64_t reporttime = anomalies.query_key(k, &value) - it.second.first;
				if (reporttime > lifetime * stretch) {
					PRINT_CF("Time-stretch reporting failed Key: " << it.first <<
									 " Index-1: " << it.second.first << " Index-T " <<
									 it.second.second << " Reporting index " <<
									 anomalies.query_key(k, &value) << " for stretch " <<
									 stretch);
					failures++;
				}
				//result << idx++ << " " << lifetime << " " << reporttime << " " <<
					//lifetime * stretch << std::endl;
				result << it.first << " " << it.second.first << " " << it.second.second
					<< " " << anomalies.query_key(k, &value) << " " <<
					reporttime/(double)lifetime << std::endl;
			}
		}
	} else if (odp) {
		result.open("raw/immediate-reporting.data");
		result << "x_0 y_0 y_1" << std::endl;
		for (auto it : key_lifetime) {
			if (it.second.first < it.second.second) {
				uint64_t value;
				key_object k(it.first, 0, 0, 0);
				uint64_t reportindex = anomalies.query_key(k, &value);
				if (reportindex != it.second.second) {
					PRINT_CF("Immediate reporting failed Key: " << it.first <<
									 " Index-T " << it.second.second << " Reporting index "
									 << reportindex);
					failures++;
				}
				result << idx++ << " " << it.second.second << " " << reportindex <<
					std::endl;
			}
		}
	} else if (count_stretch) {
		result.open("raw/count-stretch.data");
		result << "x_0 y_0 y_1 y_2" << std::endl;
		// for count stretch the count stored with keys in anomalies is the actual
		// count at the time of reporting.
		for (auto it : key_lifetime) {
			if (it.second.first < it.second.second) {
				uint64_t value;
				key_object k(it.first, 0, 0, 0);
				uint64_t reportcount = anomalies.query_key(k, &value);
				if (reportcount > THRESHOLD_VALUE * 2) {
					PRINT_CF("Count stretch reporting failed Key: " << it.first <<
									 " Reporting count " << reportcount);
					failures++;
				}
				result << idx++ << " " << THRESHOLD_VALUE << " " << reportcount << " "
					<< THRESHOLD_VALUE * 2 << std::endl;
			}
		}
	} else {
		abort();
	}

	if (!failures)
		return true;
	else {
		PRINT_CF("Failed to report " << failures << " keys on time.");
		return false;
	}
	//result.close();
}

template <class key_object>
bool CascadeFilter<key_object>::is_full(uint32_t level) const {
	if (num_obs_seen == 0)
		return false;

	if (max_age) {
		// we do a flush when the number of observations seen is increased
		// by ram_size/2.
		uint64_t num_obs = get_filter(0)->total_slots()/max_age;
		if (num_obs_seen % num_obs == 0)
			return true;
	} else {
		double load_factor = get_filter(level)->occupied_slots() /
			(double)get_filter(level)->total_slots();
		if (load_factor > 0.90) {
			DEBUG_CF("Load factor: " << load_factor);
			return true;
		}
	}
	return false;
}

template <class key_object>
bool CascadeFilter<key_object>::need_shuffle_merge_time_stretch(void) const {
	uint64_t num_obs = get_filter(0)->total_slots()/max_age;
	int num_obs_frac = num_obs_seen/num_obs;
	if (num_obs_frac > 1)
		return true;
	return false;
}

template <class key_object>
void CascadeFilter<key_object>::increment_age(uint32_t level) {
	ages[level] = (ages[level] + 1) % max_age;
}

template <class key_object>
uint32_t CascadeFilter<key_object>::find_first_empty_level(void) const {
	uint32_t empty_level;
	uint64_t total_occupied_slots = get_filter(0)->occupied_slots();
	for (empty_level = 1; empty_level < total_num_levels; empty_level++) {
		/* (prashant): This is a upper-bound on the number of slots that are
		 * needed in the empty level for the shuffle-merge to finish successfully.
		 * We can probably give a little slack in the constraints here.
		 */
		uint64_t available_slots = 	get_filter(empty_level)->total_slots() -
			get_filter(empty_level)->occupied_slots();
		if (!is_full(empty_level) && total_occupied_slots <= available_slots)
			break;
		total_occupied_slots += get_filter(empty_level)->occupied_slots();
	}
	/* If found an empty level. Merge all levels before the empty level into the
	 * empty level. Else create a new level and merge all the levels. */
	uint32_t nlevels = 0;
	if (empty_level < total_num_levels)
		nlevels = empty_level;

	return nlevels;
}

template <class key_object>
uint32_t CascadeFilter<key_object>::find_levels_to_flush(void) const {
	uint32_t empty_level;
	for (empty_level = 1; empty_level < total_num_levels - 1; empty_level++) {
		if (flushes[empty_level] < gfactor - 1)
			break;
	}

	return empty_level;
}

template <class key_object>
void CascadeFilter<key_object>::perform_shuffle_merge_if_needed(void) {
	if (is_full(0)) {
		if (max_age) {
			// Age is increased for level 0 whenever it is 1/alpha full.
			ages[0] = (ages[0] + 1) % max_age;
			if (need_shuffle_merge_time_stretch()) {
				DEBUG_CF("Number of observations seen: " << num_obs_seen);
				DEBUG_CF("Flushing " << num_flush);
				shuffle_merge();
				// Increment the flushing count.
				num_flush++;
			}
		} else {
			DEBUG_CF("Flushing " << num_flush);
			shuffle_merge();
			// Increment the flushing count.
			num_flush++;
		}
	}
}

template <class key_object>
void CascadeFilter<key_object>::update_flush_counters(uint32_t nlevels) {
	for (uint32_t i = 1; i < nlevels; i++)
		flushes[i] = (flushes[i] + 1) % (gfactor);
}

// if the age of the level is changed (incremented) during this flush then
// all the keys are aged that have age equal to the current level age.
// if the age of level is not changed (incremented) during this flush then
// all the keys that have age one smaller than the current level age are
// aged.
template <class key_object>
bool CascadeFilter<key_object>::is_aged(const key_object k) const {
	uint8_t age = k.value & BITMASK(num_age_bits);
	if (age == ages[k.level])
		return true;
	return false;
}

template <class key_object>
void CascadeFilter<key_object>::smear_element(CQF<key_object> *qf_arr,
																							key_object k, uint32_t nlevels) {
	for (int32_t i = nlevels; i > 0; i--) {
		if (k.count > thresholds[i]) {
			key_object cur(k.key, k.value, thresholds[i], 0);
			qf_arr[i].insert(cur, LOCK_AND_SPIN);
			k.count -= thresholds[i];
		} else {
			if (max_age) {
				uint64_t value;
				uint64_t cur_count = filters[i].query_key(k, &value);
				// reset the value bits of the key.
				k.value = k.value & ~BITMASK(num_age_bits);
				if (cur_count) { // if key is already present then use the exisiting age.
					uint8_t cur_age = value & BITMASK(num_age_bits);
					k.value = k.value | cur_age;
				} else {	// assign the age based in the current num_flush of the level
					k.value = k.value | ages[i];
				}
			}
			qf_arr[i].insert(k, LOCK_AND_SPIN);
			k.count = 0;
			break;
		}
	}
	/* If some observations are left then insert them in the first level. */
	if (k.count > 0)
		qf_arr[0].insert(k, LOCK_AND_SPIN);
}

template <class key_object>
void CascadeFilter<key_object>::insert_element(CQF<key_object> *qf_arr,
																							key_object cur, uint32_t
																							nlevels) {
	uint64_t value;
	if (cur.count >= THRESHOLD_VALUE) {
		if (anomalies.query_key(cur, &value) == 0) {
			// the count is the index at which the key is reported.
			// for count-stretch the count is the current count of the key.
			if (!count_stretch)
				cur.count = num_obs_seen;
			// value 0 means that it is reported through shuffle-merge.
			cur.value = 0;
			if (anomalies.if_full())
				abort();
			anomalies.insert(cur, LOCK_AND_SPIN);
			DEBUG_CF("Reporting " << cur.to_string());
		}
	} else {
		if (max_age) {
			// if key is aged then flush it to the next level.
			if (cur.level < nlevels && is_aged(cur)) { // flush the key down.
				assert(cur.level < nlevels);
				smear_element(qf_arr, cur, cur.level + 1);
				//DEBUG_CF("Aged " << cur.to_string());
			}
			// not aged yet. reinsert the key with aggregated count in the lowest
			// level (involved in the shuffle-merge) it was present in.
			else {
				qf_arr[cur.level].insert(cur, LOCK_AND_SPIN);
				//DEBUG_CF("Live " << cur.to_string());
			}
		} else
			smear_element(qf_arr, cur, nlevels);
	}
}

template <class key_object>
CascadeFilter<key_object>::Iterator::Iterator(typename
																							CQF<key_object>::Iterator arr[],
																							uint32_t num_levels) :
	qfi_arr(arr), iter_num_levels(num_levels) {
		for (uint32_t i = 0; i < iter_num_levels; i++) {
			if (!qfi_arr[i].done()) {
				key_object k = *qfi_arr[i];
				k.level = i;
				min_heap.push(k);
			}
		}
	}

template <class key_object>
typename CascadeFilter<key_object>::Iterator
CascadeFilter<key_object>::begin(uint32_t num_levels) const {
	typename CQF<key_object>::Iterator *qfi_arr =
		(typename CQF<key_object>::Iterator*)calloc(NUM_MAX_LEVELS,
																								sizeof(typename
																											 CQF<key_object>::Iterator));

	/* Initialize the iterator for all the levels. */
	for (uint32_t i = 0; i < num_levels; i++)
		qfi_arr[i] = filters[i].begin();

	return Iterator(qfi_arr, num_levels);
}

template <class key_object>
typename CascadeFilter<key_object>::Iterator
CascadeFilter<key_object>::end() const {
	typename CQF<key_object>::Iterator qfi_arr = filters[0].end(ages[0], 0);

	return Iterator(&qfi_arr, 1);
}

template <class key_object>
key_object CascadeFilter<key_object>::Iterator::operator*(void) const {
	key_object k = min_heap.top();
	return k;
}

template <class key_object>
void CascadeFilter<key_object>::Iterator::operator++(void) {
	key_object k = min_heap.top();
	min_heap.pop();

	// Incrementing the iterator of the level of the current smallest key and if
	// its not done then insert it in the min heap.
	++qfi_arr[k.level];
	uint32_t level = k.level;
	if (!qfi_arr[level].done()) {
		key_object k = *qfi_arr[level];
		k.level = level;
		min_heap.push(k);
	}
}

template <class key_object>
bool CascadeFilter<key_object>::Iterator::done() const {
	return min_heap.empty();
}

template <class key_object>
bool operator!=(const typename CascadeFilter<key_object>::Iterator& a, const
								typename CascadeFilter<key_object>::Iterator& b) {
	return !a.done() || !b.done();
}

template <class key_object>
void CascadeFilter<key_object>::merge() {
	uint32_t nlevels = find_levels_to_flush();
	assert(nlevels < total_num_levels);

	/* merge nlevels in (nlevels+1)th level. */
	KeyObject cur_key, next_key;
	DEBUG_CF("Merging CQFs 0 to " << nlevels - 1 << " into the CQF "
					 << nlevels);

	DEBUG_CF("Old CQFs");
	for (uint32_t i = 0; i <= nlevels; i++) {
		DEBUG_CF("CQF " << i << " threshold " << thresholds[i]);
		filters[i].dump_metadata();
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
			filters[nlevels].insert(cur_key, LOCK_AND_SPIN);
			/* Update cur_key. */
			cur_key = next_key;
		}
		/* Increment the iterator. */
		++it;
	} while(!it.done());

	/* Insert the last key in the cascade filter. */
	filters[nlevels].insert(cur_key, LOCK_AND_SPIN);

	/* Reset filters that were merged except the last in which the flush
	 * happened. */
	for (uint32_t i = 0; i < nlevels; i++)
		filters[i].reset();

	DEBUG_CF("New CQFs");
	for (uint32_t i = 0; i <= nlevels; i++) {
		DEBUG_CF("CQF " << i);
		filters[i].dump_metadata();
	}
}

template <class key_object>
void CascadeFilter<key_object>::shuffle_merge() {
	/* The empty level is also involved in the shuffle merge. */
	uint32_t nlevels = 0;
	if (max_age) {
		nlevels = find_levels_to_flush();
		// increment age of all the levels that are flushed.
		for (uint32_t i = 1; i < nlevels; i++)
			increment_age(i);
		nlevels += 1;
	} else
		nlevels = find_first_empty_level() + 1;

	assert(nlevels <= total_num_levels);
	DEBUG_CF("Shuffle merging CQFs 0 to " << nlevels - 1);

	KeyObject cur_key, next_key;
	CQF<key_object> *new_filters;
	new_filters = (CQF<key_object>*)calloc(nlevels, sizeof(CQF<key_object>));

	/* Initialize new filters. */
	for (uint32_t i = 0; i < nlevels; i++) {
		std::string file_ext("_cqf.ser");
		std::string file = prefix + std::to_string(num_flush) + "_" +
			std::to_string(i) + file_ext;
		DEBUG_CF("Creating new level " << file);
		new_filters[i] = CQF<key_object>(sizes[i], num_hash_bits, num_value_bits,
																	/*mem*/ false, file.c_str(), seed);
	}

	DEBUG_CF("Old CQFs");
	for (uint32_t i = 0; i < nlevels; i++) {
		DEBUG_CF("CQF " << i << " threshold " << thresholds[i] << " age " <<
						 (unsigned)ages[i]);
		filters[i].dump_metadata();
	}

	/* Initialize cascade filter iterator. */
	CascadeFilter<key_object>::Iterator it = begin(nlevels);
	cur_key = *it;
	++it;

	while(!it.done()) {
		next_key = *it;
		/* If next_key is same as cur_key then aggregate counts.
		 * Also, keep the age (i.e., value) from the lowest level containing the
		 * key.
		 * Else, smear the count across levels starting from the bottom one.
		 * */
		if (cur_key == next_key) {
			cur_key.count += next_key.count;
			cur_key.value = next_key.value;
			cur_key.level = next_key.level;
		} else {
			//PRINT_CF("Inserting " << cur_key.to_string());
			insert_element(new_filters, cur_key, nlevels - 1);
			/* Update cur_key. */
			cur_key = next_key;
		}
		/* Increment the iterator. */
		++it;
	}

	//PRINT_CF("Inserting " << cur_key.to_string());
	/* Insert the last key in the cascade filter. */
	insert_element(new_filters, cur_key, nlevels - 1);

	update_flush_counters(nlevels);

	DEBUG_CF("New CQFs");
	for (uint32_t i = 0; i < nlevels; i++) {
		DEBUG_CF("CQF " << i << " age " << (unsigned)ages[i]);
		new_filters[i].dump_metadata();
	}

	/* Destroy the existing filters and replace them with the new filters. */
	for (uint32_t i = 0; i < nlevels; i++) {
		filters[i].destroy();
		filters[i] = new_filters[i];
		//memcpy(&filters[i], &new_filters[i], sizeof(QF));
	}
}

template <class key_object>
void CascadeFilter<key_object>::find_anomalies(void) {
	KeyObject cur_key, next_key;
	CascadeFilter<key_object>::Iterator it = begin(total_num_levels);
	cur_key = *it;
	++it;

	uint64_t value;
	while(!it.done()) {
		next_key = *it;
		/* If next_key is same as cur_key then aggregate counts.
		 * Also, keep the age (i.e., value) from the lowest level containing the
		 * key.
		 * Else, smear the count across levels starting from the bottom one.
		 * */
		if (cur_key == next_key) {
			cur_key.count += next_key.count;
		} else {
			if (cur_key.count >= THRESHOLD_VALUE) {
				if (anomalies.query_key(cur_key, &value) == 0) {
					// the count is the index at which the key is reported.
					// for count-stretch the count is the current count of the key.
					if (!count_stretch)
						cur_key.count = num_obs_seen;
					// value 0 means that it is reported through shuffle-merge.
					cur_key.value = 0;
					if (anomalies.if_full())
						abort();
					anomalies.insert(cur_key, LOCK_AND_SPIN);
				}
			}
			/* Update cur_key. */
			cur_key = next_key;
		}
		/* Increment the iterator. */
		++it;
	}

	if (cur_key.count >= THRESHOLD_VALUE) {
		if (anomalies.query_key(cur_key, &value) == 0) {
			// the count is the index at which the key is reported.
			// for count-stretch the count is the current count of the key.
			if (!count_stretch)
				cur_key.count = num_obs_seen;
			// value 0 means that it is reported through shuffle-merge.
			cur_key.value = 0;
			if (anomalies.if_full())
				abort();
			anomalies.insert(cur_key, LOCK_AND_SPIN);
		}
	}
}

template <class key_object>
bool CascadeFilter<key_object>::insert(const key_object& k,
																			 uint64_t obs_cnt, enum lock flag) {
	if (flag != NO_LOCK)
		if (!cf_lw_lock.lock(flag))
			return false;

	num_obs_seen = obs_cnt;
	perform_shuffle_merge_if_needed();

	uint64_t value;
	// if the key is already reported then don't insert.
	if (anomalies.query_key(k, &value)) {
		if (flag != NO_LOCK)
			cf_lw_lock.unlock();
		return true;
	}

	key_object dup_k(k);
	// Get the current RAM count/age of the key.
	uint64_t ram_count = filters[0].query_key(dup_k, &value);

	// This code is for the time-stretch case.
	/* use lower-order bits to store the age. */
	if (max_age) {
		dup_k.value *= max_age;	// We shift the value to store age in lower bits.
		if (ram_count) { // if key is already present then use the exisiting age.
			uint8_t cur_age = value & BITMASK(num_age_bits);
			dup_k.value = dup_k.value | cur_age;
		} else // else we use the current age of the level.
			dup_k.value = dup_k.value | ages[0];
	}

	/**
	 * we don't need a lock on the in-memory CQF while inserting. However, I
	 * still have the flag "LOCK_AND_SPIN" here just to be extra sure that we
	 * will not corrupt the data structure.
	 */
	bool ret = filters[0].insert(dup_k, LOCK_AND_SPIN);

	// update the RAM count after the current insertion.
	ram_count += dup_k.count;

	// To check if a key has the THRESHOLD value in RAM.
	// This is not on-demand popcorning.
	if (ram_count >= THRESHOLD_VALUE &&
			anomalies.query_key(dup_k, &value) == 0) {
		// the count is the index at which the key is reported.
		// for count-stretch the count is the current count of the key.
		if (!count_stretch)
			dup_k.count = num_obs_seen;
		else
			dup_k.count = ram_count;
		// value 1 means that it is reported through odp.
		dup_k.value = 0;
		if (anomalies.if_full())
			abort();
		anomalies.insert(dup_k, LOCK_AND_SPIN);
	}

	// This code is for the immediate reporting case.
	// If the count of the key is equal to "THRESHOLD_VALUE -
	// TOTAL_DISK_THRESHOLD" then we will
	// have to perform an on-demand poprorn.
	//
	// We will not remove the key if it has been seen THRESHOLD times.
	// The key will be removed in the next shuffle merge.
	if (odp && ram_count >= popcorn_threshold &&
			anomalies.query_key(dup_k, &value) == 0) {
		uint64_t aggr_count;
		aggr_count = ondisk_count(dup_k) + ram_count;
		if (aggr_count == THRESHOLD_VALUE) {
			// count in the index at which the key is reported.
			// value 1 means that it is reported through odp.
			dup_k.count = num_obs_seen;
			dup_k.value = 1;
			if (anomalies.if_full())
				abort();
			anomalies.insert(dup_k, LOCK_AND_SPIN);
		}
	}

	if (flag != NO_LOCK)
		cf_lw_lock.unlock();

	return ret;
}

template <class key_object>
bool CascadeFilter<key_object>::remove(const key_object& k, enum lock flag) {
	if (flag != NO_LOCK)
		if (!cf_lw_lock.lock(flag))
			return false;

	for (uint32_t i = 0; i < total_num_levels; i++)
		filters[i].remove(k, LOCK_AND_SPIN);

	if (flag != NO_LOCK)
		cf_lw_lock.unlock();

	return true;
}

template <class key_object>
uint64_t CascadeFilter<key_object>::ondisk_count(key_object k) const {
	uint64_t value;
	uint64_t count = 0;
	for (uint32_t i = 1; i < total_num_levels; i++) {
		count += filters[i].query_key(k, &value);
	}
	return count;
}

template <class key_object>
uint64_t CascadeFilter<key_object>::count_key_value(const key_object& k, enum
																										lock flag) {
	if (flag != NO_LOCK)
		if (!cf_lw_lock.lock(flag))
			return false;

	uint64_t value;
	uint64_t count = anomalies.query_key(k, &value);
	if (count == 0) {
		for (uint32_t i = 0; i < total_num_levels; i++) {
			count += filters[i].query_key(k, &value);
		}
	}

	if (flag != NO_LOCK)
		cf_lw_lock.unlock();
	return count;
}

#endif
