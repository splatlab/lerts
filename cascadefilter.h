/*
 * =====================================================================================
 *
 *       Filename:  cascadefilter.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017-10-01 11:40:01 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Prashant Pandey (), ppandey@cs.stonybrook.edu
 *   Organization:  Stony Brook University
 *
 * =====================================================================================
 */


#ifdef _CASCADEFILTER_H_
#define _CASCADEFILTER_H_


#include <sys/types.h>
#include <string>
#include <stdlib.h>
#include <stdint.h>

#include "threadsafe-gqf/gqf.h"

#define NUM_MAX_FILTERS 10;

class CascadeFilter {
	QF *filters[NUM_MAX_FILTERS];
	uint32_t thresholds[NUM_MAX_FILTERS];
	uint64_t sizes[NUM_MAX_FILTERS];
};


#endif
