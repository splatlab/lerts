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

	auto check_power_of_two = [](const int nfilters) -> bool {
		if (nfilters > 1 && ceil(log2(nfilters) != floor(log2(nfilters)))) {
			std::string e = "Number of cones should be a power of 2.";
			throw std::runtime_error{e};
		}
		return true;
	};

	auto enusure_opt_count_stretch = [](const PopcornFilterOpts pfopt) -> bool {
		if (pfopt.nagebits > 0 && (pfopt.greedy == 1  || pfopt.pinning == 1)) {
			std::string e = "Optimizations can only be specified for the count-stretch filter";
			throw std::runtime_error{e};
		}
		return true;
	};

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
									option("-v", "--threshold_value") & value("threshold_value", pfopt.threshold_value) %
									"threshold_value  to report. (default is 24)",
									option("-e", "--greedy-flushing").set(pfopt.greedy, 1) %
									"greedy flushing optimization. (default is 0)",
									option("-p", "--pinning").set(pfopt.pinning, 1) %
									"pinning optimizations. (default is 0)",
									option("-i", "--input-file") & value("input_file", pfopt.ip_file) %
									"input file containing keys and values. (default is generate keys from a Zipfian distribution.)",
									option("-u", "--udp-port") & value("udp_port", pfopt.port) %
									"port at which generator will send the packets.)"
									//option("-h", "--help")      % "show help"
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
			case mode::bm_pf:
				try {
					check_power_of_two(pfopt.nfilters);
					enusure_opt_count_stretch(pfopt);
				} catch (std::exception& e) {
					std::cout << "\n\nParsing command line failed with exception: " <<
						e.what() << "\n";
					std::cout << "\n\n";
					std::cout << make_man_page(cli, "main");
					return 1;
				}
				popcornfilter_main(pfopt);
				break;
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
