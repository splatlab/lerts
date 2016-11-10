#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <openssl/rand.h>

#include <gqf.h>

uint64_t tv2msec(struct timeval tv)
{
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int cmp_uint64_t(const void *a, const void *b)
{
	const uint64_t *ua = (const uint64_t*)a, *ub = (const uint64_t *)b;
	return *ua < *ub ? -1 : *ua == *ub ? 0 : 1;
}

typedef struct insert_args {
	QF *cf;
	uint64_t *vals;
	int freq;
	uint64_t start;
	uint64_t end;
} insert_args;

void *ds_insert(void *args)
{
	insert_args *a = (insert_args *)args;
	fprintf(stdout, "Entering thread with bounds: %ld %ld\n", a->start, a->end);
	for (int i = a->start; i < a->end; i++) {
		/*fprintf(stdout, "Inserting: %ld\n", a->vals[i]);*/
		qf_insert(a->cf, a->vals[i], 0, a->freq);
	}
	return NULL;
}

int main(int argc, char **argv)
{
	QF cfa, cfb, cfc, cfr;
	QF *qf_arr[3];
	QFi cfia, cfib, cfic, cfir;
	uint64_t qbits = atoi(argv[1]);
	/*uint64_t rqbits = qbits+2;*/
	/*uint64_t cbits = atoi(argv[2]);*/
	int freq = atoi(argv[2]);
	uint64_t nslots = (1ULL << qbits);
	/*uint64_t rnslots = (1ULL << rqbits);*/
	uint64_t nvals = 950*nslots/1000;
	uint64_t *valsa, *valsb, *valsc, *valsd;
	uint64_t i, j;
	pthread_t thread1, thread2, thread3, thread4;
	FILE *fp_tmp_itr = fopen("tmp_itr.txt", "w");
	FILE *fp_wait_time = fopen("wait_time.txt", "w");
	/*uint64_t *othervals;*/
	/*unsigned int exp;*/
	/*FILE *fp_insert = fopen("../vldb16/data/qf-insert-temp.txt", "w");*/
	/*FILE *fp_exit_lookup = fopen("../vldb16/data/qf-exists-lookup-temp.txt", "w");*/
	/*FILE *fp_false_lookup = fopen("../vldb16/data/qf-false-lookup-temp.txt", "w");*/
	/*struct timeval tv_insert[40];*/
	/*struct timeval tv_exit_lookup[40];*/
	/*struct timeval tv_false_lookup[40];*/
	/*uint64_t fps = 0;*/

	if (fp_tmp_itr == NULL) {
		printf("Can't open the data file");
		exit(1);
	}
	if (fp_wait_time == NULL) {
		printf("Can't open the log file");
		exit(1);
	}
	
	/*if (fp_insert == NULL || fp_exit_lookup == NULL || fp_false_lookup == NULL) {*/
		/*printf("Can't open the data file");*/
		/*exit(1);*/
	/*}*/

	/*qf_init(&cfa, nslots, qbits+8, 0);*/
	/*qf_init(&cfb, nslots, qbits+8, 0);*/
	/*qf_init(&cfc, nslots, qbits+8, 0);*/

	qf_init(&cfr, nslots, qbits+BITS_PER_SLOT, 0);

	valsa = calloc(nvals, sizeof(valsa[0]));
	RAND_pseudo_bytes((unsigned char *)valsa, sizeof(*valsa) * nvals);
	for (i = 0; i < nvals; i++)
		valsa[i] = (1 * valsa[i]) % cfr.range;

	/*valsb = calloc(nvals, sizeof(valsb[0]));*/
	/*RAND_pseudo_bytes((unsigned char *)valsb, sizeof(*valsb) * nvals/cbits);*/
	/*for (i = 0; i < nvals/cbits; i++)*/
		/*valsb[i] = (1 * valsb[i]) % cfr.range;*/

	/*valsc = calloc(nvals, sizeof(valsc[0]));*/
	/*RAND_pseudo_bytes((unsigned char *)valsc, sizeof(*valsc) * nvals/cbits);*/
	/*for (i = 0; i < nvals/cbits; i++)*/
		/*valsc[i] = (1 * valsc[i]) % cfr.range;*/
	
	/*valsd = calloc(nvals, sizeof(valsd[0]));*/
	/*RAND_pseudo_bytes((unsigned char *)valsd, sizeof(*valsd) * nvals/cbits);*/
	/*for (i = 0; i < nvals/cbits; i++)*/
		/*valsc[i] = (1 * valsd[i]) % cfr.range;*/

	fprintf(stdout, "Total number of items: %ld\n", nvals);
	
	insert_args args1, args2, args3, args4;
	args1.cf = &cfr; args1.vals = valsa; args1.freq = freq; args1.start = 0; args1.end = (nvals/4);
	args2.cf = &cfr; args2.vals = valsa; args2.freq = freq; args2.start = nvals/4+1; args2.end = nvals/2;
	args3.cf = &cfr; args3.vals = valsa; args3.freq = freq; args3.start = nvals/2+1; args3.end = 3*nvals/4;
	args4.cf = &cfr; args4.vals = valsa; args4.freq = freq; args4.start = 3*nvals/4+1; args4.end = nvals;
	if (pthread_create(&thread1, NULL, &ds_insert, &args1)) {
		fprintf(stderr, "Error creating thread1\n");
		exit(0);
	}
	if (pthread_create(&thread2, NULL, &ds_insert, &args2)) {
		fprintf(stderr, "Error creating thread2\n");
		exit(0);
	}
	if (pthread_create(&thread3, NULL, &ds_insert, &args3)) {
		fprintf(stderr, "Error creating thread3\n");
		exit(0);
	}
	if (pthread_create(&thread4, NULL, &ds_insert, &args4)) {
		fprintf(stderr, "Error creating thread4\n");
		exit(0);
	}
	
	if (pthread_join(thread1, NULL)) {
		fprintf(stderr, "Error joining thread1\n");
		exit(0);
	}
	if (pthread_join(thread2, NULL)) {
		fprintf(stderr, "Error joining thread2\n");
		exit(0);
	}
	if (pthread_join(thread3, NULL)) {
		fprintf(stderr, "Error joining thread3\n");
		exit(0);
	}
	if (pthread_join(thread4, NULL)) {
		fprintf(stderr, "Error joining thread4\n");
		exit(0);
	}
	
	fprintf(stdout, "Inserted all items: %ld\n", nvals);
#if 0
	for (i=0; i<nvals/cbits; i++) {
		uint64_t cnt = qf_count_key_value(&cfr, valsa[i], 0);
		if (cnt < freq) {
			fprintf(stderr, "Failed lookup while inserting for %ld. Expexted: %d. Returned: %ld\n", valsa[i], freq, cnt);
			abort();
		}
	}
	for (i=0; i<nvals/cbits; i++) {
		uint64_t cnt = qf_count_key_value(&cfr, valsb[i], 0);
		if (cnt < freq) {
			fprintf(stderr, "Failed lookup while inserting for %ld. Expexted: %d. Returned: %ld\n", valsb[i], freq, cnt);
			abort();
		}
	}
	for (i=0; i<nvals/cbits; i++) {
		uint64_t cnt = qf_count_key_value(&cfr, valsc[i], 0);
		if (cnt < freq) {
			fprintf(stderr, "Failed lookup while inserting for %ld. Expexted: %d. Returned: %ld\n", valsc[i], freq, cnt);
			abort();
		}
	}
	fprintf(stdout, "Verified all items: %ld\n", nvals/cbits);

	for (i=0; i<nvals/cbits; i++) {
		for (j=0; j < cbits; j++) {
			qf_insert(&cfa, valsa[i], 0, freq);
			uint64_t cnt = qf_count_key_value(&cfa, valsa[i], 0);
			if (cnt != ((j+1)*freq)) {
				fprintf(stderr, "Failed lookup while inserting for %ld. Expexted: %d. Returned: %ld\n", valsa[i], (j+1)*freq, cnt);
				abort();
			}
		}
	}
	for (i=0; i<nvals/cbits; i++) {
		for (j=0; j < cbits; j++) {
			qf_insert(&cfb, valsb[i], 0, freq);
			uint64_t cnt = qf_count_key_value(&cfb, valsb[i], 0);
			if (cnt != ((j+1)*freq)) {
				fprintf(stderr, "Failed lookup while inserting for %ld. Expexted: %d. Returned: %ld\n", valsb[i], (j+1)*freq, cnt);
				abort();
			}
		}
	}
	for (i=0; i<nvals/cbits; i++) {
		for (j=0; j < cbits; j++) {
			qf_insert(&cfc, valsc[i], 0, freq);
			uint64_t cnt = qf_count_key_value(&cfc, valsc[i], 0);
			if (cnt != ((j+1)*freq)) {
				fprintf(stderr, "Failed lookup while inserting for %ld. Expexted: %d. Returned: %ld\n", valsc[i], (j+1)*freq, cnt);
				abort();
			}
		}
	}

	for (i=0; i<nvals/cbits; i++) {
		if (qf_count_key_value(&cfa, valsa[i], 0) < freq*cbits) {
			fprintf(stderr, "Failed lookup for %ld\n", valsa[i]);
			abort();
		}
	}
	for (i=0; i<nvals/cbits; i++) {
		if (qf_count_key_value(&cfb, valsb[i], 0) < freq*cbits) {
			fprintf(stderr, "Failed lookup for %ld\n", valsb[i]);
			abort();
		}
	}
	for (i=0; i<nvals/cbits; i++) {
		if (qf_count_key_value(&cfc, valsc[i], 0) < freq*cbits) {
			fprintf(stderr, "Failed lookup for %ld\n", valsc[i]);
			abort();
		}
	}

	qf_iterator(&cfa, &cfia, 0);
	do {
		uint64_t key, value, count;
		qfi_get(&cfia, &key, &value, &count);
		if (qf_count_key_value(&cfa, key, 0) < freq*cbits) {
			fprintf(stderr, "Failed lookup from A for: %ld. Returned count: %ld\n", key, qf_count_key_value(&cfa, key, 0));
			abort();
		}
	} while(!qfi_next(&cfia));
	qf_iterator(&cfb, &cfib, 0);
	do {
		uint64_t key, value, count;
		qfi_get(&cfib, &key, &value, &count);
		if (qf_count_key_value(&cfb, key, 0) < freq*cbits) {
			fprintf(stderr, "Failed lookup from B for: %ld. Returned count: %ld\n", key, qf_count_key_value(&cfb, key, 0));
			abort();
		}
	} while(!qfi_next(&cfib));
	qf_iterator(&cfc, &cfic, 0);
	do {
		uint64_t key, value, count;
		qfi_get(&cfic, &key, &value, &count);
		if (qf_count_key_value(&cfc, key, 0) < freq*cbits) {
			fprintf(stderr, "Failed lookup from C for: %ld. Returned count: %ld\n", key, qf_count_key_value(&cfc, key, 0));
			abort();
		}
	} while(!qfi_next(&cfic));

	/*for (exp = 0; exp < 40; exp += 2) {*/
		/*i = (exp/2)*(nvals/20);*/
		/*j = ((exp/2) + 1)*(nvals/20);*/
		/*gettimeofday(&tv_insert[exp], NULL);*/
		/*for (;i < j; i++) {*/
			/*qf_insert(&cf, vals[i], 0, 1);*/
		/*}*/
		/*gettimeofday(&tv_insert[exp+1], NULL);*/

		/*i = (exp/2)*(nvals/20);*/
		/*gettimeofday(&tv_exit_lookup[exp], NULL);*/
		/*for (;i < j; i++) {*/
		/*if (!qf_count_key_value(&cf, vals[i], 0)) {*/
				/*fprintf(stderr, "Failed lookup for 0x%lx\n", vals[i]);*/
				/*abort();*/
			/*}*/
		/*}*/
		/*gettimeofday(&tv_exit_lookup[exp+1], NULL);*/

		/*i = (exp/2)*(nvals/20);*/
		/*gettimeofday(&tv_false_lookup[exp], NULL);*/
		/*for (;i < j; i++) {*/
			/*fps += qf_count_key_value(&cf, othervals[i], 0);*/
		/*}*/
		/*gettimeofday(&tv_false_lookup[exp+1], NULL);*/
	/*}*/

	/*qf_merge(&cfa, &cfb, &cfr);*/
	qf_arr[0] = &cfa; qf_arr[1] = &cfb; qf_arr[2] = &cfc;
	qf_multi_merge(qf_arr, 3, &cfr);
	fprintf(stdout, "Total number of items after merging: %ld\n", cfr.ndistinct_elts);

	fprintf(stdout, "Verify that all items are present in the merged QF \n");
	for (i=0; i<nvals/cbits; i++) {
		if (qf_count_key_value(&cfr, valsa[i], 0) != freq*cbits) {
			fprintf(stderr, "Failed lookup from A for: %ld. Returned count: %ld\n", valsa[i], qf_count_key_value(&cfr, valsa[i], 0));
			abort();
		}
	}
	for (i=0; i<nvals/cbits; i++) {
		if (qf_count_key_value(&cfr, valsb[i], 0) != freq*cbits) {
			fprintf(stderr, "Failed lookup from B for: %ld. Returned count: %ld\n", valsb[i], qf_count_key_value(&cfr, valsb[i], 0));
			abort();
		}
	}
	for (i=0; i<nvals/cbits; i++) {
		if (qf_count_key_value(&cfr, valsc[i], 0) != freq*cbits) {
			fprintf(stderr, "Failed lookup from C for: %ld. Returned count: %ld\n", valsc[i], qf_count_key_value(&cfr, valsc[i], 0));
			abort();
		}
	}


	qf_iterator(&cfr, &cfir, 0);
	do {
		uint64_t key, value, count;
		qfi_get(&cfir, &key, &value, &count);
		if (count > 1)
			fprintf(fp_tmp_itr, "%ld %ld\n", key, count);
		/*qfi_next(&cfi);*/
	} while(!qfi_next(&cfir));
	fclose(fp_tmp_itr);

#endif

	for (i=0; i<cfr.num_locks; i++) {
		fprintf(fp_wait_time, "%ld %ld\n", cfr.wait_times[i].total_time, cfr.wait_times[i].locks_taken);
	}
	fclose(fp_wait_time);



	/*fprintf(fp_insert, "x_0    y_0\n");*/
	/*for (exp = 0; exp < 40; exp += 2) {*/
		/*fprintf(fp_insert, "%d %f\n",*/
						/*((exp/2)*5), 0.001 * (nvals/20)/(tv2msec(tv_insert[exp+1]) - tv2msec(tv_insert[exp])));*/
	/*}*/
	/*printf("\n Insert Performance written");*/
	/*fprintf(fp_exit_lookup, "x_0    y_0\n");*/
	/*for (exp = 0; exp < 40; exp += 2) {*/
		/*fprintf(fp_exit_lookup, "%d %f\n",*/
						/*((exp/2)*5), 0.001 * (nvals/20)/(tv2msec(tv_exit_lookup[exp+1]) - tv2msec(tv_exit_lookup[exp])));*/
	/*}*/
	/*printf("\n Existing Lookup Performance written");*/
	/*fprintf(fp_false_lookup, "x_0    y_0\n");*/
	/*for (exp = 0; exp < 40; exp += 2) {*/
		/*fprintf(fp_false_lookup, "%d %f\n",*/
						/*((exp/2)*5), 0.001 * (nvals/20)/(tv2msec(tv_false_lookup[exp+1]) - tv2msec(tv_false_lookup[exp])));*/
	/*}*/
	/*printf("\n False Lookup Performance written");*/

	/*
		 printf("Insert:          %f ins/sec\n"
		 "Lookup existing: %f lookups/sec\n"
		 "Lookup random:   %f lookus/sec\n",
		 1000.0 * nvals / (tv2msec(tv[1]) - tv2msec(tv[0])),
		 1000.0 * nvals / (tv2msec(tv[2]) - tv2msec(tv[1])),
		 1000.0 * nvals / (tv2msec(tv[3]) - tv2msec(tv[2])));
		 */

	/*printf("\nFP rate: %f (%lu/%lu)\n", 1.0 * fps / nvals, fps, nvals);*/

	/*fclose(fp_insert);*/
	/*fclose(fp_exit_lookup);*/
	/*fclose(fp_false_lookup); */

	return 0;
}
