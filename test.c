#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <openssl/rand.h>

#include "threadsafe-gqf/gqf.h"

uint64_t tv2msec(struct timeval tv)
{
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int cmp_uint64_t(const void *a, const void *b)
{
	const uint64_t *ua = (const uint64_t*)a, *ub = (const uint64_t *)b;
	return *ua < *ub ? -1 : *ua == *ub ? 0 : 1;
}

int main(int argc, char **argv)
{
	QF cf;
	/*QFi cfi;*/
	uint64_t qbits = atoi(argv[1]);
	int mem = atoi(argv[2]);
	/*uint64_t nfilters = atoi(argv[2]);*/
	/*uint64_t total_occupied_slots = 0;*/
	/*uint64_t rqbits = qbits+ ceil(nfilters/2.0);*/
	uint64_t nhashbits = qbits + 8;
	uint64_t nslots = (1ULL << qbits);
	/*uint64_t rnslots = (1ULL << rqbits);*/
	uint64_t nvals = 85*nslots/100;
	/*nvals /= 5;*/
	uint64_t *vals;
	/*unsigned int i, j;*/
	/*FILE *fp_tmp_itr = fopen("tmp_itr.txt", "w");*/
	/*uint64_t *othervals;*/
	/*unsigned int exp;*/
	/*FILE *log_shift = fopen("shit_log.log", "w+");*/
	/*FILE *fp_insert = fopen("../vldb16/data/qf-insert-temp.txt", "w");*/
	/*FILE *fp_exit_lookup = fopen("../vldb16/data/qf-exists-lookup-temp.txt", "w");*/
	/*FILE *fp_false_lookup = fopen("../vldb16/data/qf-false-lookup-temp.txt", "w");*/
	/*struct timeval tv_insert[2];*/
	/*struct timeval tv_exit_lookup[40];*/
	/*struct timeval tv_false_lookup[40];*/
	/*uint64_t fps = 0;*/

	/*if (fp_tmp_itr == NULL) {*/
	/*printf("Can't open the data file");*/
	/*exit(1);*/
	/*}*/

	/*if (fp_insert == NULL || fp_exit_lookup == NULL || fp_false_lookup == NULL) {*/
	/*printf("Can't open the data file");*/
	/*exit(1);*/
	/*}*/

	/*printf("Creating %ld QFs with %ld slots\n", nfilters, nslots);*/
	/*for (uint32_t i = 0; i < nfilters; i++) {*/
	/*qf_init(&cfs[i], nslots, nhashbits, 0);*/
	/*qf_arr[i] = &cfs[i];*/
	/*}*/

	/*qf_init(&cf, nslots, nhashbits, 0);*/
	qf_init(&cf, nslots, nhashbits, 0, mem, "test_qf.ser", 23425);

	vals = (uint64_t*)malloc(nvals*sizeof(vals[0]));
	RAND_pseudo_bytes((unsigned char *)vals, sizeof(*vals) * nvals);
	for (uint64_t i = 0; i < nvals; i++) {
		vals[i] = (1 * vals[i]) % cf.metadata->range;
	}

	/*for (uint64_t i = 0; i < 5; i++) {*/
	for (uint64_t j = 0; j < nvals; j++) {
		qf_insert(&cf, vals[j], 0, 1, false, false);
	}
	fprintf(stdout, "Inserted all items.\n");
	for (uint64_t j = 0; j < nvals; j++) {
		uint64_t count = qf_count_key_value(&cf, vals[j], 0);
		if (!count) {
			fprintf(stderr, "failed lookup after insertion for %lx %ld.\n", vals[j], count);
			abort();
		}
	}
	fprintf(stdout, "Verified all items.\n");
	qf_destroy(&cf, false);

	if (!mem) {
		QF cf_read;
		qf_read(&cf_read, "test_qf.ser");
		fprintf(stdout, "Reading CQF from disk. \n");
		for (uint64_t j = 0; j < nvals; j++) {
			uint64_t count = qf_count_key_value(&cf_read, vals[j], 0);
			if (!count) {
				fprintf(stderr, "failed lookup after reading for %lx %ld.\n", vals[j], count);
				abort();
			}
		}
		QFi cfi;
		qf_iterator(&cf_read, &cfi, 0);
		fprintf(stdout, "Iterating over hashes in the CQF. \n");
		do {
			uint64_t key, value, count;
			qfi_get(&cfi, &key, &value, &count);
			count = qf_count_key_value(&cf_read, key, 0);
			if (!count) {
				fprintf(stderr, "failed lookup after during iteration for %lx %ld.\n", key, count);
				abort();
			}
		} while(!qfi_next(&cfi));
		fprintf(stdout, "Verified all items.\n");
	}

	/*qf_dump(&cf);*/
#if 0
	srand(time(NULL));
	for (uint64_t j = 0; j < nvals; j++) {
		uint64_t count = qf_count_key_value(&cf, vals[j], 0);
		if (count > 0) {
			/*fprintf(stdout, "count before deletion %lx %ld.\n",vals[j], count);*/
			uint16_t dec = rand()%100;
			/*fprintf(stdout, "removing item %lx %d.\n", vals[j], dec);*/
			qf_remove(&cf, vals[j], 0, dec);
			/*qf_dump(&cf);*/
			count = qf_count_key_value(&cf, vals[j], 0);
			if ((dec > 0 && count < 100-dec) || (dec == 0 && count > 0)) {
				fprintf(stderr, "failed lookup after removal for %lx %ld.\n", vals[j], count);
				abort();
			}
		}
	}
#endif
	/*for (uint64_t j = 0; j < nvals/5; j++) {*/
	/*if (0 < qf_count_key_value(&cf, vals[j], 0)) {*/
	/*fprintf(stderr, "failed lookup after removal for %lx.\n", vals[j]);*/
	/*abort();*/
	/*}*/
	/*}*/
	/*}*/

	/*for (uint32_t i = 0; i < nfilters; i++) {*/
	/*memset(vals, 0, nvals*sizeof(vals[0]));*/
	/*RAND_pseudo_bytes((unsigned char *)vals, sizeof(*vals) * nvals);*/
	/*for (uint32_t k = 0; k < nvals; k++)*/
	/*vals[k] = (1 * vals[k]) % cfs[i].range;*/

	/*for (uint32_t j = 0; j < nvals; j++) {*/
	/*qf_insert(&cfs[i], vals[j], 0, 1);*/
	/*uint64_t cnt = qf_count_key_value(&cfs[i], vals[j], 0);*/
	/*if (!cnt) {*/
	/*fprintf(stderr, "failed lookup while inserting for %ld.\n", vals[j]);*/
	/*abort();*/
	/*}*/
	/*}*/
	/*total_occupied_slots += cfs[i].noccupied_slots;*/
	/*}*/

	/*uint64_t rqbits = ceil(log2(total_occupied_slots + 0.05*total_occupied_slots));*/
	/*uint64_t rnslots = pow(2, rqbits);*/
	/*qf_init(&cfr, rnslots, nhashbits, 0);*/
	/*printf("Creating fianl QFs with %ld slots\n", rnslots);*/

	/*gettimeofday(&tv_insert[0], NULL);*/
	/*qf_multi_merge(qf_arr, nfilters, &cfr);*/
	/*gettimeofday(&tv_insert[1], NULL);*/
	/*printf("Merge Performance:\n");*/
	/*printf(" %f",*/
	/*0.001 * (nvals*nfilters)/(tv2msec(tv_insert[1]) - tv2msec(tv_insert[0])));*/
	/*printf(" Million merges per second\n");*/

#if 0
	valsb = (uint64_t*)calloc(nvals, sizeof(valsb[0]));
	RAND_pseudo_bytes((unsigned char *)valsb, sizeof(*valsb) * nvals/cbits);
	for (i = 0; i < nvals/cbits; i++)
		valsb[i] = (1 * valsb[i]) % cfb.range;

	valsc = (uint64_t*)calloc(nvals, sizeof(valsc[0]));
	RAND_pseudo_bytes((unsigned char *)valsc, sizeof(*valsc) * nvals/cbits);
	for (i = 0; i < nvals/cbits; i++)
		valsc[i] = (1 * valsc[i]) % cfb.range;

	fprintf(stdout, "Total number of items: %ld\n", nvals/cbits);
	for (i=0; i<nvals/cbits; i++) {
		for (j=0; j < cbits; j++) {
			qf_insert(&cfa, valsa[i], 0, freq);
			uint64_t cnt = qf_count_key_value(&cfa, valsa[i], 0);
			if (cnt < ((j+1)*freq)) {
				fprintf(stderr, "failed lookup while inserting for %ld. expexted: %d. returned: %ld\n", valsa[i], (j+1)*freq, cnt);
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
#endif

#if 0
	qf_serialize(&cfa, "tmpfile");
	qf_deserialize(&cft, "tmpfile");
	/*qf_dump(&cfa);*/
	/*qf_dump(&cft);*/

	for (i=0; i<nvals/cbits; i++) {
		if (qf_count_key_value(&cft, valsa[i], 0) < freq*cbits) {
			fprintf(stderr, "Failed lookup for 0x%lx\n", valsa[i]);
			abort();
		}
	}
	fprintf(stdout, "Serialization-Deserialization verified.\n");

#ifdef LOG_NUM_SHIFTS
	for (i=0; i<len; i++) {
		fprintf(log_shift, "%d %f\n", i, (double)((shift_count[i]/(double)nvals)*100));
	}
#endif

#endif
#if 0
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
#endif
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
#if 0
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
#endif
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

	/*fclose(fp_tmp_itr);*/
	/*fclose(log_shift);*/
	/*fclose(fp_insert);*/
	/*fclose(fp_exit_lookup);*/
	/*fclose(fp_false_lookup); */

	return 0;
}
