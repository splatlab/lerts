#ifndef __PROG_OPTS__
#define __PROG_OPTS__

#include <memory>
#include "spdlog/spdlog.h"

class PopcornFilterOpts {
	public:
		int nfilters{1};
		int qbits{16};
		int nlevels{4};
		int gfactor{4};
		int nthreads{1};
		int nagebits{0};
		int do_odp = {1};
		std::string ip_file;
		std::shared_ptr<spdlog::logger> console{nullptr};
};


#endif //__MANTIS_PROG_OPTS__
