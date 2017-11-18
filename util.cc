/*
 * ============================================================================
 *
 *       Filename:  util.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017-09-21 01:01:53 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Prashant Pandey (), ppandey@cs.stonybrook.edu
 *   Organization:  Stony Brook University
 *
 * ============================================================================
 */

#include <iostream>
#include <algorithm>
#include <cstring>
#include <vector>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <bitset>
#include <cassert>

#include "util.h"

void print_time_elapsed(std::string desc, struct timeval* start, struct
												timeval* end)
{
	struct timeval elapsed;
	if (start->tv_usec > end->tv_usec) {
		end->tv_usec += 1000000;
		end->tv_sec--;
	}
	elapsed.tv_usec = end->tv_usec - start->tv_usec;
	elapsed.tv_sec = end->tv_sec - start->tv_sec;
	float time_elapsed = (elapsed.tv_sec * 1000000 + elapsed.tv_usec)/1000000.f;
	std::cout << desc << "Total Time Elapsed: " << std::to_string(time_elapsed) << 
		"seconds" << std::endl;
}

void analyze_stream(uint64_t *vals, uint64_t nvals) {
	std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>> key_lifetime;
	std::multiset<uint64_t> key_counts;

	PRINT_CF("Analyzing the steam");

	uint64_t total_inserted_keys = 0;
	for (uint32_t i = 0; i < nvals; i++) {
		uint64_t key = vals[i];
		if (key_counts.count(key) == 0) {
			key_counts.insert(key);
			key_lifetime[key] = std::pair<uint64_t, uint64_t>(i, i);
			total_inserted_keys++;
		} else if (key_counts.count(key) < 23) {
			key_counts.insert(key);
			total_inserted_keys++;
		} else if (key_counts.count(key) == 23) {
			key_counts.insert(key);
			key_lifetime[key].second = i;
			total_inserted_keys++;
		}
	}
	for (auto it : key_lifetime) {
		assert (it.second.first <= it.second.second);
		if (it.second.first < it.second.second)
			PRINT_CF(it.first << " " << it.second.first << " " << it.second.second);
	}
	PRINT_CF("Total inserted keys: " << total_inserted_keys);
}
