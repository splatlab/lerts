/*
 * =====================================================================================
 *
 *       Filename:  qf_wrapper.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/28/2015 04:48:55 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Prashant Pandey (ppandey@cs.stonybrook.edu), 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef GQF_WRAPPER_H
#define GQF_WRAPPER_H

#include "gqf.h"

extern inline int gqf_init(QF *qf, uint64_t nbits, uint64_t num_hash_bits)
{
	uint64_t nslots = 1 << nbits;
	qf_init(qf, nslots, num_hash_bits, 0);
	return 0;
}

extern inline bool gqf_insert(QF *qf, __uint128_t val, uint64_t count, bool lock, bool spin)
{
	return qf_insert(qf, val, 0, count, lock, spin);
}

extern inline int gqf_lookup(QF *qf, __uint128_t val)
{
	return qf_count_key_value(qf, val, 0);
}

extern inline __uint128_t gqf_range(QF *qf)
{
	return qf->range;
}

extern inline int gqf_destroy(QF *qf)
{
	qf_destroy(qf);
	return 0;
}

extern inline int gqf_reset(QF *qf)
{
	qf_reset(qf);
	return 0;
}

extern inline int gqf_iterator(QF *qf, QFi *qfi, uint64_t pos)
{
	qf_iterator(qf, qfi, pos);
	return 0;
}

/* Returns 0 if the iterator is still valid (i.e. has not reached the
 * end of the QF. */
extern inline int gqf_get(QFi *qfi, uint64_t *key, uint64_t *value, uint64_t *count)
{
	return qfi_get(qfi, key, value, count);
}

/* Advance to next entry.  Returns whether or not another entry is
 * found.  */
extern inline int gqf_next(QFi *qfi)
{
	return qfi_next(qfi);
}

/* Check to see if the if the end of the QF */
extern inline int gqf_end(QFi *qfi)
{
	return qfi_end(qfi);
}

extern inline int gqf_get_stats(QF *qf, uint64_t *distinct_elts, uint64_t *total_elts)
{
	*distinct_elts = qf->ndistinct_elts;
	*total_elts = qf->nelts;
	return 0;
}
#endif
