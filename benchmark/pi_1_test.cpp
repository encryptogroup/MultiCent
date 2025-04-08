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
            std::vector<common::utils::wire_t>, std::vector<std::vector<common::utils::wire_t>>> generateCircuit(size_t n, size_t total_size, size_t nmbr_bits, size_t depth) {
    
    common::utils::Circuit<Ring> circ;
    size_t next_free_shuffle_id = 0;

    // This inherits the communication from subcirc::pi_2 + 4n bytes online and 1 round for the output layer
    // 64t*nmbr_bits + 24n^2 + 32tnD + 8tn + 100t bytes setup
    // 48t*nmbr_bits + 48n^2 + 16tnD + 4tn +  64t + 4n bytes online 
    // 4*nmbr_bits + 3D + 13 + 1 rounds
    // vector of LSBs, then vector of bits before that, ..., vector of MSBs
    std::vector<std::vector<common::utils::wire_t>> source_bits(nmbr_bits);
    std::vector<std::vector<common::utils::wire_t>> destination_bits(nmbr_bits);
    std::vector<common::utils::wire_t> vertex_flags;
    std::vector<std::vector<common::utils::wire_t>> payload(n);

    for (size_t i = 0; i < nmbr_bits; i++) {
        for (size_t j = 0; j < total_size; j++) {
            source_bits[i].push_back(circ.newInputWire());
            destination_bits[i].push_back(circ.newInputWire());
        }
    }
    for (size_t j = 0; j < total_size; j++)  {
        vertex_flags.push_back(circ.newInputWire());
    }
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < total_size; j++)  {
            payload[i].push_back(circ.newInputWire());
        }
    }

    auto output_vector = subcirc::pi_1(source_bits, destination_bits, vertex_flags, payload, circ, next_free_shuffle_id, n, nmbr_bits, depth);

    for (size_t i = 0; i < n; i++) {
        circ.setAsOutput(output_vector[i]); // Only output data for vertices
    }

    return {circ.orderGatesByLevel(), source_bits, destination_bits, vertex_flags, payload};
}

void add_list_entry(Ring source, Ring dest, Ring vertex, std::vector<std::vector<common::utils::wire_t>> &source_bits,
                                            std::vector<std::vector<common::utils::wire_t>> &destination_bits,
                                            std::vector<common::utils::wire_t>              &vertex_flags,
                                            std::vector<std::vector<common::utils::wire_t>> &payload,
                                            std::unordered_map<common::utils::wire_t, Ring> &input_to_val, size_t &i, size_t nmbr_bits) {
    
    for (size_t j = 0; j < nmbr_bits; j++) {
        input_to_val[source_bits[j][i]] = (source >> j) & 1;
        input_to_val[destination_bits[j][i]] = (dest >> j) & 1;
    }
    assert((source >> nmbr_bits) == 0);
    assert((dest >> nmbr_bits) == 0);
    
    input_to_val[vertex_flags[i]] = vertex;
    for (size_t j = 0; j < payload.size(); j++) {
        if (vertex == 1 && source - 1 == j)
            input_to_val[payload[j][i]] = 1;
        else
            input_to_val[payload[j][i]] = 0;
    }

    i++;
}

void benchmark(const bpo::variables_map& opts) {
    
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
                                {"repeat", repeat}};
    output_data["benchmarks_pre"] = json::array();
    output_data["benchmarks"] = json::array();

    std::cout << "--- Details ---\n";
    for (const auto& [key, value] : output_data["details"].items()) {
        std::cout << key << ": " << value << "\n";
    }
    std::cout << std::endl;

    /*
    Graph instance:
    v1 - v2
    ||   ||
    v3   v4 - v5

    Which in list form is (kind of random order here for testing):
    (1,1,1)
    (2,2,1)
    (1,2,0)
    (4,5,0)
    (2,1,0)
    (1,3,0)
    (1,3,0)
    (3,1,0)
    (5,5,1)
    (4,4,1)
    (3,3,1)
    (3,1,0)
    (2,4,0)
    (4,2,0)
    (2,4,0)
    (5,4,0)
    (4,2,0)
    */

    size_t n = 5;
    size_t m = 2 * 6;
    size_t total_size = n + m;
    size_t nmbr_bits = std::ceil(std::log2(n + 2));
    size_t depth = 2;

    auto [circ, source_bits, destination_bits, vertex_flags, payload] = generateCircuit(n, total_size, nmbr_bits, depth);
    std::cout << "--- Circuit ---\n";
    std::cout << circ << std::endl;

    std::unordered_map<common::utils::wire_t, Ring> input_to_val;
    std::unordered_map<common::utils::wire_t, int> input_to_pid;

    assert(source_bits.size() == nmbr_bits);
    assert(destination_bits.size() == nmbr_bits);
    for (size_t i = 0; i < nmbr_bits; i++) {
        assert(source_bits[i].size() == total_size);
        assert(destination_bits[i].size() == total_size);
    }
    assert(vertex_flags.size() == total_size);

    size_t iter = 0;
    add_list_entry(1, 1, 1, source_bits, destination_bits, vertex_flags, payload, input_to_val, iter, nmbr_bits);
    add_list_entry(2, 2, 1, source_bits, destination_bits, vertex_flags, payload, input_to_val, iter, nmbr_bits);
    add_list_entry(1, 2, 0, source_bits, destination_bits, vertex_flags, payload, input_to_val, iter, nmbr_bits);
    add_list_entry(4, 5, 0, source_bits, destination_bits, vertex_flags, payload, input_to_val, iter, nmbr_bits);
    add_list_entry(2, 1, 0, source_bits, destination_bits, vertex_flags, payload, input_to_val, iter, nmbr_bits);
    add_list_entry(1, 3, 0, source_bits, destination_bits, vertex_flags, payload, input_to_val, iter, nmbr_bits);
    add_list_entry(1, 3, 0, source_bits, destination_bits, vertex_flags, payload, input_to_val, iter, nmbr_bits);
    add_list_entry(3, 1, 0, source_bits, destination_bits, vertex_flags, payload, input_to_val, iter, nmbr_bits);
    add_list_entry(5, 5, 1, source_bits, destination_bits, vertex_flags, payload, input_to_val, iter, nmbr_bits);
    add_list_entry(4, 4, 1, source_bits, destination_bits, vertex_flags, payload, input_to_val, iter, nmbr_bits);
    add_list_entry(3, 3, 1, source_bits, destination_bits, vertex_flags, payload, input_to_val, iter, nmbr_bits);
    add_list_entry(3, 1, 0, source_bits, destination_bits, vertex_flags, payload, input_to_val, iter, nmbr_bits);
    add_list_entry(2, 4, 0, source_bits, destination_bits, vertex_flags, payload, input_to_val, iter, nmbr_bits);
    add_list_entry(4, 2, 0, source_bits, destination_bits, vertex_flags, payload, input_to_val, iter, nmbr_bits);
    add_list_entry(2, 4, 0, source_bits, destination_bits, vertex_flags, payload, input_to_val, iter, nmbr_bits);
    add_list_entry(5, 4, 0, source_bits, destination_bits, vertex_flags, payload, input_to_val, iter, nmbr_bits);
    add_list_entry(4, 2, 0, source_bits, destination_bits, vertex_flags, payload, input_to_val, iter, nmbr_bits);

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
        for (auto inner : in)
            input_to_pid[inner] = 2;
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
            assert(res[0] == 4);
            assert(res[1] == 5);
            assert(res[2] == 3);
            assert(res[3] == 4);
            assert(res[4] == 3);

            assert(bytes_sent == 48 * total_size * nmbr_bits + 48 * n * n + 16 * total_size * n * depth + 4 * total_size * n +  64 * total_size + 4 * n);
            assert(bytes_sent_pre == 0);
        } else {
            assert(bytes_sent == 0);
            assert(bytes_sent_pre - 56 == 64 * total_size * nmbr_bits + 24 * n * n + 32 * total_size * n * depth + 8 * total_size * n + 100 * total_size); // 56 always sent to synchronize vector sizes
        }
        assert(circ.gates_by_level.size() == 4 * nmbr_bits + 3 * depth + 13 + 1);

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
      "Benchmark pi_1 on a simple graph instance for test purposes");
    cmdline.add(prog_opts);
    cmdline.add_options()(
      "config,c", bpo::value<std::string>(),
      "configuration file for easy specification of cmd line arguments")(
      "help,h", "produce help message");

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
