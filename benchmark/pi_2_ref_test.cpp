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

std::tuple<common::utils::LevelOrderedCircuit, std::vector<std::vector<std::vector<common::utils::wire_t>>>> generateCircuit(size_t n, size_t l, std::vector<Ring> &weights) {
    
    common::utils::Circuit<Ring> circ;

    // This inherits the communication from subcirc::pi_3_ref + 4n bytes online and 1 round for the output layer
    // 4*n^2*(D+l-2) bytes setup
    // 8*n^2*(D+l-2) + 4*n bytes online
    // D + ceil(log2(l)) - 1 + 1 rounds

    std::vector<std::vector<std::vector<common::utils::wire_t>>> adj_matrices(l);

    for (size_t k = 0; k < l; k++) {
        for (size_t i = 0; i < n; i++) {
            adj_matrices[k].push_back(std::vector<common::utils::wire_t>());
            for (size_t j = 0; j < n; j++) {
                adj_matrices[k][i].push_back(circ.newInputWire());
            }
        }
    }

    auto output_vector = subcirc::pi_2_reference(adj_matrices, circ, n, weights);

    for (auto out : output_vector) {
        circ.setAsOutput(out);
    }

    return {circ.orderGatesByLevel(), adj_matrices};
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
    || / ||
    v3   v4

    Which we split into the following three adjacency matrices:
    0 1 1 0   0 0 1 0   0 0 0 0
    1 0 1 1   0 0 0 0   0 0 0 1
    1 1 0 0   1 0 0 0   0 0 0 0
    0 1 0 0   0 0 0 0   0 1 0 0
    */

    size_t n = 4;
    size_t l = 3;
    std::vector<Ring> weights = {10000000, 100000, 1000, 1};
    [[maybe_unused]] size_t log_l = std::ceil(std::log2(l));

    auto [circ, adj_matrices] = generateCircuit(n, l, weights);
    std::cout << "--- Circuit ---\n";
    std::cout << circ << std::endl;

    std::unordered_map<common::utils::wire_t, Ring> input_to_val;
    std::unordered_map<common::utils::wire_t, int> input_to_pid;

    assert(adj_matrices.size() == l);
    for (size_t i = 0; i < l; i++) {
        assert(adj_matrices[i].size() == n);
        assert(adj_matrices[i][0].size() == n);
    }

    input_to_val[adj_matrices[0][0][0]] = 0;
    input_to_val[adj_matrices[0][0][1]] = 1;
    input_to_val[adj_matrices[0][0][2]] = 1;
    input_to_val[adj_matrices[0][0][3]] = 0;
    input_to_val[adj_matrices[0][1][0]] = 1;
    input_to_val[adj_matrices[0][1][1]] = 0;
    input_to_val[adj_matrices[0][1][2]] = 1;
    input_to_val[adj_matrices[0][1][3]] = 1;
    input_to_val[adj_matrices[0][2][0]] = 1;
    input_to_val[adj_matrices[0][2][1]] = 1;
    input_to_val[adj_matrices[0][2][2]] = 0;
    input_to_val[adj_matrices[0][2][3]] = 0;
    input_to_val[adj_matrices[0][3][0]] = 0;
    input_to_val[adj_matrices[0][3][1]] = 1;
    input_to_val[adj_matrices[0][3][2]] = 0;
    input_to_val[adj_matrices[0][3][3]] = 0;

    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            if ((i == 0 && j == 2) || (i == 2 && j == 0)) {
                input_to_val[adj_matrices[1][i][j]] = 1;
            } else {
                input_to_val[adj_matrices[1][i][j]] = 0;
            }
        }
    }
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            if ((i == 1 && j == 3) || (i == 3 && j == 1)) {
                input_to_val[adj_matrices[2][i][j]] = 1;
            } else {
                input_to_val[adj_matrices[2][i][j]] = 0;
            }
        }
    }

    for (size_t k = 0; k < l; k++) 
        for (size_t i = 0; i < n; i++)
            for (size_t j = 0; j < n; j++)
                input_to_pid[adj_matrices[k][i][j]] = 2;
    
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
            assert(res[0] == 20510023); // 2 of length 1, 5 of length 2, 10 of length 3, 23 of length 4
            assert(res[1] == 30513025); // 3 of length 1, 5 of length 2, 13 of length 3, 25 of length 4
            assert(res[2] == 20510023); // 2 of length 1, 5 of length 2, 10 of length 3, 23 of length 4
            assert(res[3] == 10305013); // 1 of length 1, 3 of length 2,  5 of length 3, 13 of length 4

            assert(bytes_sent == 8*n*n*(weights.size() + l - 2) + 4*n);
            assert(bytes_sent_pre == 0);
        } else {
            assert(bytes_sent == 0);
            assert(bytes_sent_pre - 56 == 4*n*n*(weights.size() + l - 2)); // 56 always sent to synchronize vector sizes
        }
        assert(circ.gates_by_level.size() == weights.size() + log_l);

        
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
      "Benchmark reference pi_3 based on matrix multiplication on a simple graph instance for test purposes");
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
