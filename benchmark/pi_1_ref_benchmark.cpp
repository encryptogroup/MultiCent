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

std::tuple<common::utils::LevelOrderedCircuit, std::vector<std::vector<std::vector<common::utils::wire_t>>>> generateCircuit(size_t n, size_t l, size_t depth) {
    
    common::utils::Circuit<Ring> circ;

    // This inherits the communication from subcirc::pi_3_ref + 4n bytes online and 1 round for the output layer
    // 4n^3*(D-1)+24n^2 bytes setup
    // 8n^3*(D-1)+48n^2 bytes online
    // D + 5 rounds

    std::vector<std::vector<std::vector<common::utils::wire_t>>> adj_matrices(l);

    for (size_t k = 0; k < l; k++) {
        for (size_t i = 0; i < n; i++) {
            adj_matrices[k].push_back(std::vector<common::utils::wire_t>());
            for (size_t j = 0; j < n; j++) {
                adj_matrices[k][i].push_back(circ.newInputWire());
            }
        }
    }

    auto output_vector = subcirc::pi_1_reference(adj_matrices, circ, n, depth);

    for (auto out : output_vector) {
        circ.setAsOutput(out);
    }

    return {circ.orderGatesByLevel(), adj_matrices};
}

void benchmark(const bpo::variables_map& opts) {

    auto n = opts["nodes"].as<size_t>();
    auto l = opts["layers"].as<size_t>();
    auto depth = opts["depth"].as<size_t>();
    if (depth == 0)
        throw std::runtime_error("D=0 (depth zero) not allowed for reference implementations");
    
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
                                {"layers", l},
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
    Just generate l many nxn zero matrices as the protocol is oblivious regarding the matrix content.
    */

    auto [circ, adj_matrices] = generateCircuit(n, l, depth);
    std::cout << "--- Circuit ---\n";
    std::cout << circ << std::endl;

    std::unordered_map<common::utils::wire_t, Ring> input_to_val;
    std::unordered_map<common::utils::wire_t, int> input_to_pid;

    assert(adj_matrices.size() == l);
    for (size_t i = 0; i < l; i++) {
        assert(adj_matrices[i].size() == n);
        assert(adj_matrices[i][0].size() == n);
    }

    for (size_t k = 0; k < l; k++) 
        for (size_t i = 0; i < n; i++)
            for (size_t j = 0; j < n; j++) {
                input_to_val[adj_matrices[k][i][j]] = 0;
                input_to_pid[adj_matrices[k][i][j]] = 2;
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
            assert(bytes_sent == 8*n*n*n*(depth-1) + 48*n*n + 4*n);
            assert(bytes_sent_pre == 0);
        } else {
            assert(bytes_sent == 0);
            assert(bytes_sent_pre - 56 == 4*n*n*n*(depth-1) + 24*n*n); // 56 always sent to synchronize vector sizes
        }
        assert(circ.gates_by_level.size() == depth + 6);

        
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
      "Benchmark reference pi_1 based on matrix multiplication");
    cmdline.add(prog_opts);
    cmdline.add_options()(
      "config,c", bpo::value<std::string>(),
      "configuration file for easy specification of cmd line arguments")(
      "help,h", "produce help message")("nodes,n", bpo::value<size_t>()->required(), "Number of nodes/vertices.")
      ("layers,l", bpo::value<size_t>()->required(), "Number of layers")
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
