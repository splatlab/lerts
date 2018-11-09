// Stream analytic #1 = anomaly detection from fixed key range
// Syntax: anomaly2 options port

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <iostream>
#include <algorithm>
#include <cstring>
#include <vector>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <bitset>
#include <cassert>
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
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <vector>
#include <time.h>

#include "gqf/hashutil.h"
#include "util.h"
#include "popcornfilter.h"

// hard-coded settings from generator #1

unsigned long maxkeys = 100000;

// UDP port

int port;

// defaults for command-line switches

int perpacket = 50;
int nthresh = 24;
int mthresh = 4;
int nsenders = 1;
int countflag = 0;
int stream_size = 50000000;
char *filename = NULL;

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

	while ( (op = getopt(argc, argv, "b:t:m:p:c:s:f:")) != EOF) {
		switch (op) {
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
			case 's':
				stream_size = atoi(optarg);
				break;
			case 'f':
				filename = optarg;
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
	uint32_t seed = 2038074761;
	__uint128_t range = 1ULL << 48;
	uint64_t total = 0;
	uint64_t cnt = 0;

	read_cmd_options(argc,argv);
	// create a file and mmap it to log <keys, value> from the stream.
	uint64_t arr_size = stream_size* sizeof(uint64_t);
	uint64_t *arr = (uint64_t *)malloc(arr_size);
	if (arr == NULL) {
		std::cout << "malloc failed" << std::endl;
		exit(1)
	}
	std::cout << "malloced array of size: " << arr_size << std::end;


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

	// pre-allocate to maxkeys per generator, in agreement with generator #1

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

			key = MurmurHash64A( ((void*)&key), sizeof(key), seed);
			arr[cnt++] = key % range;
			total += value + truth;
		}
	}

	std::cout << "Received stream with " << cnt << " observations and " << total
		<< std::endl;

	uint64_t final_size = cnt*sizeof(uint64_t);
	int fd = open(filename, O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);
	if (fd < 0) {
		perror("Couldn't open file:");
		exit(1);
	}
	int ret = posix_fallocate(fd, 0, final_size);
	if (ret < 0) {
		perror("Couldn't fallocate file:\n");
		exit(EXIT_FAILURE);
	}
	uint64_t *final_arr = (uint64_t *)mmap(NULL, final_size, PROT_READ |
																				 PROT_WRITE, MAP_SHARED, fd, 0);
	if (final_arr == NULL) {
		perror("Couldn't mmap file:");
		exit(1);
	}

	memcpy(final_arr, arr, final_size);

	// unmap
	munmap(arr, arr_size);
	close(fd);

	std::cout << "Dumped " << cnt << " keys in " << filename << std::endl;


	// close UDP port and print stats
	::close(socket);
	//stats();
}
