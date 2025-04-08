#pragma once

#include <boost/program_options.hpp>

namespace bpo = boost::program_options;

using json = nlohmann::json;

namespace bench {
    bpo::options_description programOptions();

    bpo::variables_map parseOptions(bpo::options_description& cmdline, bpo::options_description& prog_opts, int argc, char* argv[]);

    // pid, repeat, threads, network, seeds_h, seeds_l, output_data, save_output, save_file
    void setupBenchmark(const bpo::variables_map& opts, size_t& pid, size_t& repeat, size_t& threads, std::shared_ptr<io::NetIOMP>& network, uint64_t* seeds_h, uint64_t* seeds_l, bool& save_output, std::string& save_file);
}
