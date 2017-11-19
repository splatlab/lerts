/*
 * ============================================================================
 *
 *       Filename:  cqf.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2017-10-26 11:50:04 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Prashant Pandey (), ppandey@cs.stonybrook.edu
 *   Organization:  Stony Brook University
 *
 * ============================================================================
 */

#ifndef _CQF_H_
#define _CQF_H_

#include <iostream>
#include <cassert>
#include <unordered_set>

#include <inttypes.h>
#include <string.h>
#include <aio.h>

#include "cqf/gqf.h"

template <class key_obj>
class CQF {
	public:
		CQF();
		CQF(uint64_t nslots, uint32_t nhashbits, uint32_t nvalbits, bool mem,
				const char *filename, uint32_t seed);
		CQF(std::string& filename, bool flag);
		CQF(const CQF<key_obj>& copy_cqf);

		bool insert(const key_obj& k, enum lock flag);
		/* Will return the count. */
		uint64_t query(const key_obj& k);
		/* returns the count and value associated with the first key in the hash
		 * table. */
		uint64_t query_key(const key_obj& k, uint64_t *val);

		void remove(const key_obj& k, enum lock flag);
		void destroy();

		void serialize(std::string filename) {
			qf_serialize(&cqf, filename.c_str());
		}

		uint64_t range(void) const { return cqf.metadata->range; }
		uint32_t seed(void) const { return cqf.metadata->seed; }
		uint64_t total_elements(void) const { return cqf.metadata->nelts; }
		uint64_t distinct_elements(void) const {
			return cqf.metadata->ndistinct_elts;
		}
		uint64_t total_slots(void) const { return cqf.metadata->nslots; }
		uint64_t occupied_slots(void) const {
			return cqf.metadata->noccupied_slots;
		}
		
		//uint64_t set_size(void) const { return set.size(); }
		void reset(void) { qf_reset(&cqf); }

		void dump_metadata(void) const { DEBUG_DUMP(&cqf); }

		void drop_pages(uint64_t cur);

		class Iterator {
			public:
				Iterator(QFi it);
				key_obj operator*(void) const;
				void operator++(void);
				bool done(void) const;

			private:
				QFi iter;
		};

		Iterator begin(void) const;
		Iterator end(void) const;

	private:
		QF cqf;
		//std::unordered_set<uint64_t> set;
};

class KeyObject {
	public:
		KeyObject() : key(0), value(0), count(0), level(0) {};

		KeyObject(uint64_t k, uint64_t v, uint64_t c, uint32_t l) : key(k),
		value(v), count(c), level(l) {};

		KeyObject(const KeyObject& k) : key(k.key), value(k.value), count(k.count),
		level(k.level) {};

		bool operator==(KeyObject k) { return key == k.key; }

		uint64_t key;
		uint64_t value;
		uint64_t count;
		uint32_t level;
};

template <class key_obj>
struct compare {
	bool operator()(const key_obj& lhs, const key_obj& rhs) {
		return lhs.key > rhs.key;
	}
};

template <class key_obj>
CQF<key_obj>::CQF() {
	// dumpy filter.
	qf_init(&cqf, 1ULL << 6, 10, 0, true, "", 23423);
}

template <class key_obj>
CQF<key_obj>::CQF(uint64_t nslots, uint32_t nhashbits, uint32_t nvalbits, bool
									mem, const char *filename, uint32_t seed) {
	qf_init(&cqf, nslots, nhashbits, nvalbits, mem, filename, seed);
}

template <class key_obj>
CQF<key_obj>::CQF(std::string& filename, bool flag) {
	if (flag)
		qf_read(&cqf, filename.c_str());
	else
		qf_deserialize(&cqf, filename.c_str());
}

template <class key_obj>
CQF<key_obj>::CQF(const CQF<key_obj>& copy_cqf) {
	memcpy(cqf, copy_cqf.get_cqf(), sizeof(QF));
}

template <class key_obj>
bool CQF<key_obj>::insert(const key_obj& k, enum lock flag) {
	return qf_insert(&cqf, k.key, k.value, k.count, flag);
	// To validate the CQF
	//set.insert(k.key);
}

template <class key_obj>
uint64_t CQF<key_obj>::query(const key_obj& k) {
	return qf_count_key_value(&cqf, k.key, k.value);
}

template <class key_obj>
uint64_t CQF<key_obj>::query_key(const key_obj& k, uint64_t *val) {
	return qf_query(&cqf, k.key, val);
}

template <class key_obj>
void CQF<key_obj>::remove(const key_obj& k, enum lock flag) {
	qf_remove(&cqf, k.key, k.value, k.count, flag);
}

template <class key_obj>
void CQF<key_obj>::destroy() {
	qf_destroy(&cqf, false);
}

static void skip_keys(QFi *iter, uint32_t level_age, uint32_t max_age) {
	while (!qfi_end(iter)) {
		uint64_t key, value, count;
		qfi_get(iter, &key, &value, &count);
		uint32_t age = count & max_age;
		if (age != (level_age + 1) % max_age)
			qfi_next(iter);
		else
			break;
	}
}

template <class key_obj>
CQF<key_obj>::Iterator::Iterator(QFi it)
	: iter(it) {};

template <class key_obj>
key_obj CQF<key_obj>::Iterator::operator*(void) const {
	uint64_t key = 0, value = 0, count = 0;
	qfi_get(&iter, &key, &value, &count);
	return key_obj(key, value, count, 0);
}

template<class key_obj>
void CQF<key_obj>::Iterator::operator++(void) {
	qfi_next(&iter);
}

template<class key_obj>
bool CQF<key_obj>::Iterator::done(void) const {
	return qfi_end(&iter);
}

template<class key_obj>
typename CQF<key_obj>::Iterator CQF<key_obj>::begin(void) const {
	QFi qfi;
	qf_iterator(&this->cqf, &qfi, 0);

	return Iterator(qfi);
}

template<class key_obj>
typename CQF<key_obj>::Iterator CQF<key_obj>::end(void) const {
	QFi qfi;
	qf_iterator(&this->cqf, &qfi, 0xffffffffffffffff);
	return Iterator(qfi);
}

#endif
