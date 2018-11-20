/*
 * ============================================================================
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

#include "zipf.h"
#include "util.h"
#include "lock.h"
#include "gqf_cpp.h"

#define MAX_VALUE(nbits) ((1ULL << (nbits)) - 1)
#define BITMASK(nbits)                                    \
	((nbits) == 64 ? 0xffffffffffffffff : MAX_VALUE(nbits))

template <class key_object>
class CascadeFilter {
	public:
		CascadeFilter(uint32_t id, uint32_t nkeybits, uint32_t nvaluebits, uint32_t
									nagebits, bool odp, bool greedy, bool pinning, uint32_t
									threshold_value, uint32_t filter_thlds[], uint64_t
									filter_sizes[], uint32_t num_levels, std::string& prefix);

		~CascadeFilter();

		const CQF<key_object>* get_filter(uint32_t level) const;

		void print_anomaly_stats(void);
		uint64_t get_num_keys_above_threshold(void) const;
		uint64_t get_num_elements(void) const;
		uint64_t get_num_dist_elements(void) const;
		uint64_t get_num_observations(void) const;
		uint32_t get_num_key_bits(void) const;
		uint32_t get_num_value_bits(void) const;
		uint32_t get_num_age_bits(void) const;
		uint32_t get_seed(void) const;
		uint64_t get_max_size(void) const;

		/* Increment the counter for this key/value pair by count. */
		bool insert(const key_object& key_val_cnt, uint8_t flag);

		/* Remove count instances of this key/value combination. */
		bool remove(const key_object& key_val_cnt, uint8_t flag);

		/* Return the number of times key has been inserted, with the given
			 value, into the cascade qf. */
		uint64_t count_key_value(const key_object& key_val_cnt, uint8_t flag);

		/**
		 * report anomalies currently in the system.
		 */
		void find_anomalies(void);

		bool validate_key_lifetimes(std::unordered_map<uint64_t,
																std::pair<uint64_t, uint64_t>> key_lifetime,
																uint64_t *vals);

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
		bool is_ram_full() const;

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

		bool perform_shuffle_merge_if_needed(uint8_t flag);

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
		 * It uses a fix schedule for merges. There are "r" merges to the first
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

		uint64_t ondisk_count(key_object k);


		CQF<key_object> *filters;
		CQF<key_object> anomalies;
		std::ofstream anomaly_log;
		uint32_t total_num_levels;
		uint32_t num_key_bits;
		uint32_t num_value_bits;
		uint32_t num_age_bits;
		uint32_t threshold_value;
		bool odp;
		bool count_stretch;
		bool greedy;
		bool pinning;
		uint64_t num_odps;
		uint64_t num_point_queries;
		uint64_t total_anomalies;
		uint64_t total_reported_shuffle_merge;
		uint64_t total_reported_odp;
		std::string prefix;
		std::vector<uint32_t> thresholds;
		std::vector<uint64_t> sizes;
		uint32_t *flushes;
		uint32_t *ages;
		uint32_t num_flush;
		uint32_t gfactor;
		uint32_t popcorn_threshold;
		uint32_t max_age;
		uint32_t num_obs_seen;
		uint32_t seed;
		ReaderWriterLock cf_rw_lock;
		uint32_t id;
		// to instrument
		float total_mem_time;
		float total_flush_time;
		struct timeval mem_time_start, mem_time_end;
		struct timeval flush_time_start, flush_time_end;
};

template <class key_object = KeyObject>
bool operator!=(const typename CascadeFilter<key_object>::Iterator& a, const
								typename CascadeFilter<key_object>::Iterator& b);

template <class key_object>
CascadeFilter<key_object>::CascadeFilter(uint32_t id, uint32_t nhashbits,
																				 uint32_t nvaluebits, uint32_t
																				 nagebits, bool odp, bool greedy, bool
																				 pinning, uint32_t threshold_value,
																				 uint32_t filter_thlds[], uint64_t
																				 filter_sizes[], uint32_t num_levels,
																				 std::string& prefix) :
	total_num_levels(num_levels), num_key_bits(nhashbits),
	num_value_bits(nvaluebits + nagebits), num_age_bits(nagebits),
	threshold_value(threshold_value), odp(odp), greedy(greedy),
	pinning(pinning), prefix(prefix), id(id)
{
	total_mem_time = total_flush_time = 0;
	total_anomalies = total_reported_shuffle_merge = total_reported_odp =
		num_odps = num_point_queries = 0;
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
	seed = GQF_SEED;
	thresholds.reserve(num_levels);
	sizes.reserve(num_levels);
	for (uint32_t i = 0; i < num_levels; i++) {
		thresholds.emplace_back(filter_thlds[i]);
		sizes.emplace_back(filter_sizes[i]);
	}
	flushes = (uint32_t*)calloc(num_levels, sizeof(*flushes));
	ages = (uint32_t*)calloc(num_levels, sizeof(*ages));

	// creating an exact CQF to store anomalies.
#ifdef VALIDATE
	uint64_t anomaly_filter_size = sizes[0];
#else
	uint64_t anomaly_filter_size = sizes[0] / 4;
	std::string anomaly_log_ext("anomaly.log");
	std::string file = prefix + anomaly_log_ext;
	anomaly_log.open(file.c_str());
#endif

	DEBUG("Creating anomaly filter of " << anomaly_filter_size <<
				" slots and THRESHOLD " << threshold_value);
	// We use one extra value bit to record whether the key was reported through
	// a shuffle-merge or an odp.
	anomalies = CQF<key_object>(anomaly_filter_size, num_key_bits,
															num_value_bits + 1, QF_HASH_INVERTIBLE, seed);

	filters = (CQF<key_object>*)calloc(num_levels, sizeof(CQF<key_object>));

	if (max_age == 0) {
		uint32_t sum_disk_threshold = 0;
		for (uint32_t i = 1; i < total_num_levels; i++)
			sum_disk_threshold += thresholds[i];
		popcorn_threshold = threshold_value - sum_disk_threshold;
	}

	// We use 1 bit to mark the keys with absolute count.
	if (odp)
		num_value_bits++;

	/* Initialize all the filters. */
	//TODO: (prashant) Maybe make this a lazy initilization.
	for (uint32_t i = 0; i < total_num_levels; i++) {
		DEBUG("Creating level: " << i << " of " << sizes[i] <<
					" slots and threshold " << thresholds[i]);
		std::string file_ext("_cqf.ser");
		std::string file = prefix + std::to_string(i) + file_ext;
		// We use an extra bit for the absolute count optimization.
		// We use the lower-order bit to store the absolute count value.
		filters[i] = CQF<key_object>(sizes[i], num_key_bits, num_value_bits,
																 QF_HASH_INVERTIBLE, seed, file);
	}
}

template <class key_object>
CascadeFilter<key_object>::~CascadeFilter() {
	anomaly_log.close();
	for (uint32_t i = 0; i < total_num_levels; i++)
		filters[i].destroy();
}

	template <class key_object>
const CQF<key_object>* CascadeFilter<key_object>::get_filter(uint32_t level)
	const {
		return &filters[level];
	}

template <class key_object>
uint32_t CascadeFilter<key_object>::get_num_key_bits(void) const {
	return num_key_bits;
}

template <class key_object>
uint32_t CascadeFilter<key_object>::get_num_value_bits(void) const {
	return num_value_bits;
}

template <class key_object>
uint32_t CascadeFilter<key_object>::get_num_age_bits(void) const {
	return num_age_bits;
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
		total_count += get_filter(i)->total_elts();
	return total_count;
}

template <class key_object>
uint64_t CascadeFilter<key_object>::get_num_dist_elements(void) const {
	uint64_t total_count = 0;
	for (uint32_t i = 0; i < total_num_levels; i++)
		total_count += get_filter(i)->dist_elts();
	return total_count;
}

template <class key_object>
uint64_t CascadeFilter<key_object>::get_num_observations(void) const {
	return num_obs_seen;
}

template <class key_object>
void CascadeFilter<key_object>::print_anomaly_stats(void) {
#if 0
	// find anomalies that are not reported yet.
	//find_anomalies();
	uint64_t num_shuffle_merge = 0, num_odp = 0;
	typename CQF<key_object>::Iterator it = anomalies.begin();
	while (!it.done()) {
		key_object k = *it;
		if (k.value == 0) {
			num_shuffle_merge++;
			if (count_stretch)
				DEBUG("Shuffle-merge: " << k.key << " count: " << k.count);
			else
				DEBUG("Shuffle-merge: " << k.key << " index: " << k.count);
		} else if (k.value == 1) {
			num_odp++;
			DEBUG("ODP: " << k.key << " index: " << k.count);
		} else {
			ERROR("Wrong value in the anomaly CQF.");
			abort();
		}
		++it;
	}
#endif
	
	if (odp) {
		PRINT("Num odps: " << num_odps);
		PRINT("Num point queries: " << num_point_queries);
	}
	PRINT("Number of keys above the THRESHOLD value " <<
				total_anomalies);
	PRINT("Number of keys reported through shuffle-merges " <<
				total_reported_shuffle_merge);
	PRINT("Number of keys reported through odp " << total_reported_odp);
	PRINT("Total memory time (secs): " << total_mem_time);
	PRINT("Total flush time (secs): " << total_flush_time);
}

template <class key_object>
uint64_t CascadeFilter<key_object>::get_num_keys_above_threshold(void) const {
	return total_anomalies;
}

template <class key_object>
bool CascadeFilter<key_object>::validate_key_lifetimes(
								std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>>
								key_lifetime, uint64_t *vals) {
	uint32_t failures = 0;
	std::ofstream result;

	// find anomalies that are not reported yet.
	if (!odp)
		find_anomalies();

	DEBUG("Anomaly CQF ");
	anomalies.dump_metadata();

	//PRINT("Number of keys above threshold: " << get_num_keys_above_threshold());
	if (max_age) {
		result.open("raw/time-stretch.data");
		//result << "x_0 y_0 y_1 y_2" << std::endl;
		double stretch = 1 + 1 / (float)num_age_bits;
		result << "Maximum allowed stretch: " << stretch << std::endl;
		result << "Key Index-0 Index-T Lifetime ReportIndex Stretch" << std::endl;
		for (auto it : key_lifetime) {
			if (it.second.first < it.second.second) {
				uint64_t value;
				key_object k(it.first, 0, 0, 0);
				uint64_t lifetime = it.second.second - it.second.first;
				uint64_t reporttime = anomalies.query_key(k, &value, 0) -
					it.second.first;
				if (reporttime > lifetime * stretch) {
					PRINT("Time-stretch reporting failed Key: " << it.first <<
								" Index-1: " << it.second.first << " Index-T " <<
								it.second.second << " Reporting index " <<
								anomalies.query_key(k, &value, 0) << " for stretch " <<
								stretch);
					failures++;
				}
				//result << idx++ << " " << lifetime << " " << reporttime << " " <<
				//lifetime * stretch << std::endl;
				result << it.first << " " << it.second.first << " " << it.second.second
					<< " " << (it.second.second - it.second.first) << " " <<
					anomalies.query_key(k, &value, 0) << " " <<
					reporttime/(double)lifetime << std::endl;
			}
		}
	} else if (odp) {
		result.open("raw/immediate-reporting.data");
		result << "key odp Index-0 Index-T Lifetime Report_Index" << std::endl;
		for (auto it : key_lifetime) {
			if (it.second.first < it.second.second) {
				uint64_t value = 0;
				key_object k(it.first, 0, 0, 0);
				uint64_t lifetime = it.second.second - it.second.first;
				uint64_t reportindex = anomalies.query_key(k, &value, 0);
				if (reportindex != it.second.second) {
					PRINT("Immediate reporting failed Key: " << it.first <<
								" Index-T " << it.second.second << " Reporting index "
								<< reportindex);
					failures++;
				}
				result << it.first << " " <<  value << " " << it.second.first << " "
					<< it.second.second << " " << lifetime << " "
					<< reportindex << std::endl;
			}
		}
	} else if (count_stretch) {
		result.open("raw/count-stretch.data");
		//result << "x_0 y_0 y_1 y_2" << std::endl;
		result << "Key Index-0 Index-T Lifetime ReportCount Stretch TimeStretch" << std::endl;
		// for count stretch the count stored with keys in anomalies is the actual
		// count at the time of reporting.
		for (auto it : key_lifetime) {
			if (it.second.first < it.second.second) {
				uint64_t value;
				key_object k(it.first, 0, 0, 0);
				uint64_t reportindex = anomalies.query_key(k, &value, 0);
				uint64_t reportcount = popcornfilter::actual_count_at_index(vals,
																																		it.first,
																																		reportindex);
				if (reportcount > threshold_value * 2) {
					PRINT("Count stretch reporting failed Key: " << it.first <<
								" Reporting count " << reportcount);
					failures++;
				}
				uint64_t lifetime = it.second.second - it.second.first;
				uint64_t reporttime = anomalies.query_key(k, &value, 0) -
					it.second.first;
				//result << idx++ << " " << threshold_value << " " << reportcount << " "
				//<< threshold_value * 2 << std::endl;
				result << it.first << " " << it.second.first << " " << it.second.second
					<< " " << (it.second.second - it.second.first) << " " <<
					reportcount << " " <<
					reportcount/(double)threshold_value << " " <<
					reporttime/(double)lifetime << std::endl;
			}
		}
	} else {
		abort();
	}

	if (!failures)
		return true;
	else {
		PRINT("Failed to report " << failures << " keys on time.");
		return false;
	}
	//result.close();
}

template <class key_object>
bool CascadeFilter<key_object>::is_ram_full() const {
	if (num_obs_seen == 0)
		return false;

	if (max_age) {
		// we do a flush when the number of observations seen is increased
		// by ram_size/max_age.
		uint64_t num_obs = get_filter(0)->total_slots()/max_age;
		if (num_obs_seen % num_obs == 0)
			return true;
	} else {
		if (greedy) {
			if (get_filter(0)->is_full()) {
				double load_factor = get_filter(0)->occupied_slots() /
					(double)get_filter(0)->total_slots();
				DEBUG("Load factor: " << load_factor);
				return true;
			}
		} else {
			uint64_t num_obs = get_filter(0)->total_slots() * 0.75;
			if (num_obs_seen % num_obs == 0) {
				return true;
			}
		}
	}
	return false;
}

template <class key_object>
bool CascadeFilter<key_object>::need_shuffle_merge_time_stretch(void) const {
	uint64_t num_obs = get_filter(0)->total_slots()/max_age;
	int num_obs_frac = num_obs_seen/num_obs;
	if (num_obs_frac > 0)
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
		/* (prashant): This is an upper-bound on the number of slots that are
		 * needed in the empty level for the shuffle-merge to finish successfully.
		 * We can probably give a little slack in the constraints here.
		 */
		uint64_t available_slots = 	get_filter(empty_level)->total_slots() -
			get_filter(empty_level)->occupied_slots();
		if (!get_filter(empty_level)->is_full() && total_occupied_slots <=
				available_slots)
			break;
		total_occupied_slots += get_filter(empty_level)->occupied_slots();
	}
	/* If found an empty level. Merge all levels before the empty level into the
	 * empty level. Else create a new level and merge all the levels. */
	uint32_t nlevels = 0;
	if (empty_level < total_num_levels)
		nlevels = empty_level;

	// TODO: Create a new empty level if the data structure is full.

	return nlevels;
}

template <class key_object>
uint32_t CascadeFilter<key_object>::find_levels_to_flush(void) const {
	uint32_t empty_level;
	for (empty_level = 1; empty_level < total_num_levels - 1; empty_level++) {
		if (flushes[empty_level] < gfactor - 1)
			break;
	}

	// TODO: Create a new empty level if the data structure is full.
	return empty_level;
}

template <class key_object>
bool CascadeFilter<key_object>::perform_shuffle_merge_if_needed(uint8_t flag) {
	if (is_ram_full()) {
		if (max_age) {
			// Age is increased for level 0 whenever it is 1/alpha full.
			ages[0] = (ages[0] + 1) % max_age;
			if (need_shuffle_merge_time_stretch()) {
				DEBUG("Number of observations seen: " << num_obs_seen);
				gettimeofday(&flush_time_start, NULL);
				// release the reader lock.
				if (GET_PF_NO_LOCK(flag) != PF_NO_LOCK)
					cf_rw_lock.read_unlock();
				// acquire the write lock.
				if (GET_PF_NO_LOCK(flag) != PF_NO_LOCK)
					if (!cf_rw_lock.write_lock(flag))
						return false;
				//PRINT("CascadeFilter " << id << " Flushing " << num_flush << 
							//" Num obs: " << num_obs_seen);
				shuffle_merge();
				gettimeofday(&flush_time_end, NULL);
				total_flush_time += popcornfilter::cal_time_elapsed(&flush_time_start,
																														&flush_time_end);
				// Increment the flushing count.
				num_flush++;
				// release the write lock.
				if (GET_PF_NO_LOCK(flag) != PF_NO_LOCK)
					cf_rw_lock.write_unlock();
				// acquire the reader lock.
				if (GET_PF_NO_LOCK(flag) != PF_NO_LOCK)
					if (!cf_rw_lock.read_lock(flag))
						return false;
			}
		} else {
			gettimeofday(&flush_time_start, NULL);
			// release the reader lock.
			if (GET_PF_NO_LOCK(flag) != PF_NO_LOCK)
				cf_rw_lock.read_unlock();
			// acquire the write lock.
			if (GET_PF_NO_LOCK(flag) != PF_NO_LOCK)
				if (!cf_rw_lock.write_lock(flag))
					return false;
			//PRINT("CascadeFilter " << id << " Flushing " << num_flush << 
						//" Num obs: " << num_obs_seen);
			shuffle_merge();
			gettimeofday(&flush_time_end, NULL);
			total_flush_time += popcornfilter::cal_time_elapsed(&flush_time_start,
																													&flush_time_end);
			// Increment the flushing count.
			num_flush++;
			// release the write lock.
			if (GET_PF_NO_LOCK(flag) != PF_NO_LOCK)
				cf_rw_lock.write_unlock();
			// acquire the reader lock.
			if (GET_PF_NO_LOCK(flag) != PF_NO_LOCK)
				if (!cf_rw_lock.read_lock(flag))
					return false;
		}
	}

	return true;
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
	// We use the absolute count optimization in two cases:
	// 1. If the absolute count optimization is on and:
	//     1. If all the levels are involved in the shuffle-merge
	//     2. If the last level involved in the shuffle-merge has the absolute
	//     count of the key.
	if (pinning && (nlevels == total_num_levels - 1 || (k.value & 1) == 1)) {
		uint64_t count = k.count;
		int32_t i = 0;
		uint32_t final_level = 0;
		// Find the highest level where the key is supposed to be inserted.
		for (i = nlevels; i > 0; i--) {
			if (count >= thresholds[i]) {
				count -= thresholds[i];
				if (count == 0) {
					final_level = i;
					break;
				}
			}
			else
				break;
		}
		if (count > 0)
			final_level = i;
		k.value = k.value | 1;	// set the absolute count bit.
		k.level = final_level;
		int cqf_ret = qf_arr[final_level].insert(k, PF_NO_LOCK | QF_KEY_IS_HASH);
		if (cqf_ret == QF_NO_SPACE) {
			ERROR("CascadeFilter " << id << " Level " << final_level <<
						" is too full. Please increase the size.");
			exit(1);
		}
	} else {
		for (int32_t i = nlevels; i > 0; i--) {
			if (k.count >= thresholds[i]) {
				key_object cur(k.key, k.value, thresholds[i], i);
				int cqf_ret = qf_arr[i].insert(cur, PF_NO_LOCK | QF_KEY_IS_HASH);
				if (cqf_ret == QF_NO_SPACE) {
					ERROR("CascadeFilter " << id << " Level " << i <<
								" is too full. Please increase the size.");
					exit(1);
				}
				k.count -= thresholds[i];
			} else {
				if (max_age) {
					uint64_t value;
					uint64_t cur_count = filters[i].query_key(k, &value, QF_KEY_IS_HASH);
					// reset the value bits of the key.
					k.value = k.value & ~BITMASK(num_age_bits);
					if (cur_count) { // if key is already present then use the existing age.
						uint8_t cur_age = value & BITMASK(num_age_bits);
						k.value = k.value | cur_age;
					} else {	// assign the age based on the current num_flush of the level
						k.value = k.value | ages[i];
					}
				}
				k.level = i;
				int cqf_ret = qf_arr[i].insert(k, PF_NO_LOCK | QF_KEY_IS_HASH);
				if (cqf_ret == QF_NO_SPACE) {
					ERROR("CascadeFilter " << id << " Level " << i <<
								" is too full. Please increase the size.");
					exit(1);
				}
				k.count = 0;
				break;
			}
		}
		/* If some observations are left then insert them in the first level. */
		if (k.count > 0) {
			k.level = 0;
			int cqf_ret = qf_arr[0].insert(k, PF_NO_LOCK | QF_KEY_IS_HASH);
			if (cqf_ret == QF_NO_SPACE) {
				ERROR("CascadeFilter " << id <<
							" Level 0 is too full. Please increase the size.");
				exit(1);
			}
		}
	}
}

template <class key_object>
void CascadeFilter<key_object>::insert_element(CQF<key_object> *qf_arr,
																							 key_object cur, uint32_t
																							 nlevels) {
	uint64_t value;
	if (cur.count >= threshold_value) {
		if (anomalies.query_key(cur, &value, QF_KEY_IS_HASH) == 0) {
			// the count is the index at which the key is reported.
			cur.count = num_obs_seen;
			// value 0 means that it is reported through shuffle-merge.
			cur.value = 0;
#ifdef VALIDATE
			if (anomalies.is_full())
				abort();
#else
			if (anomalies.is_full())
				anomalies.reset();
			anomaly_log << "Reporting shuffle-merge: " << cur.to_string() <<
				std::endl;
#endif
			anomalies.insert(cur, PF_WAIT_FOR_LOCK | QF_KEY_IS_HASH);
			DEBUG("Reporting shuffle-merge: " << cur.to_string());
			total_anomalies++;
			total_reported_shuffle_merge++;
		}
	} else {
		if (max_age) {
			// if key is aged then flush it to the next level.
			if (cur.level < nlevels && is_aged(cur)) { // flush the key down.
				assert(cur.level < nlevels);
				smear_element(qf_arr, cur, cur.level + 1);
				//DEBUG("Aged " << cur.to_string());
			}
			// not aged yet. reinsert the key with aggregated count in the lowest
			// level (involved in the shuffle-merge) it was present in.
			else {
				int cqf_ret = qf_arr[cur.level].insert(cur, PF_NO_LOCK |
																							 QF_KEY_IS_HASH);
				if (cqf_ret == QF_NO_SPACE) {
					ERROR("CascadeFilter " << id << " Level " << cur.level <<
								" is too full. Please increase the size.");
					exit(1);
				}
				//DEBUG("Live " << cur.to_string());
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
				key_object k = qfi_arr[i].get_cur_hash();
				k.level = i;
				min_heap.push(k);
			}
		}
	}

template <class key_object>
typename CascadeFilter<key_object>::Iterator
CascadeFilter<key_object>::begin(uint32_t num_levels) const {
	typename CQF<key_object>::Iterator *qfi_arr =
		(typename CQF<key_object>::Iterator*)calloc(num_levels,
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
		key_object k = qfi_arr[level].get_cur_hash();
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
	DEBUG("Merging CQFs 0 to " << nlevels - 1 << " into the CQF "
				<< nlevels);

	DEBUG("Old CQFs");
	for (uint32_t i = 0; i <= nlevels; i++) {
		DEBUG("CQF " << i << " threshold " << thresholds[i]);
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
			int cqf_ret = filters[nlevels].insert(cur_key, PF_WAIT_FOR_LOCK |
																						QF_KEY_IS_HASH);
			if (cqf_ret == QF_NO_SPACE) {
				ERROR("CascadeFilter " << id << " Level " << nlevels <<
							" is too full. Please increase the size.");
				exit(1);
			}
			/* Update cur_key. */
			cur_key = next_key;
		}
		/* Increment the iterator. */
		++it;
	} while(!it.done());

	/* Insert the last key in the cascade filter. */
	int cqf_ret = filters[nlevels].insert(cur_key, PF_WAIT_FOR_LOCK |
																				QF_KEY_IS_HASH);
	if (cqf_ret == QF_NO_SPACE) {
		ERROR("CascadeFilter " << id << " Level " << nlevels <<
					" is too full. Please increase the size.");
		exit(1);
	}

	/* Reset filters that were merged except the last in which the flush
	 * happened. */
	for (uint32_t i = 0; i < nlevels; i++)
		filters[i].reset();

	DEBUG("New CQFs");
	for (uint32_t i = 0; i <= nlevels; i++) {
		DEBUG("CQF " << i);
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
	} else {
		if (greedy)
			nlevels = find_first_empty_level() + 1;
		else
			nlevels = find_levels_to_flush() + 1;
	}

	assert(nlevels <= total_num_levels);
	DEBUG("Shuffle merging CQFs 0 to " << nlevels - 1);

	KeyObject cur_key, next_key;
	CQF<key_object> *new_filters;
	new_filters = (CQF<key_object>*)calloc(nlevels, sizeof(CQF<key_object>));

	/* Initialize new filters. */
	for (uint32_t i = 0; i < nlevels; i++) {
		std::string file_ext("_cqf.ser");
		std::string file = prefix + std::to_string(num_flush) + "_" +
			std::to_string(i) + file_ext;
		DEBUG("Creating new level " << file);
		new_filters[i] = CQF<key_object>(sizes[i], num_key_bits, num_value_bits,
																		 QF_HASH_INVERTIBLE, seed, file);
	}

	DEBUG("Old CQFs");
	for (uint32_t i = 0; i < nlevels; i++) {
		DEBUG("CQF " << i << " threshold " << thresholds[i] << " age " <<
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
			// check if the absolute bit is 0.
			// Aggregate count if the absolute count bit is not set.
			if (!odp || (odp && (cur_key.value & 1) == 0)) {
				cur_key.count += next_key.count;
				cur_key.value = next_key.value;
				cur_key.level = next_key.level;
			}
		} else {
			//PRINT("Inserting " << cur_key.to_string());
			// When only odp is enabled the absolute count is only set in RAM
			if (odp && (cur_key.value & 1) == 1)
				insert_element(new_filters, cur_key, 0);
			else
				insert_element(new_filters, cur_key, nlevels - 1);
			/* Update cur_key. */
			cur_key = next_key;
		}
		/* Increment the iterator. */
		++it;
	}

	//PRINT("Inserting " << cur_key.to_string());
	/* Insert the last key in the cascade filter. */
	insert_element(new_filters, cur_key, nlevels - 1);

	update_flush_counters(nlevels);

	DEBUG("New CQFs");
	for (uint32_t i = 0; i < nlevels; i++) {
		DEBUG("CQF " << i << " age " << (unsigned)ages[i]);
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

	DEBUG("Finding anomalies final time.");
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
			if (cur_key.count >= threshold_value) {
				if (anomalies.query_key(cur_key, &value, QF_KEY_IS_HASH) == 0) {
					// the count is the index at which the key is reported.
					cur_key.count = num_obs_seen;
					// value 0 means that it is reported through shuffle-merge.
					cur_key.value = 0;
#ifdef VALIDATE
					if (anomalies.is_full())
						abort();
#else
					if (anomalies.is_full())
						anomalies.reset();
					anomaly_log << "Reporting shuffle-merge: " << cur_key.to_string() <<
						std::endl;
#endif
					anomalies.insert(cur_key, PF_WAIT_FOR_LOCK | QF_KEY_IS_HASH);
					DEBUG("Reporting shuffle-merge: " << cur_key.to_string());
					total_anomalies++;
					total_reported_shuffle_merge++;
				}
			}
			/* Update cur_key. */
			cur_key = next_key;
		}
		/* Increment the iterator. */
		++it;
	}

	if (cur_key.count >= threshold_value) {
		if (anomalies.query_key(cur_key, &value, QF_KEY_IS_HASH) == 0) {
			// the count is the index at which the key is reported.
			cur_key.count = num_obs_seen;
			// value 0 means that it is reported through shuffle-merge.
			cur_key.value = 0;
#ifdef VALIDATE
			if (anomalies.is_full())
				abort();
#else
			if (anomalies.is_full())
				anomalies.reset();
			anomaly_log << "Reporting shuffle-merge: " << cur_key.to_string() <<
				std::endl;
#endif
			anomalies.insert(cur_key, PF_WAIT_FOR_LOCK | QF_KEY_IS_HASH);
			DEBUG("Reporting shuffle-merge: " << cur_key.to_string());
			total_anomalies++;
			total_reported_shuffle_merge++;
		}
	}
}

template <class key_object>
bool CascadeFilter<key_object>::insert(const key_object& k, uint8_t flag) {
	if (GET_PF_NO_LOCK(flag) != PF_NO_LOCK)
		if (!cf_rw_lock.read_lock(flag))
			return false;

	if (!perform_shuffle_merge_if_needed(flag))
		return false;

	gettimeofday(&mem_time_start, NULL);
	uint64_t value = 0;
	// if the key is already reported then don't insert.
	if (anomalies.query_key(k, &value, 0)) {
		if (GET_PF_NO_LOCK(flag) != PF_NO_LOCK)
			cf_rw_lock.read_unlock();
		num_obs_seen += k.count;
		return true;
	}

	key_object dup_k(k);
	// Get the current RAM count/age of the key.
	value = 0;		// reset value to reuse it.
	uint64_t ram_count = filters[0].query_key(dup_k, &value, 0);
	bool absolute_count{false};
	// Check if the absolute count is enabled
	// If it's set then the RAM count is the absolute count of the key.
	if (odp && ram_count > 0 && (value & 1)  == 1) {
		absolute_count = true;
		/* use lower-order bits to store the absolute count bit. */
		dup_k.value = value;
	}

	// This code is for the time-stretch case.
	/* use lower-order bits to store the age. */
	if (max_age) {
		dup_k.value *= max_age;	// We shift the value to store age in lower bits.
		if (ram_count) { // if key is already present then use the existing age.
			uint8_t cur_age = value & BITMASK(num_age_bits);
			dup_k.value = dup_k.value | cur_age;
		} else // else we use the current age of the level.
			dup_k.value = dup_k.value | ages[0];
	}

	/**
	 * we don't need a lock on the in-memory CQF while inserting. However, I
	 * still have the flag "PF_WAIT_FOR_LOCK" here just to be extra sure that we
	 * will not corrupt the data structure.
	 */
	int cqf_ret = filters[0].insert(dup_k, PF_WAIT_FOR_LOCK);
	bool ret;
	if (cqf_ret < 0)
		ret = false;
	else
		ret = true;

	// update the RAM count after the current insertion.
	ram_count += dup_k.count;

	// To check if a key has the THRESHOLD value in RAM.
	// This is not on-demand popcorning.
	if (ram_count >= threshold_value &&
			anomalies.query_key(dup_k, &value, 0) == 0) {
		// the count is the index at which the key is reported.
		dup_k.count = num_obs_seen;
		// value 1 means that it is reported through odp.
		if (!absolute_count)
			dup_k.value = 0;
		else
			dup_k.value = 1;
#ifdef VALIDATE
		if (anomalies.is_full())
			abort();
#else
		if (anomalies.is_full())
			anomalies.reset();
		anomaly_log << "Reporting shuffle-merge: " << dup_k.to_string() <<
			std::endl;
#endif
		anomalies.insert(dup_k, PF_WAIT_FOR_LOCK);
		DEBUG("Reporting shuffle-merge: " << dup_k.to_string());
		total_anomalies++;
		if (!absolute_count)
			total_reported_shuffle_merge++;
		else
			total_reported_odp++;
	}
	gettimeofday(&mem_time_end, NULL);
	total_mem_time += popcornfilter::cal_time_elapsed(&mem_time_start,
																										&mem_time_end);

	// This code is for the immediate reporting case.
	// If the count of the key is equal to "threshold_value -
	// TOTAL_DISK_THRESHOLD" and that's not the final count (i.e., key is not
	// pinned in RAM) then we will have to perform an on-demand poprorn.
	//
	// We will not remove the key if it has been seen THRESHOLD times.
	// The key will be removed in the next shuffle merge.
	if (odp && ram_count >= popcorn_threshold && !absolute_count &&
			anomalies.query_key(dup_k, &value, 0) == 0) {
		uint64_t aggr_count = 0, ondisk_cnt = 0;
		ondisk_cnt = ondisk_count(dup_k);
		num_odps++;
		if (ondisk_cnt > 0) {
			aggr_count = ondisk_cnt + ram_count;
			if (aggr_count >= threshold_value) {
				// count is the index at which the key is reported.
				// value 1 means that it is reported through odp.
				dup_k.count = num_obs_seen;
				dup_k.value = 1;
#ifdef VALIDATE
				if (anomalies.is_full())
					abort();
#else
				if (anomalies.is_full())
					anomalies.reset();
				anomaly_log << "Reporting odp: " << dup_k.to_string() <<
					std::endl;
#endif
				anomalies.insert(dup_k, PF_WAIT_FOR_LOCK);
				DEBUG("Reporting odp: " << dup_k.to_string());
				total_anomalies++;
				total_reported_odp++;
			} else {
				DEBUG("Adding on-disk count to a level in RAM.");
				// add the on-disk count to RAM.
				// delete the existing key
				cqf_ret = filters[0].delete_key(dup_k, PF_WAIT_FOR_LOCK);
				if (cqf_ret < 0) {
					ERROR("CascadeFilter " << id << " Cannot delete key: " << dup_k.key);
					exit(1);
				}
				dup_k.count = aggr_count;
				dup_k.value = dup_k.value | 1;		// set the absolute count bit
				cqf_ret = filters[0].insert(dup_k, PF_WAIT_FOR_LOCK);
				if (cqf_ret < 0)
					ret = false;
				else
					ret = true;
			}
		}
	}
	num_obs_seen += k.count;

	if (GET_PF_NO_LOCK(flag) != PF_NO_LOCK)
		cf_rw_lock.read_unlock();

	return ret;
}

template <class key_object>
bool CascadeFilter<key_object>::remove(const key_object& k, uint8_t flag) {
	cf_rw_lock.write_lock(PF_WAIT_FOR_LOCK);

	for (uint32_t i = 0; i < total_num_levels; i++)
		filters[i].remove(k, PF_WAIT_FOR_LOCK);

	cf_rw_lock.write_unlock();

	return true;
}

// This method is not thread-safe. The caller is required to acquire
// appropriate locks.
template <class key_object>
uint64_t CascadeFilter<key_object>::ondisk_count(key_object k) {
	uint64_t count = 0;
	for (uint32_t i = 1; i < total_num_levels; i++) {
		uint64_t value = 0;
		count += filters[i].query_key(k, &value, 0);
		num_point_queries++;
		// if pinning is enabled and the key is pinned.
		if (pinning && count > 0 && (value & 1)  == 1)
			break;
	}
	return count;
}

template <class key_object>
uint64_t CascadeFilter<key_object>::count_key_value(const key_object& k,
																										uint8_t flag) {
	if (GET_PF_NO_LOCK(flag) != PF_NO_LOCK)
		if (!cf_rw_lock.read_lock(flag))
			return false;

	uint64_t value;
	uint64_t count = anomalies.query_key(k, &value, 0);
	if (count == 0) {
		for (uint32_t i = 0; i < total_num_levels; i++) {
			count += filters[i].query_key(k, &value, 0);
		}
	}

	if (GET_PF_NO_LOCK(flag) != PF_NO_LOCK)
		cf_rw_lock.read_unlock();
	return count;
}

#endif
