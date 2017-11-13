// Stream analytic #1 = anomaly detection from fixed key range
// Syntax: anomaly2 options port

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <fstream>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>

#include <stdexcept>
#include <signal.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <vector>
#include <tr1/unordered_map>
#include <time.h>

#include "popcornfilter.h"

// hard-coded settings from generator #1

unsigned long maxkeys = 100000;

// UDP port

int port;

// defaults for command-line switches

int size = 26;
int perpacket = 50;
int nthresh = 24;
int mthresh = 4;
int nsenders = 1;
int countflag = 0;

// packet stats

uint64_t nrecv = 0;
int nshut = 0;
int *shut;
uint64_t *count;

// anomaly stats

int ptrue = 0;
int pfalse = 0;
int nfalse = 0;
int ntrue = 0;

// parse command line
// regular argument = port to read from
// optional arguments:
//   -b = bundling factor = # of datums in one packet
//   -t = check key for anomalous when appears this many times
//   -m = key is anomalous if value=1 <= this many times
//   -p = # of generators sending to me, used to pre-allocate hash table
//   -c = 1 to print stats on packets received from each sender

void read_cmd_options(int argc, char **argv)
{
	int op;

	while ( (op = getopt(argc, argv, "s:b:t:m:p:c:")) != EOF) {
		switch (op) {
			case 's':
				size = atoi(optarg);
				break;
			case 'b':
				perpacket = atoi(optarg);
				break;
			case 't':
				nthresh = atoi(optarg);
				break;
			case 'm':
				mthresh = atoi(optarg);
				break;
			case 'p':
				nsenders = atoi(optarg);
				break;
			case 'c':
				countflag = 1;
				break;
		}
	}

	if (optind != argc-1) {
		fprintf(stderr,"Syntax: anomaly1 options port\n");
		exit(1);
	}

	int flag = 0;
	if (perpacket <= 0) flag = 1;
	if (nthresh <= 0 || mthresh <= 0) flag = 1;
	if (nsenders <= 0) flag = 1;
	if (flag) {
		fprintf(stderr, "ERROR: invalid command-line switch\n");
		exit(1);
	}

	// set UDP port

	port = atoi(argv[optind]);
}

// process STOP packets
// return 1 if have received STOP packet from every sender
// else return 0 if not ready to STOP

int shutdown(char *buf)
{
	int iwhich = atoi(buf);
	if (shut[iwhich]) return 0;
	shut[iwhich] = 1;
	nshut++;
	if (nshut == nsenders) return 1;
	return 0;
}

// stats on packets and keys received, and anomalies found

void stats()
{
	printf("\n");
	printf("packets received = %" PRIu64 "\n",nrecv);
	if (countflag && nsenders > 1)
		for (int i = 0; i < nsenders; i++)
			printf("packets received from sender %d = %" PRIu64 "\n",i,count[i]);

	uint64_t ndatum = nrecv * perpacket;
	fprintf(stdout,"datums received = %" PRIu64 "\n",ndatum);

	printf("true anomalies = %d\n",ptrue);
	printf("false positives = %d\n",pfalse);
	printf("false negatives = %d\n",nfalse);
	printf("true negatives = %d\n",ntrue);
}

// main program

int main(int argc, char **argv)
{
	uint32_t seed = time(NULL);

	FILE *fp_tmp_itr = fopen("tmp_itr.txt", "w");
	if (fp_tmp_itr == NULL) {
		printf("Can't open the data file");
		exit(1);
	}

	read_cmd_options(argc,argv);

	PopcornFilter<KeyObject> pf(4, 20, 4, 4);

	// sender stats and stop flags

	count = new uint64_t[nsenders];
	shut = new int[nsenders];
	for (int i = 0; i < nsenders; i++) count[i] = shut[i] = 0;

	// setup UDP port

	const int socket = ::socket(PF_INET6, SOCK_DGRAM, 0);
	if (socket == -1)
		throw std::runtime_error(std::string("socket(): ") + ::strerror(errno));

	struct sockaddr_in6 address;
	::memset(&address, 0, sizeof(address));
	address.sin6_family = AF_INET6;
	address.sin6_addr = in6addr_any;
	address.sin6_port = htons(port);
	if (-1 == ::bind(socket, reinterpret_cast<sockaddr*>(&address),
									 sizeof(address)))
		throw std::runtime_error(std::string("bind(): ") + ::strerror(errno));

	// packet buffer length of 64 bytes per datum is ample

	int maxbuf = 64*perpacket;
	std::vector<char> buffer(maxbuf);
	int ipacket;

	// loop on reading packets

	while (true) {

		// read a packet with Nbytes

		const int nbytes = ::recv(socket,&buffer[0],buffer.size()-1,0);
		buffer[nbytes] = '\0';

		// check if STOP packet
		// exit if have received STOP packet from every sender

		if (nbytes < 8) {
			if (shutdown(&buffer[0])) break;
			continue;
		}

		nrecv++;

		// tally stats on packets from each sender

		if (countflag) {
			sscanf(&buffer[0],"packet %d",&ipacket);
			count[ipacket % nsenders]++;
		}

		// scan past header line

		strtok(&buffer[0],"\n");

		// process perpacket datums in packet

		for (int i = 0; i < perpacket; i++) {
			uint64_t key = strtoul(strtok(NULL,",\n"),NULL,0);
			uint32_t value = strtoul(strtok(NULL,",\n"),NULL,0);
			uint32_t truth = strtoul(strtok(NULL,",\n"),NULL,0);

			// Insert the concatenation of key and value in the QF
			// then query the count of <key+0> and <key+1>
			// If the sum of these two counts exceeds the nthreash
			// then check for the anomaly threshhold against the count <key+1>

			key = HashUtil::MurmurHash64A( ((void*)&key), sizeof(key), seed);
			key = ((key << 1) | value) % pf.get_range();

			pf.insert(KeyObject(key, 0, 1, 0), LOCK_AND_SPIN);

#if 0
			int cnt0, cnt1;
			cnt0 = qf_count_key_value(&cf, key0, 0);
			cnt1 = qf_count_key_value(&cf, key1, 0);

			if (value) {
				cnt1++;
			} else {
				cnt0++;
			}

			if (cnt0 + cnt1 == nthresh) {
				if (cnt1 > mthresh) {
					if (truth) {
						nfalse++;
						//	printf("false negative = %" PRIu64 "\n",(uint64_t)key);
					} else ntrue++;
				} else {
					if (truth) {
						ptrue++;
						//	printf("true anomaly = %" PRIu64 "\n",(uint64_t)key);
					} else {
						pfalse++;
						//	printf("false positive = %" PRIu64 "\n",(uint64_t)key);
					}
				}
				if (value) {
					qf_insert(&cf, key1, 0, 1);
				} else {
					qf_insert(&cf, key0, 0, 1);
				}
			} else if(cnt0 + cnt1 < nthresh) {
				if (value) {
					qf_insert(&cf, key1, 0, 1);
				} else {
					qf_insert(&cf, key0, 0, 1);
				}
			}
#endif
		}
	}

	PRINT_CF("Total elements inserted: " << pf.get_total_elements());
	PRINT_CF("Total distinct elements inserted: " <<
					 pf.get_total_dist_elements());

	::close(socket);
	//stats();
}