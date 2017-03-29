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

#include "threadsafe-gqf/gqf.h"
#include "hashutil.h"

using namespace std;

/* Print elapsed time using the start and end timeval */
void print_time_elapsed(string desc, struct timeval* start, struct timeval* end)
{
	struct timeval elapsed;
	if (start->tv_usec > end->tv_usec) {
		end->tv_usec += 1000000;
		end->tv_sec--;
	}
	elapsed.tv_usec = end->tv_usec - start->tv_usec;
	elapsed.tv_sec = end->tv_sec - start->tv_sec;
	float time_elapsed = (elapsed.tv_sec * 1000000 + elapsed.tv_usec)/1000000.f;
	cout << desc << "Total Time Elapsed: " << to_string(time_elapsed) << 
		"seconds" << endl;
}

int main(int argc, char **argv)
{
	if (argc < 2) {
			cout << "Not suffcient args." << endl;
			abort();
	}

	uint32_t flag = atoi(argv[1]);
	struct timeval start1, end1;
	struct timezone tzp;

	if (flag) { // create
		if (argc < 4) {
			cout << "Not suffcient args." << endl;
			abort();
		}

		QF *cfs;
		uint64_t qbits = atoi(argv[2]);
		uint32_t nfilters = atoi(argv[3]);
		uint64_t nhashbits = qbits + 8;
		uint64_t nslots = (1ULL << qbits);
		uint64_t nvals = 750*nslots/1000;
		cfs = (QF *)calloc(nfilters, sizeof(QF));

		cout << "Creating " << nfilters << " CQFs each with " << nslots << " slots \
			on disk" << endl;
		uint32_t seed = time(NULL);
		for (uint32_t i = 0; i < nfilters; i++) {
			string file("_cqf.ser");
			file = std::to_string(i) + file;
			qf_init(&cfs[i], nslots, nhashbits, 0, false, file.c_str(), seed);
		}

		__uint128_t *vals;
		vals = (__uint128_t*)malloc(nvals*sizeof(vals[0]));
		for (uint32_t i = 0; i < nfilters; i++) {
			cout << "Generating random numbers." << endl;
			memset(vals, 0, nvals*sizeof(vals[0]));
			RAND_pseudo_bytes((unsigned char *)vals, sizeof(*vals) * nvals);
			for (uint32_t k = 0; k < nvals; k++)
				vals[k] = (1 * vals[k]) % cfs[i].metadata->range;

			cout << "Inserting items in the " << i << " CQF." << endl;
			gettimeofday(&start1, &tzp);
			for (uint32_t j = 0; j < nvals; j++) {
				qf_insert(&cfs[i], vals[j], 0, 1, false, false);
				uint64_t cnt = qf_count_key_value(&cfs[i], vals[j], 0);
				if (!cnt) {
					cout << "Failed lookup while inserting for " << (uint64_t)vals[j] <<
						endl;
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
		uint64_t nfilters = 0;

		for (int i = 2, j = 0; i < argc; i++, j++) {
			qf_read(cf_arr[j], argv[i]);
			nfilters++;
			total_occupied_slots += cf_arr[j]->metadata->noccupied_slots;
		}

		uint64_t nhashbits = cf_arr[0]->metadata->key_bits;
		uint32_t seed = cf_arr[0]->metadata->seed;
		uint64_t rqbits = ceil(log2(total_occupied_slots + 0.05*total_occupied_slots));
		uint64_t rnslots = pow(2, rqbits);
		string final_qf("final_qf.ser");
		cout << "Creating final CQFs with " << rnslots << " slots" << endl;
		qf_init(&cfr, rnslots, nhashbits, 0, false, final_qf.c_str(), seed);

		cout << "Merging CQFs" << endl;
		gettimeofday(&start1, &tzp);
		qf_multi_merge(cf_arr, nfilters, &cfr);
		gettimeofday(&end1, &tzp);
		print_time_elapsed("", &start1, &end1);
	}
	return 0;
}
