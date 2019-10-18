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
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
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
	if (argc < 3) {
		std::cout << "Please specify nvals and filename options.\n";
		exit(1);
	}
	uint64_t nvals = 1ULL << atoi(argv[1]);
	uint64_t nram = 1ULL << atoi(argv[2]);
	char *filename = argv[3];

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

	RAND_bytes((unsigned char *)vals, sizeof(*vals) * (nvals));
	for (uint64_t i = 0; i < nvals; i++)
		vals[i] = vals[i] % (1ULL << 48);
	

	srandom((int) time(0));
	for (uint64_t i = 0; i < nram; i++) {
		int item = vals[rand() % nvals];
		int cnt = popcornfilter::RandomBetween(24, 50); 
		for (int j = 0; j < cnt; j++) {
			vals[rand()%nvals] = item;
		}
	}

	// unmap
	munmap(vals, size);
	close(fd);

	return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */
