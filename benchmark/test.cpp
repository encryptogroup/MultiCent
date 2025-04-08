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
    Arithmetic inputs a,b,c,d, binary inputs e,f
    Compute (a * b) * (c + d), e & f, e ^ f, c * (a + b)

    This should take 16 bytes sent by P0 and 48 bytes sent by P1 and P2 each in 3 rounds, including the output stage.
    */

    std::vector<common::utils::wire_t> inputs(6);
    for(size_t i = 0; i < 4; i++) {
        inputs[i] = circ.newInputWire();
    }
    for(size_t i = 4; i < 6; i++) {
        inputs[i] = circ.newBinInputWire();
    }

    std::vector<common::utils::wire_t> intermediate(3);
    intermediate[0] = circ.addGate(common::utils::GateType::kMul, inputs[0], inputs[1]);
    intermediate[1] = circ.addGate(common::utils::GateType::kAdd, inputs[2], inputs[3]);
    intermediate[2] = circ.addGate(common::utils::GateType::kAdd, inputs[0], inputs[1]);

    std::vector<common::utils::wire_t> outputs(4);
    outputs[0] = circ.addGate(common::utils::GateType::kMul, intermediate[0], intermediate[1]);
    outputs[1] = circ.addGate(common::utils::GateType::kAnd, inputs[4], inputs[5]);
    outputs[2] = circ.addGate(common::utils::GateType::kXor, inputs[4], inputs[5]);
    outputs[3] = circ.addGate(common::utils::GateType::kMul, inputs[2], intermediate[2]);


    circ.setAsOutput(outputs[0]);
    circ.setAsBinOutput(outputs[1]);
    circ.setAsBinOutput(outputs[2]);
    circ.setAsOutput(outputs[3]);

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
    assert(inputs.size() == 6);
    input_to_val[inputs[0]] = 5;
    input_to_val[inputs[1]] = 3;
    input_to_val[inputs[2]] = 8;
    input_to_val[inputs[3]] = 11;
    input_to_val[inputs[4]] = 0x00FF00F1;
    input_to_val[inputs[5]] = 0xFF1F0010;
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
            assert(res[0] == 285); // (5 * 3) * (8 + 11)
            assert(res[1] == 0x001F0010); // 0x00FF00F1 & 0xFF1F0010
            assert(res[2] == 0xFFE000E1); // 0x00FF00F1 ^ 0xFF1F0010
            assert(res[3] == 64); // 8 * (5 + 3)

            assert(bytes_sent == 48);
            assert(bytes_sent_pre == 0);
        } else {
            assert(bytes_sent == 0);
            assert(bytes_sent_pre - 56 == 16); // 56 always sent to synchronize vector sizes
        }
        assert(circ.gates_by_level.size() == 3);

        
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
      "Benchmark a simple test featuring a circuit that uses some simple primitive gates");
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
