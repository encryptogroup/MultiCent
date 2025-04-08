#include <io/netmp.h>
#include <graphsc/offline_evaluator.h>
#include <graphsc/online_evaluator.h>
#include <utils/circuit.h>

#include <algorithm>
#include <boost/program_options.hpp>
#include <cmath>
#include <iostream>
#include <memory>

#include "utils.h"
#include "benchmark.h"

using namespace graphsc;
using json = nlohmann::json;
namespace bpo = boost::program_options;


std::tuple<common::utils::LevelOrderedCircuit, std::vector<common::utils::wire_t>> generateCircuit(size_t n) {
    
    common::utils::Circuit<Ring> circ;

    /*
    Circuit idea:
    Input vectors x of size n
    Apply on x permutation pi^(-1), then rho and then doubleshuffle for rho and pi

    Communication:
    Setup: P0 sends 8n bytes to set up 2 partial permutations, 8n bytes for two standard permutations and 24n bytes for the masks
    Online: P1/P2 each send 12n bytes + 4n to reveal outputs => 16n
    4 rounds including output stage
    */

    std::vector<common::utils::wire_t> input(n);
    std::generate(input.begin(), input.end(),
                [&]() { return circ.newInputWire(); });

    auto a = circ.addParamWithOptMGate(common::utils::GateType::kShuffle, input, 0, true);
    auto b = circ.addParamWithOptMGate(common::utils::GateType::kShuffle, a, 1, false);
    auto c = circ.addThreeParamMGate(common::utils::GateType::kDoubleShuffle, b, 2, 1, 0);

    for (auto i : c) {
        circ.setAsOutput(i);
    }
    
    return {circ.orderGatesByLevel(), input};
}


void benchmark(const bpo::variables_map& opts) {

    auto vec_size = opts["vec-size"].as<size_t>();
    
    size_t pid, repeat, threads;
    std::shared_ptr<io::NetIOMP> network = nullptr;
    uint64_t seeds_h[5];
    uint64_t seeds_l[5];
    json output_data;
    bool save_output;
    std::string save_file;
    bench::setupBenchmark(opts, pid, repeat, threads, network, seeds_h, seeds_l, save_output, save_file);
    output_data["details"] = {{"pid", pid},
                                {"threads", threads},
                                {"seeds_h", seeds_h},
                                {"seeds_l", seeds_l},
                                {"repeat", repeat},
                                {"vec-size", vec_size}};
    output_data["benchmarks_pre"] = json::array();
    output_data["benchmarks"] = json::array();

    std::cout << "--- Details ---\n";
    for (const auto& [key, value] : output_data["details"].items()) {
        std::cout << key << ": " << value << "\n";
    }
    std::cout << std::endl;

    auto [circ, input] = generateCircuit(vec_size);
    std::cout << "--- Circuit ---\n";
    std::cout << circ << std::endl;

    std::unordered_map<common::utils::wire_t, Ring> input_to_val;
    std::unordered_map<common::utils::wire_t, int> input_to_pid;
    assert(input.size() == vec_size);
    // Set input to 0,1,...
    for (size_t i = 0; i < vec_size; i++)
        input_to_val[input[i]] = i;

    for (auto in: input)
        input_to_pid[in] = 2;

    for (size_t r = 0; r < repeat; ++r) {
        std::cout << "--- Repetition " << r + 1 << " ---" << std::endl;

        OfflineEvaluator off_eval(pid, network, circ, threads, seeds_h, seeds_l);
        StatsPoint start_pre(*network);
        auto preproc = off_eval.run(input_to_pid);
        StatsPoint end_pre(*network);
        network->sync();

        auto rbench_pre = end_pre - start_pre;
        output_data["benchmarks_pre"].push_back(rbench_pre);
        size_t bytes_sent_pre = 0;
        for (const auto& val : rbench_pre["communication"]) {
            bytes_sent_pre += val.get<int64_t>();
        }
        std::cout << "setup time: " << rbench_pre["time"] << " ms" << std::endl;
        std::cout << "setup sent: " << bytes_sent_pre << " bytes" << std::endl;
        
        OnlineEvaluator eval(pid, network, std::move(preproc), circ, 
                    threads, seeds_h, seeds_l);
        StatsPoint start(*network);
        auto res = eval.evaluateCircuit(input_to_val);
        StatsPoint end(*network);
        auto rbench = end - start;
        output_data["benchmarks"].push_back(rbench);

        size_t bytes_sent = 0;
        for (const auto& val : rbench["communication"]) {
            bytes_sent += val.get<int64_t>();
        }

        if (pid != 0) {
            // Output should be 0,1,...
            for (size_t i = 0; i < vec_size; i++)
                assert(res[i] == i);

            assert(bytes_sent == 16 * vec_size);
            assert(bytes_sent_pre == 0);
        } else {
            assert(bytes_sent == 0);
            assert(bytes_sent_pre - 56 == 40 * vec_size); // 56 always sent to synchronize vector sizes
        }
        assert(circ.gates_by_level.size() == 4);

        
        std::cout << "time: " << rbench["time"] << " ms" << std::endl;
        std::cout << "sent: " << bytes_sent << " bytes" << std::endl;

        std::cout << std::endl;
    }
    output_data["stats"] = {{"peak_virtual_memory", peakVirtualMemory()},
                            {"peak_resident_set_size", peakResidentSetSize()}};

    std::cout << "--- Overall Statistics ---\n";
    for (const auto& [key, value] : output_data["stats"].items()) {
        std::cout << key << ": " << value << "\n";
    }
    std::cout << std::endl;

    if (save_output) {
        saveJson(output_data, save_file);
    }
}

int main(int argc, char* argv[]) {
    auto prog_opts(bench::programOptions());
    bpo::options_description cmdline(
      "Benchmark a simple test for the double shuffling");
    cmdline.add(prog_opts);
    cmdline.add_options()(
      "config,c", bpo::value<std::string>(),
      "configuration file for easy specification of cmd line arguments")(
      "help,h", "produce help message") ("vec-size,v", bpo::value<size_t>()->required(), "Number of vector elements.");

    bpo::variables_map opts = bench::parseOptions(cmdline, prog_opts, argc, argv);
    if (opts.count("pid") == 0) {
        return 0; // Help page etc.
    }

    try {
        benchmark(opts);
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}
