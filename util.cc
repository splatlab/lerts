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
#include <bitset>
#include <cassert>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>

#include <stdexcept>
#include <signal.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

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

std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>>
analyze_stream(uint64_t *vals, uint64_t nvals, uint32_t threshold) {
	std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>> key_lifetime;
	std::multiset<uint64_t> key_counts;
	std::vector<uint64_t> freq_dist;
	freq_dist.resize(30);

	PRINT_CF("Analyzing Stream");

#if 0
	for (uint32_t i = 0; i < nvals; i++) {
		uint64_t key = vals[i];
		key_counts.insert(key);
	}

	for (auto key : key_counts) {
		int count = key_counts.count(key);
		if (count <= 25)
			freq_dist[count]++;
	}

	uint32_t i = 0;
	for (auto count : freq_dist) {
		std::cout << i++ << " " << count << std::endl;
	}
#endif

	for (uint32_t i = 0; i < nvals; i++) {
		uint64_t key = vals[i];
		if (key_counts.count(key) < threshold) {
			key_counts.insert(key);
			if (key_counts.count(key) == 1)
				key_lifetime[key] = std::pair<uint64_t, uint64_t>(i, i);

			if (key_counts.count(key) == threshold)
				key_lifetime[key].second = i;
		}
	}

	uint64_t total_anomalies = 0;
	for (auto it : key_lifetime) {
		assert (it.second.first <= it.second.second);
		if (it.second.first < it.second.second) {
			//PRINT_CF(it.first << " " << it.second.first << " " << it.second.second);
			total_anomalies++;
		}
	}

	PRINT_CF("Number of keys above threshold: " << total_anomalies);

	return key_lifetime;
}

uint64_t *read_stream_from_disk(std::string file) {
	struct stat sb;
	int ret;

	int fd = open(file.c_str(), O_RDWR, S_IRWXU);
	if (fd < 0) {
		perror("Couldn't open file:\n");
		exit(EXIT_FAILURE);
	}

	ret = fstat (fd, &sb);
	if ( ret < 0) {
		perror ("fstat");
		exit(EXIT_FAILURE);
	}

	if (!S_ISREG (sb.st_mode)) {
		fprintf (stderr, "%s is not a file\n", file.c_str());
		exit(EXIT_FAILURE);
	}

	uint64_t *vals = (uint64_t *)mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE,
																		MAP_SHARED, fd, 0);
	PRINT_CF("Read " << sb.st_size / 8 << " keys from disk");

	return vals;
}

std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>>
read_stream_log_from_disk(std::string file) {
	uint64_t key, index0, index24;
	std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>> key_lifetime;

	std::ifstream statsfile(file.c_str());
	while (statsfile >> key >> index0 >> index24) {
		assert (index0 <= index24);
		if (index0 < index24) {
			std::pair<uint64_t, uint64_t> val(index0, index24);
			key_lifetime[key] = val;
		}
	}
	statsfile.close();

	PRINT_CF("Number of keys above threshold: " << key_lifetime.size());

	return key_lifetime;
}

