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


std::tuple<common::utils::LevelOrderedCircuit, std::vector<common::utils::wire_t>> generateCircuit() {
    common::utils::Circuit<Ring> circ;

    /*
    Circuit idea:
    Arithmetic inputs a, b, c, d, e
    Just compute EQZ on all of them, i.e., output arithmetic 1 if and only if value is 0.

    Each EQZ layer has the cost of an AND and B2A has the cost of a MULT
    =>
    120 bytes offline, 240 + 20 bytes online, 6 + 1 rounds
    */

    std::vector<common::utils::wire_t> inputs(5);
    for (size_t i = 0; i < 5; i++) {
        inputs[i] = circ.newInputWire();
    }

    std::vector<common::utils::wire_t> outputs(5);
    for (size_t i = 0; i < 5; i++) {
        auto level1 = circ.addParamGate(common::utils::GateType::kEqualsZero, inputs[i], 0);
        auto level2 = circ.addParamGate(common::utils::GateType::kEqualsZero, level1, 1);
        auto level3 = circ.addParamGate(common::utils::GateType::kEqualsZero, level2, 2);
        auto level4 = circ.addParamGate(common::utils::GateType::kEqualsZero, level3, 3);
        auto level5 = circ.addParamGate(common::utils::GateType::kEqualsZero, level4, 4);
        outputs[i] = circ.addGate(common::utils::GateType::kConvertB2A, level5);
    }

    for (size_t i = 0; i < 5; i++) 
        circ.setAsOutput(outputs[i]);

    return {circ.orderGatesByLevel(), inputs};
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

    auto [circ, inputs] = generateCircuit();
    std::cout << "--- Circuit ---\n";
    std::cout << circ << std::endl;

    std::unordered_map<common::utils::wire_t, Ring> input_to_val;
    std::unordered_map<common::utils::wire_t, int> input_to_pid;
    assert(inputs.size() == 5);
    input_to_val[inputs[0]] = -1;
    input_to_val[inputs[1]] = 0;
    input_to_val[inputs[2]] = 1;
    input_to_val[inputs[3]] = 2;
    input_to_val[inputs[4]] = 811;
    for (auto in: inputs) {
        input_to_pid[in] = 2;
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
            assert(res[0] == 0);
            assert(res[1] == 1);
            assert(res[2] == 0);
            assert(res[3] == 0);
            assert(res[4] == 0);

            assert(bytes_sent == 260);
            assert(bytes_sent_pre == 0);
        } else {
            assert(bytes_sent == 0);
            assert(bytes_sent_pre - 56 == 120); // 56 always sent to synchronize vector sizes
        }
        assert(circ.gates_by_level.size() == 7);

        
        std::cout << "time: " << rbench["time"] << " ms" << std::endl;
        std::cout << "sent: " << bytes_sent << " bytes" << std::endl;

        std::cout << std::endl;
    }
    output_data["stats"] = {{"peak_virtual_memory", peakVirtualMemory()},
                            {"peak_resident_set_size", peakResidentSetSize()}};

    std::cout << "--- Statistics ---\n";
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
      "Benchmark a test for EQZ checks");
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
