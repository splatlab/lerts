/*
 * ============================================================================
 *
 *         Author:  Prashant Pandey (), ppandey@cs.stonybrook.edu
 *   Organization:  Stony Brook University
 *
 * ============================================================================
 */

#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <atomic>
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
#include <stdexcept>
#include <signal.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <sys/socket.h>

#include "clipp.h"
#include "ProgOpts.h"
#include "popcornfilter.h"
#include "gqf/hashutil.h"

#define BUFFER_SIZE (1ULL << 16)
#define BATCH_SIZE 1000
std::atomic<uint64_t> offset{0};

typedef struct socket_attr {
	int nsenders{1};
	uint64_t nrecv{0};
	int nshut {0};
	int countflag{0};
	int *shut;
	uint64_t *count;
	int perpacket{50};
} socket_attr;

socket_attr attr;

template <class key_object>
class ThreadArgs {
	public:
		PopcornFilter<key_object> *pf;
		uint64_t *vals;
		uint64_t stream_size;
		uint64_t batch_size;

		ThreadArgs() : pf(nullptr), vals(nullptr), stream_size(0), batch_size{0}
		{};
		ThreadArgs(PopcornFilter<key_object> *pf, uint64_t *vals, uint64_t
							 stream_size, uint64_t batch_size) : pf(pf), vals(vals),
		stream_size(stream_size), batch_size(batch_size) {};
};

template <class key_object>
class ThreadArgsSocket {
	public:
		PopcornFilter<key_object> *pf;
		int socket;

		ThreadArgsSocket() : pf(NULL), socket(0) {};
		ThreadArgsSocket(PopcornFilter<key_object> *pf, int socket) : pf(pf),
		socket(socket) {};
};

// process STOP packets
// return 1 if have received STOP packet from every sender
// else return 0 if not ready to STOP
int shutdown(char *buf)
{
	int iwhich = atoi(buf);
	if (attr.shut[iwhich]) return 0;
	attr.shut[iwhich] = 1;
	attr.nshut++;
	if (attr.nshut == attr.nsenders) return 1;
	return 0;
}

void *thread_insert_socket(void *a) {
	ThreadArgsSocket<KeyObject> *args = (ThreadArgsSocket<KeyObject>*)a;

	CQF<KeyObject> buffer_cqf(BUFFER_SIZE, args->pf->get_total_key_bits(),
														args->pf->get_num_value_bits(), QF_HASH_INVERTIBLE,
														args->pf->get_seed());

	uint32_t seed = 2038074761;
	__uint128_t range = 1ULL << 48;
	uint64_t total = 0;

	// sender stats and stop flags
	attr.shut = new int[attr.nsenders];
	attr.count = new uint64_t[attr.nsenders];
	for (int i = 0; i < attr.nsenders; i++)
		attr.count[i] = attr.shut[i] = 0;

	int maxbuf = 64*attr.perpacket;
	std::vector<char> buffer(maxbuf);
	int ipacket;

	while (true) {
		// read a packet with Nbytes
		const int nbytes = ::recv(args->socket,&buffer[0],buffer.size()-1,0);
		buffer[nbytes] = '\0';
		// check if STOP packet
		// exit if have received STOP packet from every sender
		if (nbytes < 8) {
			if (shutdown(&buffer[0]))
				break;
			continue;
		}
		attr.nrecv++;
		// tally stats on packets from each sender
		if (attr.countflag) {
			sscanf(&buffer[0],"packet %d",&ipacket);
			attr.count[ipacket % attr.nsenders]++;
		}

		// scan past header line
		strtok(&buffer[0],"\n");

		// process perpacket datums in packet
		for (int i = 0; i < attr.perpacket; i++) {
			uint64_t key = strtoul(strtok(NULL,",\n"),NULL,0);
			uint32_t value = strtoul(strtok(NULL,",\n"),NULL,0);
			uint32_t truth = strtoul(strtok(NULL,",\n"),NULL,0);

			key = MurmurHash64A( ((void*)&key), sizeof(key), seed);
			key = key % range;
			// insert in the popcorn filter.
			if (!args->pf->insert(KeyObject(key, 0, 1, 0),
														PF_TRY_ONCE_LOCK)) {
				DEBUG("Inserting in the buffer.");
				buffer_cqf.insert(KeyObject(key, 0, 1, 0), PF_NO_LOCK);
				double load_factor = buffer_cqf.occupied_slots() /
					(double)buffer_cqf.total_slots();
				if (load_factor > 0.75) {
					DEBUG("Dumping buffer.");
					typename CQF<KeyObject>::Iterator it = buffer_cqf.begin();
					do {
						KeyObject key = *it;
						if (!args->pf->insert(key, PF_WAIT_FOR_LOCK)) {
							std::cerr << "Failed insertion for " << (uint64_t)key.key <<
								std::endl;
							abort();
						}
						++it;
					} while(!it.done());
					buffer_cqf.reset();
				}
			}
			total += value + truth;
		}
	}
	/* Finally dump anything left in the buffer. */
	if (buffer_cqf.total_elts() > 0) {
		PRINT("Dumping buffer final time.");
		typename CQF<KeyObject>::Iterator it = buffer_cqf.begin();
		do {
			KeyObject key = *it;
			//PRINT("Inserting key " + key.to_string());
			if (!args->pf->insert(key, PF_WAIT_FOR_LOCK)) {
				std::cerr << "Failed insertion for " << (uint64_t)key.key << std::endl;
				abort();
			}
			++it;
		} while(!it.done());
	}

	PRINT("Total: " << total);
	// close UDP port and print stats
	::close(args->socket);

	return nullptr;
}

void *thread_insert(void *a) {
	ThreadArgs<KeyObject> *args = (ThreadArgs<KeyObject>*)a;

	CQF<KeyObject> buffer(BUFFER_SIZE, args->pf->get_total_key_bits(),
												args->pf->get_num_value_bits(), QF_HASH_INVERTIBLE,
												args->pf->get_seed());

	/* First try and insert the key-value pair in the cascade filter. If the
	 * insert fails then insert in the buffer. Later dump the buffer in the
	 * cascade filter.
	 */
	while (offset != args->stream_size) {
		uint64_t start = offset;
		if (start + args->batch_size > args->stream_size)
			args->batch_size = args->stream_size - start;
		offset += args->batch_size;
		uint64_t end = start + args->batch_size;
		DEBUG("Inserting from: " << start << " to: " << end);
		while (start < end) {
			uint64_t key = args->vals[start];
			if (!args->pf->insert(KeyObject(key, 0, 1, 0),
														PF_TRY_ONCE_LOCK)) {
				DEBUG("Inserting in the buffer.");
				buffer.insert(KeyObject(key, 0, 1, 0), PF_NO_LOCK);
				double load_factor = buffer.occupied_slots() /
					(double)buffer.total_slots();
				if (load_factor > 0.75) {
					DEBUG("Dumping buffer.");
					typename CQF<KeyObject>::Iterator it = buffer.begin();
					do {
						KeyObject key = *it;
						uint64_t count = key.count;
						key.count = 1;
						for (uint64_t c = 0; c < count; c++) {
							if (!args->pf->insert(key, PF_WAIT_FOR_LOCK)) {
								std::cerr << "Failed insertion for " << (uint64_t)key.key <<
									std::endl;
								abort();
							}
						}
						++it;
					} while(!it.done());
					buffer.reset();
				}
			}
			start++;
		}
	}
	/* Finally dump anything left in the buffer. */
	if (buffer.total_elts() > 0) {
		PRINT("Dumping buffer final time.");
		typename CQF<KeyObject>::Iterator it = buffer.begin();
		do {
			KeyObject key = *it;
			//PRINT("Inserting key " + key.to_string());
			if (!args->pf->insert(key, PF_WAIT_FOR_LOCK)) {
				std::cerr << "Failed insertion for " << (uint64_t)key.key << std::endl;
				abort();
			}
			++it;
		} while(!it.done());
	}

	return nullptr;
}

void perform_insertion(std::vector<void*> args, uint32_t nthreads, bool udp)
{
	pthread_t threads[nthreads];
	void *(*start_routine) (void *) = nullptr;
	
	if (udp)
		start_routine = &thread_insert_socket;
	else
		start_routine = &thread_insert;

	for (uint32_t i = 0; i < nthreads; i++) {
		//if (!udp) {
			//ThreadArgs<KeyObject> *a = (ThreadArgs<KeyObject>*)args[i];
			//DEBUG("Starting thread " << i << " from " << a->start << " to " <<
						//a->end);
		//}
		if (pthread_create(&threads[i], NULL, start_routine, (void*)args[i])) {
			std::cerr << "Error creating thread " << i << std::endl;
			abort();
		}
	}

	for (uint32_t i = 0; i < nthreads; i++)
		if (pthread_join(threads[i], NULL)) {
			std::cerr << "Error joining thread " << i << std::endl;
			abort();
		}
}

// random generator function:
uint64_t myrandom (uint64_t i) { return std::rand()%i;}

/* 
 * ===  FUNCTION  =============================================================
 *         Name:  main
 *  Description:  
 * ===========================================================================
 */
int popcornfilter_main (PopcornFilterOpts opts)
{
	uint64_t qbits = opts.qbits;
	uint32_t nlevels = opts.nlevels;
	uint32_t gfactor = opts.gfactor;
	uint64_t nfilters = opts.nfilters;
	uint64_t nthreads = opts.nthreads;
	uint32_t nagebits = opts.nagebits;
	uint32_t do_odp = opts.do_odp;
	uint32_t greedy = opts.greedy;
	uint32_t pinning = opts.pinning;
	uint32_t threshold_value = opts.threshold_value;

	PopcornFilter<KeyObject> pf(nfilters, qbits, nlevels, gfactor, nagebits,
															do_odp, greedy, pinning, threshold_value);

	uint64_t nvals = 0;
	uint64_t *vals = nullptr;
	int socket = 0;
	std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>> keylifetimes;

	if (opts.ip_file.size() > 1) {	// read input from a file.
		PRINT("Reading input stream and logs from disk");
		std::string streamlogfile(opts.ip_file + ".log");
		vals = popcornfilter::read_stream_from_disk(opts.ip_file, &nvals);
#ifdef VALIDATE
		keylifetimes = popcornfilter::analyze_stream(vals, nvals, threshold_value);
#endif
	} else if (opts.port > 0) {
		// setup UDP port
		socket = ::socket(PF_INET6, SOCK_DGRAM, 0);
		if (socket == -1)
			throw std::runtime_error(std::string("socket(): ") + ::strerror(errno));

		struct sockaddr_in6 address;
		::memset(&address, 0, sizeof(address));
		address.sin6_family = AF_INET6;
		address.sin6_addr = in6addr_any;
		address.sin6_port = htons(opts.port);
		if (-1 == ::bind(socket, reinterpret_cast<sockaddr*>(&address),
										 sizeof(address)))
			throw std::runtime_error(std::string("bind(): ") + ::strerror(errno));
	} else {
		nvals = pf.get_max_size();
#if 0
		// This is a specific generator to produce Jon's use-case.
		uint64_t quarter = nvals;
		uint64_t half = 2*quarter;
		nvals = nvals + nvals + 7 * (nvals/4);
		uint64_t rest = nvals - half;
		vals = (uint64_t*)calloc(nvals, sizeof(vals[0]));
		memset(vals, 0, nvals * sizeof(vals[0]));
		/* Generate random keys from a Zipfian distribution. */
		PRINT("Generating " << nvals << " random numbers.");

		RAND_bytes((unsigned char *)vals, sizeof(*vals) * (quarter));
		for (uint64_t i = 0; i < quarter; i++) {
			vals[i] = vals[i] % pf.get_range();
			vals[i + quarter] = vals[i] % pf.get_range();
		}
		RAND_bytes((unsigned char *)(vals + half), sizeof(*vals) * (rest));
		for (uint64_t i = half; i < nvals; i++)
			vals[i] = vals[i] % pf.get_range();

		//RAND_pseudo_bytes((unsigned char *)vals, sizeof(*vals) * (nvals));
		//for (uint64_t i = 0; i < nvals; i++)
		//vals[i] = vals[i] % pf.get_range();
#endif
		vals = (uint64_t*)calloc(nvals, sizeof(vals[0]));
		memset(vals, 0, nvals * sizeof(vals[0]));
		/* Generate random keys from a Zipfian distribution. */
		PRINT("Generating " << nvals << " random numbers.");
		generate_random_keys(vals, nvals, nvals, 1.5);
		//std::random_shuffle(&vals[0], &vals[nvals-1], myrandom);
#ifdef VALIDATE
		keylifetimes = popcornfilter::analyze_stream(vals, nvals, threshold_value);
		//popcornfilter::induce_special_case(vals, threshold_value, 29000, 40000, 5);
		//keylifetimes = popcornfilter::analyze_stream(vals, nvals, threshold_value);
#endif
	}
	PRINT("Total observations: " << nvals);

	struct timeval start, end;
	struct timezone tzp;
	std::vector<void*> args;
	args.reserve(nthreads);

	if (opts.port > 0) {
		for (uint64_t i = 0; i < nthreads; i++) {
			ThreadArgsSocket<KeyObject> *obj = new ThreadArgsSocket<KeyObject>(&pf, socket);
			args.emplace_back((void*)obj);
		}
	} else {
		for (uint64_t i = 0; i < nthreads; i++) {
			uint64_t batch_size;
			if (BATCH_SIZE * nthreads > nvals) {
				batch_size = nvals/nthreads;
			} else {
				batch_size = BATCH_SIZE;
			}
			ThreadArgs<KeyObject> *obj = new ThreadArgs<KeyObject>(&pf, vals,
																														 nvals,
																														 batch_size);
			args.emplace_back((void*)obj);
		}
	}

	PRINT("Inserting elements.");
	gettimeofday(&start, &tzp);
	perform_insertion(args, nthreads, opts.port > 0 ? 1 : 0);
	gettimeofday(&end, &tzp);
	popcornfilter::print_time_elapsed("", &start, &end);
	PRINT("Finished insertions.");
	PRINT("Insertion throughput: " << nvals/(float)popcornfilter::cal_time_elapsed(&start, &end));

	//PRINT("Total distinct elements inserted: " <<
				//pf.get_total_dist_elements());

#ifdef VALIDATE
	PRINT("Querying elements.");
	gettimeofday(&start, &tzp);
	for (uint64_t k = 0; k < nvals; k++) {
		uint64_t key = vals[k];
		if (pf.query(KeyObject(key, 0, 0, 0), PF_WAIT_FOR_LOCK) < 1) {
			std::cerr << "Failed lookup for " <<
				(uint64_t)vals[k] << " index: " << k << std::endl;
			abort();
		}
	}
	gettimeofday(&end, &tzp);
	popcornfilter::print_time_elapsed("", &start, &end);
	PRINT("Finished lookups.");
#endif

#ifdef VALIDATE
	if (nthreads == 1) {
		PRINT("Performing validation");
		if (pf.validate_anomalies(keylifetimes, vals))
			PRINT("Validation successful!");
		else
			PRINT("Validation failed!");
	}
#else
	if (!opts.do_odp)
		pf.find_anomalies();
#endif

	PRINT("Total number of keys above threshold: " <<
				pf.get_total_keys_above_threshold());

#ifdef VALIDATE
	pf.print_stats();
#endif

	return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */
