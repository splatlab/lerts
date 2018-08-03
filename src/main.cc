/*
 * ============================================================================
 *
 *         Author:  Prashant Pandey (), ppandey@cs.stonybrook.edu
 *   Organization:  Stony Brook University
 *
 * ============================================================================
 */

#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <exception>

#include "clipp.h"
#include "ProgOpts.h"


int popcornfilter_main (PopcornFilterOpts opts);

/* 
 * ===  FUNCTION  =============================================================
 *         Name:  main
 *  Description:  
 * ============================================================================
 */
	int
main ( int argc, char *argv[] )
{
	using namespace clipp;
	enum class mode {bm_pf, help};
	mode selected = mode::help;

	auto console = spdlog::stdout_color_mt("main_console");

	PopcornFilterOpts pfopt;
	pfopt.console = console;

	auto bm_pf_mode = (
									command("popcornfilter").set(selected, mode::bm_pf),
									required("-f", "--filters") & value("num_filters", pfopt.nfilters) %
									"number of cascade filters in the popcorn filter. (default is 1)",
									required("-q", "--qbits") & value("quotient_bits", pfopt.qbits) %
									"log of number of slots in the in-memory level in each cascade filter. (default is 16)",
									required("-l", "--levels") & value("num_levels", pfopt.nlevels) %
									"number of levels in each cascade filter. (default is 4)",
									required("-g", "--growth") & value("growth_factor", pfopt.gfactor) %
									"growth factor in each cascade filter. (default is 4)",
									required("-t", "--threads") & value("num_threads", pfopt.nthreads) %
									"number of threads (default is 1)",
									option("-a", "--age_bits") & value("num_age_bits", pfopt.nagebits) %
									"number of aging bits. (default is 0)",
									option("-o", "--no-odp").set(pfopt.do_odp, 0) %
										"do not perform on-demand popcorning. (default is true.)",
									option("-i", "--input-file") & value("input_file", pfopt.ip_file) %
									"input file containing keys and values. (default is generate keys from a Zipfian distribution.)"
									//option("-h", "--help")      % "show help"
									);
  auto cli = (
							(bm_pf_mode | command("help").set(selected,mode::help) ),
							option("-v", "--version").call([]{std::cout << "version 1.0\n\n";}).doc("show version")
							);

	assert(bm_pf_mode.flags_are_prefix_free());

	decltype(parse(argc, argv, cli)) res;
	try {
		res = parse(argc, argv, cli);
	} catch (std::exception& e) {
		std::cout << "\n\nParsing command line failed with exception: " <<
			e.what() << "\n";
		std::cout << "\n\n";
		std::cout << make_man_page(cli, "main");
		return 1;
	}

	if(res) {
		switch(selected) {
			case mode::bm_pf: popcornfilter_main(pfopt);  break;
			case mode::help: std::cout << make_man_page(cli, "main"); break;
		}
	} else {
		auto b = res.begin();
		auto e = res.end();
		if (std::distance(b,e) > 0) {
			if (b->arg() == "popcornfilter") {
				std::cout << make_man_page(bm_pf_mode, "");
			} else {
				std::cout << "There is no command \"" << b->arg() << "\"\n";
				std::cout << usage_lines(cli, "main") << '\n';
			}
		} else {
			std::cout << usage_lines(cli, "main") << '\n';
		}
	}

	return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */
