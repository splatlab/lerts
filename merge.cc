/*
 * ============================================================================
 *
 *       Filename:  merge.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  25/03/2017 09:56:26 PM
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
#include <fstream>

#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <openssl/rand.h>

#include "cqf/gqf.h"
#include "hashutil.h"
#include "util.h"

int main(int argc, char **argv)
{
	if (argc < 2) {
			std::cout << "Not suffcient args." << std::endl;
			abort();
	}

	uint32_t flag = atoi(argv[1]);
	struct timeval start1, end1;
	struct timezone tzp;

	if (flag) { // create
		if (argc < 4) {
			std::cout << "Not suffcient args." << std::endl;
			abort();
		}

		QF *cfs;
		uint64_t qbits = atoi(argv[2]);
		uint32_t nfilters = atoi(argv[3]);
		uint64_t nhashbits = qbits + 8;
		uint64_t nslots = (1ULL << qbits);
		uint64_t nvals = 750*nslots/1000;
		cfs = (QF *)calloc(nfilters, sizeof(QF));

		std::cout << "Creating " << nfilters << " CQFs each with " << nslots <<
			" slots on disk" << std::endl;
		uint32_t seed = time(NULL);
		for (uint32_t i = 0; i < nfilters; i++) {
			std::string file("_cqf.ser");
			file = std::to_string(i) + file;
			qf_init(&cfs[i], nslots, nhashbits, 0, false, file.c_str(), seed);
		}

		uint64_t *vals;
		vals = (uint64_t*)malloc(nvals*sizeof(vals[0]));
		for (uint32_t i = 0; i < nfilters; i++) {
			std::cout << "Generating " << nvals << " random numbers." << std::endl;
			memset(vals, 0, nvals*sizeof(vals[0]));
			RAND_pseudo_bytes((unsigned char *)vals, sizeof(*vals) * nvals);
			for (uint32_t k = 0; k < nvals; k++)
				vals[k] = (1 * vals[k]) % cfs[i].metadata->range;

			std::cout << "Inserting items in the CQF: " << i << std::endl;
			gettimeofday(&start1, &tzp);
			for (uint32_t j = 0; j < nvals; j++) {
				qf_insert(&cfs[i], vals[j], 0, 1, /*lock*/false, /*spin*/false);
				uint64_t cnt = qf_count_key_value(&cfs[i], vals[j], 0);
				if (!cnt) {
					std::cout << "Failed lookup while inserting for " <<
						(uint64_t)vals[j] << std::endl;
					abort();
				}
			}
			gettimeofday(&end1, &tzp);
			print_time_elapsed("", &start1, &end1);
			qf_destroy(&cfs[i], false);
		}
	} else { // Merge
		QF *cf_arr[10];	// assuming we won't be merging more than 10 CQFs. 
		QF cfr;
		uint64_t total_occupied_slots = 0;
		uint64_t total_num_elements = 0;
		uint64_t nfilters = argc-2;

		for (int i = 2, j = 0; i < argc; i++, j++) {
			cf_arr[j] = (QF *)calloc(1, sizeof(QF));
			qf_read(cf_arr[j], argv[i]);
			total_occupied_slots += cf_arr[j]->metadata->noccupied_slots;
			total_num_elements += cf_arr[j]->metadata->ndistinct_elts;
			DEBUG_CF("Total occupied slots " << 
									cf_arr[j]->metadata->noccupied_slots);
			DEBUG_CF("Total num elements " <<
									cf_arr[j]->metadata->ndistinct_elts);
		}
	
		uint64_t nhashbits = cf_arr[0]->metadata->key_bits;
		uint32_t seed = cf_arr[0]->metadata->seed;
		uint64_t rqbits = ceil(log2(total_occupied_slots +
																0.05*total_occupied_slots));
		uint64_t rnslots = pow(2, rqbits);
		std::string final_qf("final_qf.ser");
		std::cout << "Creating final CQF with " << rnslots << " slots" << std::endl;
		qf_init(&cfr, rnslots, nhashbits, 0, false, final_qf.c_str(), seed);

		std::cout << "Merging " << total_num_elements << " elements from " <<
			nfilters << " CQFs" << std::endl;
		gettimeofday(&start1, &tzp);
		qf_multi_merge(cf_arr, nfilters, &cfr, false, false);	// no lock and no spin
		gettimeofday(&end1, &tzp);
		print_time_elapsed("", &start1, &end1);
	}
	return 0;
}
