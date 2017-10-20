/*
 * ============================================================================
 *
 *       Filename:  cascadefilter.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017-10-01 11:38:44 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Prashant Pandey (), ppandey@cs.stonybrook.edu
 *   Organization:  Stony Brook University
 *
 * ============================================================================
 */

#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>
#include <set>
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

#if 0
/* 
 * ===  FUNCTION  =============================================================
 *         Name:  main
 *  Description:  
 * ===========================================================================
 */
	int
main ( int argc, char *argv[] )
{
	if (argc < 4) {
		std::cout << "Not suffcient args." << std::endl;
		abort();
	}

	uint64_t qbits = atoi(argv[1]);
	uint32_t nlevels = atoi(argv[2]);
	uint32_t gfactor = atoi(argv[3]);
	uint32_t nhashbits = qbits + 10;
	uint32_t randominput = 1;	// Default value is uniform-random distribution.
	if (argc > 4)
		randominput = atoi(argv[4]);

	uint64_t sizes[nlevels];
	uint32_t thlds[nlevels];

	struct timeval start, end;
	struct timezone tzp;

	/* level sizes grow by a factor "r". */
	sizes[0] = (1ULL << qbits);
	for (uint32_t i = 1; i < nlevels; i++)
		sizes[i] = pow(gfactor, i) * sizes[0];

	uint64_t nvals = 750 * (sizes[nlevels - 1]) / 1000;

	thlds[nlevels - 1] = 1;
	uint32_t j = 1;
	/* taus grow with r^0.5. */
	uint32_t tau_ratio = sqrt(gfactor);
	for (int32_t i = nlevels - 2; i >= 0; i--, j++)
		thlds[i] = pow(tau_ratio, j) * thlds[nlevels - 1];

	/* Create a cascade filter. */
	std::cout << "Create a cascade filter with " << nhashbits << "-bit hashes, "
		<< nlevels << " levels, and " << gfactor << " as growth factor." <<
		std::endl;
	CascadeFilter<KeyObject> cf(nhashbits, thlds, sizes, nlevels);

	uint64_t *vals;
	vals = (uint64_t*)malloc(nvals*sizeof(vals[0]));
	std::cout << "Generating " << nvals << " random numbers." << std::endl;
	memset(vals, 0, nvals*sizeof(vals[0]));

	if (randominput) {
		/* Generate random keys from a uniform-random distribution. */
		RAND_pseudo_bytes((unsigned char *)vals, sizeof(*vals) * nvals);
		for (uint64_t k = 0; k < nvals; k++)
		vals[k] = (1 * vals[k]) % cf.get_filter(0)->metadata->range;
	} else {
		/* Generate random keys from a Zipfian distribution. */
		generate_random_keys(vals, nvals, nvals, 1.5);
		for (uint64_t i = 0; i < nvals; i++) {
			vals[i] = HashUtil::AES_HASH(vals[i]) % cf.get_filter(0)->metadata->range;
		}
	}
	std::cout << "Inserting elements." << std::endl;
	gettimeofday(&start, &tzp);
	for (uint64_t k = 0; k < nvals; k++)
		if (!cf.insert(KeyObject(vals[k], 0, 1, 0), LOCK_AND_SPIN)) {
			std::cerr << "Failed insertion for " <<
				(uint64_t)vals[k] << std::endl;
			abort();
		}
	gettimeofday(&end, &tzp);
	print_time_elapsed("", &start, &end);
	std::cout << "Finished insertions." << std::endl;

	DEBUG_CF("Number of elements in the CascadeFilter " << cf.get_num_elements());

	//QF new_cf;
	//qf_init(&new_cf, sizes[nlevels - 1], nhashbits, 0, [>mem<] true,
					//"", 0);
	//for (auto it = cf.begin(nlevels); it != cf.end(); ++it) {
		//KeyObject k; 
		//k = *it;
		//qf_insert(&new_cf, k.key, k.value, k.count, LOCK_AND_SPIN);
	//}
	//for (uint64_t k = 0; k < nvals; k++)
		//if (qf_count_key_value(&new_cf, vals[k], 0) < 1) {
			//std::cerr << "Failed lookup for " <<
				//(uint64_t)vals[k] << " " << k << " " << nvals << std::endl;
			//abort();
		//}
	//DEBUG_CF("Iterator verified!");

	std::cout << "Querying elements." << std::endl;
	gettimeofday(&start, &tzp);
	for (uint64_t k = 0; k < nvals; k++)
		if (cf.count_key_value(KeyObject(vals[k], 0, 0, 0)) < 1) {
			std::cerr << "Failed lookup for " <<
				(uint64_t)vals[k] << " " << k << " " << nvals << std::endl;
			abort();
		}
	gettimeofday(&end, &tzp);
	print_time_elapsed("", &start, &end);
	std::cout << "Finished lookups." << std::endl;

	return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */
#endif
