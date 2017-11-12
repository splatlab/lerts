/*
 * ============================================================================
 *
 *       Filename:  main.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2016-11-10 03:31:54 PM
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
#include <fstream>

#include <time.h>
#include <stdio.h>
#include	<stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <fcntl.h>

/* 
 * ===  FUNCTION  =============================================================
 *         Name:  main
 *  Description:  
 * ============================================================================
 */
	int
main ( int argc, char *argv[] )
{
	struct stat sb;

	uint64_t *arr;
	std::string filename("raw/streamdump");
	uint64_t cnt = 250000000;
	std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>> key_lifetime;
	std::multiset<uint64_t> key_counts;
	
	int fd = open(filename.c_str(), O_RDWR, S_IRWXU);
	if (fd < 0) {
		perror("Couldn't open file");
		exit(EXIT_FAILURE);
	}

	int ret = fstat (fd, &sb);
	if ( ret < 0) {
		perror ("Couldn't fstat file");
		exit(EXIT_FAILURE);
	}

	if (!S_ISREG (sb.st_mode)) {
		std::cerr << filename << " is not a file" << std::endl;
		exit(EXIT_FAILURE);
	}

	arr = (uint64_t *)mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED,
																fd, 0);

	for (uint32_t i = 0; i < cnt; i++) {
		uint64_t key = arr[i] >> 1;
		if (key_counts.count(key) == 0) {
			key_counts.insert(key);
			key_lifetime[key] = std::pair<uint64_t, uint64_t>(i, i);
		} else if (key_counts.count(key) < 23) {
			key_counts.insert(key);
		} else if (key_counts.count(key) == 23) {
			key_counts.insert(key);
			key_lifetime[key].second = i;
		}
	}

	std::ofstream stats("raw/stream_stats");
	stats << "Key Birthday_index T_index" << std::endl;
	for (auto it : key_lifetime) {
		assert (it.second.first <= it.second.second);
		if (it.second.first < it.second.second)
			stats << it.first << " " << it.second.first << " " <<
				it.second.second << std::endl;
	}
	stats.close();

	munmap(arr, sb.st_size);
	close(fd);

	return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */
