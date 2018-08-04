/*
 * ============================================================================
 *
 *         Author:  Prashant Pandey (), ppandey@cs.stonybrook.edu
 *   Organization:  Stony Brook University
 *
 * ============================================================================
 */

#ifndef _UTIL_H_
#define _UTIL_H_

#include <iostream>
#include <cstring>
#include <vector>
#include <cassert>
#include <fstream>
#include <unordered_map>

#ifdef DEBUG
#define PRINT_DEBUG 1
#else
#define PRINT_DEBUG 0
#endif

#define DEBUG(x) do { \
	if (PRINT_DEBUG) { std::cerr << x << std::endl; } \
} while (0)

#define ERROR(x) do { \
	{ std::cerr << x << std::endl; } \
} while (0)

#define PRINT(x) do { \
	{ std::cout << x << std::endl; } \
} while (0)

namespace popcornfilter {
	/* Print elapsed time using the start and end timeval */
	void print_time_elapsed(std::string desc, struct timeval* start, struct
													timeval* end);

	std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>>
		analyze_stream(uint64_t *vals, uint64_t nvals, uint32_t threshold);

	uint64_t *read_stream_from_disk(std::string file);
	uint64_t get_number_keys(std::string file);

	std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>>
		read_stream_log_from_disk(std::string file);
}
#endif