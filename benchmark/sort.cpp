#include <io/netmp.h>
#include <graphsc/offline_evaluator.h>
#include <graphsc/online_evaluator.h>
#include <utils/circuit.h>

#include <algorithm>
#include <boost/program_options.hpp>
#include <cmath>
#include <iostream>
#include <memory>
#include <random>

#include "utils.h"
#include "benchmark.h"
#include "subcircuits.h"

using namespace graphsc;
using json = nlohmann::json;
namespace bpo = boost::program_options;

const int USED_BITS = 32; // TODO could be decreased to improve efficiency if applicable

std::tuple<common::utils::LevelOrderedCircuit, std::vector<std::vector<common::utils::wire_t>>> generateCircuit(size_t n) {
    
    common::utils::Circuit<Ring> circ;
    size_t next_free_shuffle_id = 0;

    /*
    Circuit idea:
    Input vectors x_0, x_1, ..., x_(USED_BITS-1), x where x_0 contains the LSBs of entries in x, x_1 the next bits etc.
    while everything is shared arithmetically.
    Output x in ascending order

    Communication:
    32n*USED_BITS - 8n bytes setup
    24n*USED_BITS bytes online
    4*USED_BITS rounds
    */

    // vector of LSBs, then vector of bits before that, ..., vector of MSBs and finally vector of overall numbers (only used as payload)
    std::vector<std::vector<common::utils::wire_t>> input_vectors(USED_BITS + 1);
    std::vector<std::vector<common::utils::wire_t>> bit_vectors(USED_BITS);

    // Order of inputs shall be row by row, first bit decomposition starting at LSB and after the MSB also the arithmetic payload
    for (size_t i = 0; i < USED_BITS + 1; i++) {
        for (size_t j = 0; j < n; j++) {
            input_vectors[i].push_back(circ.newInputWire());
        }
    }
    for (size_t i = 0; i < USED_BITS; i++)
        bit_vectors[i] = input_vectors[i];

    // Compute permutation required to sort
    // 32n*USED_BITS - 28n  +  24n*USED_BITS - 16n   in 4*USED_BITS - 3 rounds
    auto sorting_perm = subcirc::genSortingPerm(bit_vectors, circ, next_free_shuffle_id, USED_BITS);
    // Apply sorting permutation to payload
    auto output_vector = subcirc::applyPerm(sorting_perm, input_vectors[USED_BITS], circ, next_free_shuffle_id); // 20n + 12n in 2 rounds

    for (auto i : output_vector) {
        circ.setAsOutput(i); // 0 + 4n in 1 round
    }

    return {circ.orderGatesByLevel(), input_vectors};
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

    auto [circ, input_vectors] = generateCircuit(vec_size);
    std::cout << "--- Circuit ---\n";
    std::cout << circ << std::endl;

    std::unordered_map<common::utils::wire_t, Ring> input_to_val;
    std::unordered_map<common::utils::wire_t, int> input_to_pid;
    assert(input_vectors.size() == USED_BITS + 1);
    for (size_t i = 0; i < USED_BITS + 1; i++)
        assert(input_vectors[i].size() == vec_size);

    // Random numbers
    std::random_device dev;
    std::mt19937 rng(dev()); // this will output different numbers per party, but only one provides input anyway
    std::uniform_int_distribution<std::mt19937::result_type> dist(0,2 << 30);

    for (size_t i = 0; i < vec_size; i++) {
        Ring x = dist(rng);
        for (size_t j = 0; j < USED_BITS; j++) {
            input_to_val[input_vectors[j][i]] = (x >> j) & 1;
        }
        input_to_val[input_vectors[USED_BITS][i]] = x;
    }

    for (size_t i = 0; i < USED_BITS + 1; i++) {
        for (auto in: input_vectors[i]) {
            input_to_pid[in] = 2;
        }
    }
    
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
            // All outputs should be in correct order
            for (size_t i = 0; i < vec_size - 1; i++) {
                assert(res[i] <= res[i + 1]);
            }

            assert(bytes_sent == 24 * vec_size * USED_BITS);
            assert(bytes_sent_pre == 0);
        } else {
            assert(bytes_sent == 0);
            assert(bytes_sent_pre - 56 == 32 * vec_size * USED_BITS - 8 * vec_size); // 56 always sent to synchronize vector sizes
        }
        assert(circ.gates_by_level.size() == 4 * USED_BITS);

        
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
      "Benchmark a simple test for sorting using radix sort");
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
