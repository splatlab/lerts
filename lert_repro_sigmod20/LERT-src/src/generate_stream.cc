/*
 * ============================================================================
 *
 *       Filename:  generate_stream.cc
 *
 *         Author:  Prashant Pandey (), ppandey2@cs.cmu.edu
 *   Organization:  Carnegie Mellon University
 *
 * ============================================================================
 */

#include <openssl/rand.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <random>
#include <cstdlib>
#include <ctime>

#include "util.h"

/* 
 * ===  FUNCTION  =============================================================
 *         Name:  main
 *  Description:  
 * ============================================================================
 */
	int
main (int argc, char *argv[])
{
	if (argc < 4) {
		std::cout << "Please specify nvals and filename options.\n";
		exit(1);
	}
	uint64_t nvals = 1ULL << atoi(argv[1]);
	uint64_t nram = 1ULL << atoi(argv[2]);
	uint32_t option = atoi(argv[3]);
	char *filename = argv[4];

	if (option > 4) {
		std::cout << "Wrong option. Should be b/w 1-4\n";
		exit(1);
	}

	// create a file for data and mmap it.
	uint64_t size = nvals*sizeof(uint64_t);
	int fd = open(filename, O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);
	if (fd < 0) {
		perror("Couldn't open file:");
		exit(1);
	}
	int ret = posix_fallocate(fd, 0, size);
	if (ret < 0) {
		perror("Couldn't fallocate file:\n");
		exit(EXIT_FAILURE);
	}

	uint64_t *vals = (uint64_t *)mmap(NULL, size, PROT_READ | PROT_WRITE,
																		MAP_SHARED, fd, 0);
	if (vals == NULL) {
		perror("Couldn't mmap file:");
		exit(1);
	}
	// zero out the memory
	memset(vals, 0, size);

	srandom((int) time(0));
	if (option == 1) { // M items with count b/w 24-50. Rest uniform-random.
		uint64_t array_cnt = 0;
		// generate M items with count b/w 24-50
		for (uint64_t i = 0; i < nram/8; i++) {
			int item = rand();
			int cnt = popcornfilter::RandomBetween(24, 50); 
			for (int j = 0; j < cnt; j++) {
				vals[array_cnt++] = item;
			}
		}
		// rest of the array with uniform-random.
		while (array_cnt < nvals) {
			vals[array_cnt++] = rand();
		}
	} else if (option == 2) { // M items with count 24. Rest with count 23.
		uint64_t array_cnt = 0;
		// generate M items with count 24
		for (uint64_t i = 0; i < nram/8; i++) {
			int item = rand();
			int cnt = 24; 
			for (int j = 0; j < cnt; j++) {
				vals[array_cnt++] = item;
			}
		}
		// rest of the array with count 23
		while (array_cnt < nvals) {
			int item = rand();
			int cnt = 23; 
			for (int j = 0; j < cnt && array_cnt < nvals; j++) {
				vals[array_cnt++] = item;
			}
		}
	} else if (option == 3) { // M items appearing in round-robin manner
		uint64_t array_cnt = 0;
		// generate M items with count 24
		for (uint64_t i = 0; i < nram; i++) {
			int item = rand();
			vals[array_cnt++] = item;
		}
		uint64_t i = 0;
		while (array_cnt < nvals) {
			vals[array_cnt++] = vals[i++];
			if (i == nram/8)
				i = 0;
		}
		//uint64_t batch = nram * 12;
		//for (uint64_t i = 0; i < batch; i++) { // 16*M random numbers
		//uint64_t item = rand();
		//vals[i] = item;
		//}
		//uint64_t *start = vals + batch;
		//memcpy(start, vals, batch * sizeof(*vals)); // last level full
		//start += batch;

		//batch = nram * 0.75;
		//for (uint64_t i = 1; i <= 4; i++) {
		//memcpy(start, vals, batch * sizeof(*vals));
		//start += batch;
		//}	// 2nd level full

		//batch = nram * 0.75 * 0.375;
		//for (uint64_t i = 1; i <= 8; i++) {
		//memcpy(start, vals, batch * sizeof(*vals));
		//start += batch;
		//}	// 1nd level full

		//batch = nram * 0.75 / 10;
		//for (uint64_t i = 1; i <= 10; i++) {
		//memcpy(start, vals, batch * sizeof(*vals));
		//start += batch;
		//}
		//// rest of the array with uniform-random.
		//uint64_t i = start - vals;
		//while (i < nvals) {
		//vals[i++] = rand();
		//}
	} else if (option == 4) { // items with count randomly picked b/w 1-24
		uint64_t array_cnt = 0;
		while (array_cnt < nvals) {
			int item = rand();
			int cnt = popcornfilter::RandomBetween(1, 25); 
			for (int j = 0; j < cnt && array_cnt < nvals; j++) {
				vals[array_cnt++] = item;
			}
		}
	}

	if (option != 3)
		std::shuffle(vals, vals + nvals, std::default_random_engine(time(0)));

	// popcornfilter::analyze_stream(vals, nvals, 24);
	// unmap
	munmap(vals, size);
	close(fd);
	return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */
