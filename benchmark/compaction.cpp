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


std::tuple<common::utils::LevelOrderedCircuit, std::vector<std::vector<common::utils::wire_t>>> generateCircuit(size_t n) {
    
    common::utils::Circuit<Ring> circ;

    /*
    Circuit idea:
    Input vectors x, y, each of size n, all values are 0/1 (but still arithmetically shared)
    Compute permutation to apply to x to order the bits, also compute ordered version of y.

    Communication:
    Setup: 2n multiplications, 2 shuffles (same perm) => 28n bytes
    Online: 2n multiplications, 2 shuffles (same perm), n reveals, 2n reveals => 36n bytes
    Rounds: 4 including output
    */

    std::vector<std::vector<common::utils::wire_t>> input_vectors;
    input_vectors.push_back(std::vector<common::utils::wire_t>(n));
    std::generate(input_vectors[0].begin(), input_vectors[0].end(),
                [&]() { return circ.newInputWire(); });
    input_vectors.push_back(std::vector<common::utils::wire_t>(n));
    std::generate(input_vectors[1].begin(), input_vectors[1].end(),
                [&]() { return circ.newInputWire(); });

    auto outputs_x = circ.addMGate(common::utils::GateType::kGenCompaction, input_vectors[0]);

    // How to sort bits:
    // - compute compaction permutation to sort
    // - shuffle the permutation and the input using the same order
    // - reveal the shuffled permutation
    // - permute shuffled input according to that
    auto perm_y = circ.addMGate(common::utils::GateType::kGenCompaction, input_vectors[1]);
    auto shuffled_perm_y = circ.addParamWithOptMGate(common::utils::GateType::kShuffle, perm_y, 0);
    auto shuffled_y = circ.addParamWithOptMGate(common::utils::GateType::kShuffle, input_vectors[1], 0);
    auto clear_shuffled_perm_y = circ.addMGate(common::utils::GateType::kReveal, shuffled_perm_y);
    auto outputs_y = circ.addMDoubleInGate(common::utils::GateType::kReorder, shuffled_y, clear_shuffled_perm_y);

    for (auto i : outputs_x) {
        circ.setAsOutput(i);
    }
    for (auto i : outputs_y) {
        circ.setAsOutput(i);
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

    int input_bits[7] = {1, 0, 0, 1, 1, 1, 0};
    std::unordered_map<common::utils::wire_t, Ring> input_to_val;
    std::unordered_map<common::utils::wire_t, int> input_to_pid;
    assert(input_vectors.size() == 2);
    for (size_t i = 0; i < 2; i++)
        assert(input_vectors[i].size() == vec_size);
    // Set first vector to input_bits || input_bits || input_bits || ...
    for (size_t i = 0; i < vec_size; i++)
        input_to_val[input_vectors[0][i]] = input_bits[i % 7];
    // Set second vector to input_bits || input_bits || input_bits || ...
    for (size_t i = 0; i < vec_size; i++)
        input_to_val[input_vectors[1][i]] = input_bits[i % 7];

    for (size_t i = 0; i < 2; i++) {
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

        size_t input_vector_sum = 0;
        for (size_t i = 0; i < vec_size; i++) {
            input_vector_sum += input_bits[i % 7];
        }
        if (pid != 0) {
            // First vec_size elements should be permutation to correctly sort input vector
            size_t next_0 = 1, next_1 = vec_size - input_vector_sum + 1; // one-indexed
            for (size_t i = 0; i < vec_size; i++) {
                if (input_bits[i % 7] == 0) {
                    assert(res[i] == next_0);
                    next_0++;
                } else {
                    assert(res[i] == next_1);
                    next_1++;
                }
            }
            // Second vector should be ordered
            size_t zeros_found = 0;
            [[maybe_unused]] bool currently_zeros = true;
            for (size_t i = 0; i < vec_size; i++) {
                if(res[vec_size + i] == 0) {
                    assert(currently_zeros);
                    zeros_found++;
                } else {
                    assert(res[vec_size + i] == 1);
                    currently_zeros = false;
                }
            }
            assert(zeros_found == vec_size - input_vector_sum);

            assert(bytes_sent == 36 * vec_size);
            assert(bytes_sent_pre == 0);
        } else {
            assert(bytes_sent == 0);
            assert(bytes_sent_pre - 56 == 28 * vec_size); // 56 always sent to synchronize vector sizes
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
      "Benchmark a simple test for compation and applying it");
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
