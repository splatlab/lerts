#ifndef __PROG_OPTS__
#define __PROG_OPTS__

#include <memory>
#include "spdlog/spdlog.h"

class PopcornFilterOpts {
	public:
		int greedy{0};
		int pinning{0};
		int nfilters{1};
		int qbits{16};
		int nlevels{4};
		int gfactor{4};
		int nthreads{1};
		int nagebits{0};
		int do_odp = {1};
		int threshold_value{24};
		std::string ip_file;
		int port{0};
		std::shared_ptr<spdlog::logger> console{nullptr};
};


#endif //__MANTIS_PROG_OPTS__
