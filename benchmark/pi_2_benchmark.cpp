#include <io/netmp.h>
#include <graphsc/offline_evaluator.h>
#include <graphsc/online_evaluator.h>
#include <utils/circuit.h>

#include <algorithm>
#include <boost/program_options.hpp>
#include <cmath>
#include <iostream>
#include <memory>
#include <cmath>

#include "utils.h"
#include "benchmark.h"
#include "subcircuits.h"

using namespace graphsc;
using json = nlohmann::json;
namespace bpo = boost::program_options;

std::tuple<common::utils::LevelOrderedCircuit, std::vector<std::vector<common::utils::wire_t>>,
            std::vector<std::vector<common::utils::wire_t>>,
            std::vector<common::utils::wire_t>, std::vector<common::utils::wire_t>> generateCircuit(size_t n, size_t total_size, size_t nmbr_bits, std::vector<Ring> &weights) {
    
    common::utils::Circuit<Ring> circ;
    size_t next_free_shuffle_id = 0;

    // This inherits the communication from subcirc::pi_3 + 4n bytes online and 1 round for the output layer
    // 96t*nmbr_bits + 256t - 48 + 32*t*D bytes setup
    // 72t*nmbr_bits + 232t - 96 + 16*t*D + 4*n bytes online 
    // 8*nmbr_bits + 20 + 3 * D + 1 rounds

    // vector of LSBs, then vector of bits before that, ..., vector of MSBs
    std::vector<std::vector<common::utils::wire_t>> source_bits(nmbr_bits);
    std::vector<std::vector<common::utils::wire_t>> destination_bits(nmbr_bits);
    std::vector<common::utils::wire_t> vertex_flags;
    std::vector<common::utils::wire_t> payload;

    for (size_t i = 0; i < nmbr_bits; i++) {
        for (size_t j = 0; j < total_size; j++) {
            source_bits[i].push_back(circ.newInputWire());
            destination_bits[i].push_back(circ.newInputWire());
        }
    }
    for (size_t j = 0; j < total_size; j++)  {
        vertex_flags.push_back(circ.newInputWire());
        payload.push_back(circ.newInputWire());
    }

    auto output_vector = subcirc::pi_2(source_bits, destination_bits, vertex_flags, payload, circ, next_free_shuffle_id, n, nmbr_bits, weights);

    for (size_t i = 0; i < n; i++) {
        circ.setAsOutput(output_vector[i]); // Only output data for vertices
    }

    return {circ.orderGatesByLevel(), source_bits, destination_bits, vertex_flags, payload};
}

void add_list_entry(Ring source, Ring dest, Ring vertex, std::vector<std::vector<common::utils::wire_t>> &source_bits,
                                            std::vector<std::vector<common::utils::wire_t>> &destination_bits,
                                            std::vector<common::utils::wire_t>              &vertex_flags,
                                            std::vector<common::utils::wire_t>              &payload,
                                            std::unordered_map<common::utils::wire_t, Ring> &input_to_val, size_t &i, size_t nmbr_bits) {
    
    for (size_t j = 0; j < nmbr_bits; j++) {
        input_to_val[source_bits[j][i]] = (source >> j) & 1;
        input_to_val[destination_bits[j][i]] = (dest >> j) & 1;
    }
    assert((source >> nmbr_bits) == 0);
    assert((dest >> nmbr_bits) == 0);
    
    input_to_val[vertex_flags[i]] = vertex;
    input_to_val[payload[i]] = 0;

    i++;
}

void benchmark(const bpo::variables_map& opts) {
    
    auto n = opts["nodes"].as<size_t>();
    auto size = opts["size"].as<size_t>();
    auto depth = opts["depth"].as<size_t>();
    size_t nmbr_bits = std::ceil(std::log2(n + 2));

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
                                {"nodes", n},
                                {"size", size},
                                {"nmbr_bits", nmbr_bits},
                                {"D", depth}};
    output_data["benchmarks_pre"] = json::array();
    output_data["benchmarks"] = json::array();

    std::cout << "--- Details ---\n";
    for (const auto& [key, value] : output_data["details"].items()) {
        std::cout << key << ": " << value << "\n";
    }
    std::cout << std::endl;

    /*
    Graph instance:
    This is just for benchmarking and the protocol is oblivious up to the total size and the number
    of nodes n, so it is sufficient to set up any example of the required size.
    Just use nodes:
    (1,1,1)
    ...
    (n,n,n)
    and edges
    (1,2,0)
    (1,2,0)
    ...
    */
    assert(n >= 2);
    assert(size >= n);

    // Weights also do not matter for run time
    std::vector<Ring> weights;
    for (size_t i = 0; i < depth; i++)
        weights.push_back(1);

    auto [circ, source_bits, destination_bits, vertex_flags, payload] = generateCircuit(n, size, nmbr_bits, weights);
    std::cout << "--- Circuit ---\n";
    std::cout << circ << std::endl;

    std::unordered_map<common::utils::wire_t, Ring> input_to_val;
    std::unordered_map<common::utils::wire_t, int> input_to_pid;

    assert(source_bits.size() == nmbr_bits + 1); // one (internal) got appended for filtering duplicates
    assert(destination_bits.size() == nmbr_bits + 1); // one (internal) got appended for filtering duplicates
    for (size_t i = 0; i < nmbr_bits; i++) {
        assert(source_bits[i].size() == size);
        assert(destination_bits[i].size() == size);
    }
    assert(vertex_flags.size() == size);

    size_t iter = 0;
    for (size_t i = 0; i < n; i++)
        add_list_entry(i + 1, i + 1, 1, source_bits, destination_bits, vertex_flags, payload, input_to_val, iter, nmbr_bits);
    for (size_t i = n; i < size; i++)
        add_list_entry(1, 2, 0, source_bits, destination_bits, vertex_flags, payload, input_to_val, iter, nmbr_bits);

    for (size_t i = 0; i < nmbr_bits; i++) {
        for (auto in: source_bits[i]) {
            input_to_pid[in] = 2;
        }
        for (auto in: destination_bits[i]) {
            input_to_pid[in] = 2;
        }
    }
    for (auto in: vertex_flags) {
        input_to_pid[in] = 2;
    }
    for (auto in: payload) {
        input_to_pid[in] = 2;
    }
    
    network->sync();
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
            assert(bytes_sent == 72 * size * nmbr_bits + 232 * size - 96 + 16 * size * weights.size() + 4 * n);
            assert(bytes_sent_pre == 0);
        } else {
            assert(bytes_sent == 0);
            if (weights.size() > 0) 
                assert(bytes_sent_pre - 56 == 96 * size * nmbr_bits + 256 * size - 48 + 32 * size * weights.size()); // 56 always sent to synchronize vector sizes
            else {
                // Lazy evaluation: double-shuffles are not set up if we use no iteration, each is 8t setup, so we expect 24t setup less
                assert(bytes_sent_pre - 56 == 96 * size * nmbr_bits + 256 * size - 48 + 32 * size * weights.size() - 24 * size);
            }
        }
        assert(circ.gates_by_level.size() == 8 * nmbr_bits + 20 + 3 * weights.size() + 1);

        
        std::cout << "time: " << rbench["time"] << " ms" << std::endl;
        std::cout << "sent: " << bytes_sent << " bytes" << std::endl;

        std::cout << std::endl;
        network->sync();
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
      "Benchmark pi_2 ");
    cmdline.add(prog_opts);
    cmdline.add_options()(
      "config,c", bpo::value<std::string>(),
      "configuration file for easy specification of cmd line arguments")(
      "help,h", "produce help message")("nodes,n", bpo::value<size_t>()->required(), "Number of nodes/vertices.")
      ("size,s", bpo::value<size_t>()->required(), "Graph size, i.e., number of list entries that are nodes and edges (all layers). Note that the protocol is for directed graphs, so for comparison make sure to count each undirected edge from the reference twice")
      ("depth,d", bpo::value<size_t>()->required(), "search depth parameter D to pi_3");

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
