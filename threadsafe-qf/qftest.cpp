#include <cinttypes>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <string.h>
#include <openssl/rand.h>
#include "qf.h"

using namespace std;

int main(int argc, char **argv)
{
	QF qf;
	unsigned int verbosity, nquotientbits, nslots;
	uint64_t range;
	uint64_t ndistinct_hash_values;
	uint64_t *distinct_hash_values = NULL;
	uint64_t i;

	srand(time(NULL));

	verbosity = atoi(argv[1]);
	nslots = atoi(argv[2]);
	nquotientbits = atoi(argv[3]);
	ndistinct_hash_values = atoi(argv[4]);

	qf_init(&qf, 1ULL << nslots, nslots + nquotientbits, 0);

	range = qf.range;

	if (ndistinct_hash_values == 0)
		ndistinct_hash_values = qf.range;

	distinct_hash_values = (uint64_t *)malloc(ndistinct_hash_values * sizeof(distinct_hash_values[0]));
	RAND_pseudo_bytes((unsigned char *)distinct_hash_values, sizeof(*distinct_hash_values) * ndistinct_hash_values);
	if (ndistinct_hash_values == qf.range)
		for (i = 0; i < ndistinct_hash_values; i++)
			distinct_hash_values[i] = i;    
	else
		for (i = 0; i < ndistinct_hash_values; i++)
			distinct_hash_values[i] = distinct_hash_values[i] % qf.range;

	std::unordered_map<uint64_t, uint64_t> MS;

	/* Perform random operations on QF and MS, checking that the results
		 are always equivalent. */
	i = 0;

	int64_t append_state = -1;
	uint64_t run_start = UINT64_MAX, current = UINT64_MAX;

	while (1) {
		i++;

		if (verbosity > 0)
			std::cout << "STEP " << i << " QF size: " << qf.nelts << " MS size: " << MS.size() << std::endl;
		if (verbosity > 2)
			qf_dump(&qf);
		// {
		//   std::multiset<uint64_t> qfms = QF.to_multiset();
		//   if (verbosity > 1) {
		// 	std::multiset<uint64_t>::iterator it;
		// 	printf("{ ");
		// 	for (it = qfms.begin(); it != qfms.end(); it++)
		// 	  printf("%3" PRId64 " ", (uint64_t)(*it));
		// 	printf(" }\n");
		//   }
		//   if (qfms != MS) {
		// 	exit(0);
		//   }
		// }

		if (qf.noccupied_slots > qf.nslots)
			break;

		uint64_t hash = rand() % qf.range;

		switch (rand() % 2) {
			case 0:
				if (verbosity > 0)
					std::cout << "Lookup " << std::hex << hash << std::dec << std::endl;
				if (qf_count_key_value(&qf, hash, 0) > 0 && 
						qf_count_key_value(&qf, hash, 0) != MS[hash]) {
					std::cout << i << " Lookup error: " << std::hex << hash << std::dec << " " 
						<< qf_count_key_value(&qf, hash, 0) << " " << MS[hash] << std::endl;
					exit(0);
				}
				break;

				/* case 1: */
				/*   { */
				/* 	if (verbosity > 0) */
				/* 	  std::cout << "Erase existing " << hash << std::endl; */
				/* 	std::multiset<uint64_t>::iterator msit = MS.lower_bound(hash); */
				/* 	if (msit == MS.end()) */
				/* 	  continue; */

				/* 	hash = *msit; */
				/*   } */
				/*   // fall-through  */

				/* case 2: */
				/*   { */
				/* 	if (verbosity > 0) */
				/* 	  std::cout << "Erase " << hash << std::endl; */
				/* 	int qfr = QF.erase1(hash); */
				/* 	std::multiset<uint64_t>::iterator msit = MS.find(hash); */
				/* 	int msr = msit != MS.end(); */
				/* 	if (msr) */
				/* 	  MS.erase(msit); */

				/* 	if (qfr != msr) { */
				/* 	  std::cout << i << " Delete error: " << hash << " "  */
				/* 		    << qfr << " " << msr << std::endl; */
				/* 	  exit(0); */
				/* 	} */

				/* 	if (QF.size() == 0) { */
				/* 	  append_state = -1; */
				/* 	  run_start = current = UINT64_MAX; */
				/* 	} else { */
				/* 	  append_state = -2; */
				/* 	} */
				/*   } */
				/*   break; */

			case 1:
				hash = distinct_hash_values[hash % ndistinct_hash_values];

				uint64_t count = 1ULL << (rand() % 26);
				count = (rand() % count) + count;
				if (verbosity > 0)
					std::cout << "Insert " << std::hex << hash << " " << count << std::dec << std::endl;
				qf_insert(&qf, hash, 0, count);
				MS[hash] = MS[hash] + count;
				append_state = -2;
				break;

				/* case 4: */
				/*   if (qf.noccupied_slots < nslots-1 && append_state >= -1 && append_state < (int64_t)hash) { */
				/* 	if (verbosity > 0) */
				/* 	  std::cout << "Append " << hash << std::endl; */
				/* 	QF.append(hash, run_start, current); */
				/* 	MS.insert(hash); */
				/* 	append_state = hash; */
				/*   } */
				/*   break; */
		}
	}

	return 0;
}
