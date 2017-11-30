/*
 * ============================================================================
 *
 *       Filename:  util.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017-09-21 12:39:52 PM
 *       Revision:  none
 *       Compiler:  gcc
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

#include "cqf/gqf.h"

#ifdef DEBUG
#define PRINT_DEBUG 1
#else
#define PRINT_DEBUG 0
#endif

#define DEBUG_CF(x) do { \
	if (PRINT_DEBUG) { std::cerr << x << std::endl; } \
} while (0)

#define PRINT_CF(x) do { \
	{ std::cout << x << std::endl; } \
} while (0)

class LightweightLock {
	public:
		LightweightLock() { locked = 0; }

		/**
		 * Try to acquire a lock once and return even if the lock is busy.
		 * If spin flag is set, then spin until the lock is available.
		 */
		bool lock(enum lock flag)
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

		void unlock(void)
		{
			__sync_lock_release(&locked);
			return;
		}

	private:
		volatile int locked;
};

/* Print elapsed time using the start and end timeval */
void print_time_elapsed(std::string desc, struct timeval* start, struct
												timeval* end);

std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>>
analyze_stream(uint64_t *vals, uint64_t nvals, uint32_t threshold);

uint64_t *read_stream_from_disk(std::string file);

std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>>
read_stream_log_from_disk(std::string file);

#endif
