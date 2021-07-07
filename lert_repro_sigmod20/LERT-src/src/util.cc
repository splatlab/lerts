/*
 * ============================================================================
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

namespace popcornfilter {
	float cal_time_elapsed(struct timeval* start, struct timeval* end)
	{
		struct timeval elapsed;
		if (start->tv_usec > end->tv_usec) {
			end->tv_usec += 1000000;
			end->tv_sec--;
		}
		elapsed.tv_usec = end->tv_usec - start->tv_usec;
		elapsed.tv_sec = end->tv_sec - start->tv_sec;
		return (elapsed.tv_sec * 1000000 + elapsed.tv_usec)/1000000.f;
	}
	
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

	float RandomBetween(float smallNumber, float bigNumber)
	{
    float diff = bigNumber - smallNumber;
    return (((float) rand() / RAND_MAX) * diff) + smallNumber;
	}

	void induce_special_case(uint64_t *vals, uint32_t threshold, uint64_t
													 start_idx, uint64_t lifetime, uint32_t num) {
		for (uint32_t j = 0; j < num; j++) {
			j *= 1000;
			uint64_t key = rand();
			vals[start_idx + j] = key;
			for (uint32_t i = 0; i < threshold - 2; i++) {
				uint64_t rand_idx = rand() % (lifetime  + 1) + start_idx + j;
				vals[rand_idx] = key;
			}
			vals[start_idx + lifetime + j] = key;
		}
	}

	std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>>
		analyze_stream(uint64_t *vals, uint64_t nvals, uint32_t threshold) {
			std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>> key_lifetime;
			std::multiset<uint64_t> key_counts;

			PRINT("Analyzing Stream");

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
			PRINT("Clearing the multimap.");
			key_counts.clear();

			uint64_t total_anomalies = 0;
			for (auto it : key_lifetime) {
				assert (it.second.first <= it.second.second);
				if (it.second.first < it.second.second) {
					//PRINT(it.first << " " << it.second.first << " " << it.second.second);
					total_anomalies++;
				}
			}

			PRINT("Number of keys: " << key_lifetime.size());
			PRINT("Number of keys above threshold: " << total_anomalies);

			return key_lifetime;
		}

	uint64_t actual_count_at_index(uint64_t *vals, uint64_t key, uint64_t index)
	{
		uint64_t count = 0;
		for (uint64_t i = 0; i <= index; i++) {
			if (vals[i] == key)
				count++;
		}

		return count;
	}

	uint64_t *read_stream_from_disk(std::string file, uint64_t *num_keys) {
		int fd = open(file.c_str(), O_RDWR);
		if (fd < 0) {
			perror("Couldn't open file:");
			exit(1);
		}
		struct stat sb;
		int ret = fstat (fd, &sb);
		if ( ret < 0) {
			perror ("fstat");
			exit(EXIT_FAILURE);
		}
		if (!S_ISREG (sb.st_mode)) {
			fprintf (stderr, "%s is not a file.\n", file.c_str());
			exit(EXIT_FAILURE);
		}

		*num_keys = sb.st_size/sizeof(uint64_t);
		uint64_t *arr = (uint64_t *)mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED,
																		 fd, 0);
		if (arr == MAP_FAILED) {
			perror("Couldn't mmap metadata.");
			exit(EXIT_FAILURE);
		}

		ret = madvise(arr, sb.st_size, MADV_SEQUENTIAL);
		if (ret == -1) {
			perror("madvise failed");
			exit(EXIT_FAILURE);
		}

		return arr;
	}
	
	uint64_t *read_file_disk(std::string file) {
		uint64_t *arr = (uint64_t *)malloc(get_number_keys(file) * sizeof(*arr));
		if (arr == NULL) {
			std::cout << "Can't allocate memory" << std::endl;
			exit(1);
		}

		std::ifstream dumpfile(file.c_str());
		uint64_t i = 0;
		while (dumpfile >> arr[i++]) {}
		return arr;
		}

		uint64_t get_number_keys(std::string file) {
			std::ifstream infile(file);
			uint64_t num_keys{0};
			if (infile.is_open()) {
				std::string line;
				while (std::getline(infile, line)) { ++num_keys; }
			}

			return num_keys;
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

				PRINT("Number of keys above threshold: " << key_lifetime.size());

				return key_lifetime;
			}
}
