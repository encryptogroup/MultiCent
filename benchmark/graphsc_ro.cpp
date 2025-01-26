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

using namespace graphsc;
using json = nlohmann::json;
namespace bpo = boost::program_options;


common::utils::Circuit<Ring> generateCircuit(size_t vec_size) {

    std::cout << "Generating circuit" << std::endl;


    std::vector<std::vector<int>> permutation;
    std::vector<int> tmp_perm(vec_size);
    for (int i = 0; i < vec_size; ++i) {
        tmp_perm[i] = i;
    }
    

    common::utils::Circuit<Ring> circ;

    int num_vert = vec_size * 0.1;
    int num_edge = vec_size - num_vert;
    int rounds = floor(log(num_vert)/log(2));

    Ring zero = 0;
    Ring c = -1;


    std::vector<common::utils::wire_t> dag_list_isV(vec_size);
    std::vector<common::utils::wire_t> dag_list_data(vec_size);
    std::vector<common::utils::wire_t> dag_list_x(vec_size);
    std::generate(dag_list_data.begin(), dag_list_data.end(), [&]() { return circ.newInputWire(); });
    std::generate(dag_list_isV.begin(), dag_list_isV.end(), [&]() { return circ.newInputWire(); });
    std::generate(dag_list_x.begin(), dag_list_x.end(), [&]() { return circ.newInputWire(); });

    std::cout << "1" << std::endl;

    // Estimation of Scatter

    std::vector<common::utils::wire_t> val(vec_size);
    std::vector<common::utils::wire_t> onlyedges(vec_size);
    

    for (int i = 0; i < vec_size; i++) {
        onlyedges[i] = circ.addConstOpGate(common::utils::GateType::kConstAdd, dag_list_isV[i], Ring(-1));
    }

    // four multiplications per level
    for (int level = 0; level < rounds; level++) {
        for (int i = 1; i < vec_size; i++){

            int divisor = 1 << level;
            int L = (i / divisor) * divisor;

            auto isedge = circ.addConstOpGate(common::utils::GateType::kConstAdd, dag_list_isV[i], Ring(-1));
            auto tmp1 = circ.addGate(common::utils::GateType::kMul, isedge, onlyedges[L]);
            auto tmp2 = circ.addGate(common::utils::GateType::kMul, val[i], val[L]);
            val[i] = circ.addGate(common::utils::GateType::kMul, tmp1, tmp2);
            onlyedges[i] = circ.addGate(common::utils::GateType::kMul, onlyedges[L], tmp1);

        }
        
    }

    for (int i = 0; i < vec_size; ++i) {
        dag_list_data[i] = val[i];
    }


    // SRC TO DST
    auto tmp2 = circ.addMGate(common::utils::GateType::kShuffle, dag_list_data);
    // auto tmp3 = circ.addMGate(common::utils::GateType::kShuffle, dag_list_isV);

    // // apply public permutation
    for (int i = 0; i < vec_size; ++i){
        dag_list_data[i] = tmp2[i];
        // dag_list_isV[i] = tmp3[i];
    }

    // GATHER
    for (int level = 0; level < rounds; level++) {
        for (int i = 1; i < vec_size; i++){

            int divisor = 1 << level;
            int L = (i / divisor) * divisor;

            auto isedge = circ.addConstOpGate(common::utils::GateType::kConstAdd, dag_list_isV[i], Ring(-1));
            auto tmp1 = circ.addGate(common::utils::GateType::kMul, isedge, onlyedges[L]);
            auto tmp2 = circ.addGate(common::utils::GateType::kMul, val[i], val[L]);
            val[i] = circ.addGate(common::utils::GateType::kMul, tmp1, tmp2);
            onlyedges[i] = circ.addGate(common::utils::GateType::kMul, onlyedges[L], tmp1);

        }
        
    }

    for (int i = 0; i < vec_size; i++) {
        dag_list_data[i] = val[i];
    }


    for (int i = 0; i < dag_list_data.size(); ++i) {
        circ.setAsOutput(dag_list_data[i]);
    }

    std::cout << "Done generating circuit" << std::endl;

    return circ;
}


void benchmark(const bpo::variables_map& opts) {

    bool save_output = false;
    std::string save_file;
    if (opts.count("output") != 0) {
        save_output = true;
        save_file = opts["output"].as<std::string>();
    }

    auto vec_size = opts["vec-size"].as<size_t>();
    auto pid = opts["pid"].as<size_t>();
    auto threads = opts["threads"].as<size_t>();
    auto seed = opts["seed"].as<size_t>();
    auto repeat = opts["repeat"].as<size_t>();
    auto port = opts["port"].as<int>();

    std::shared_ptr<io::NetIOMP> network = nullptr;
    if (opts["localhost"].as<bool>()) {
        network = std::make_shared<io::NetIOMP>(pid, 3, port, nullptr, true);
    }
    else {
        std::ifstream fnet(opts["net-config"].as<std::string>());
        if (!fnet.good()) {
        fnet.close();
        throw std::runtime_error("Could not open network config file");
        }
        json netdata;
        fnet >> netdata;
        fnet.close();

        std::vector<std::string> ipaddress(3);
        std::array<char*, 5> ip{};
        for (size_t i = 0; i < 3; ++i) {
            ipaddress[i] = netdata[i].get<std::string>();
            ip[i] = ipaddress[i].data();
        }

        network = std::make_shared<io::NetIOMP>(pid, 3, port, ip.data(), false);
    }

    json output_data;
    output_data["details"] = {{"vec_size", vec_size},
                                {"pid", pid},
                                {"threads", threads},
                                {"seed", seed},
                                {"repeat", repeat}};
    output_data["benchmarks"] = json::array();

    std::cout << "--- Details ---\n";
    for (const auto& [key, value] : output_data["details"].items()) {
        std::cout << key << ": " << value << "\n";
    }
    std::cout << std::endl;

    auto circ = generateCircuit(vec_size).orderGatesByLevel();

    std::cout << "--- Circuit ---\n";
    std::cout << circ << std::endl;
    
    std::unordered_map<common::utils::wire_t, int> input_pid_map;
    std::unordered_map<common::utils::wire_t, Ring> input_map;

    for (const auto& g : circ.gates_by_level[0]) {
        if (g->type == common::utils::GateType::kInp) {
        input_pid_map[g->out] = 1;
        input_map[g->out] = 1;
        }
    }


    emp::PRG prg(&emp::zero_block, seed);
    

    OfflineEvaluator off_eval(pid, network, circ, threads, seed);
    network->sync();

    auto preproc = off_eval.run(input_pid_map, vec_size);
   
    // StatsPoint end_pre(*network);
    OnlineEvaluator eval(pid, network, std::move(preproc), circ, 
                threads, seed);
    
    eval.setRandomInputs();

    StatsPoint start(*network);
    

    for (size_t i = 0; i < circ.gates_by_level.size(); ++i) {
        eval.evaluateGatesAtDepth(i);    
    }

    
    StatsPoint end(*network);
    auto rbench = end - start;
    output_data["benchmarks"].push_back(rbench);

    size_t bytes_sent = 0;
    for (const auto& val : rbench["communication"]) {
        bytes_sent += val.get<int64_t>();
    }

    // std::cout << "--- Repetition " << r + 1 << " ---\n";
    std::cout << "time: " << rbench["time"] << " ms\n";
    std::cout << "sent: " << bytes_sent << " bytes\n";

    std::cout << std::endl;

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

// clang-format off
bpo::options_description programOptions() {
    bpo::options_description desc("Following options are supported by config file too.");
    desc.add_options()
        ("vec-size,v", bpo::value<size_t>()->required(), "Number of gates at each level.")
        ("pid,p", bpo::value<size_t>()->required(), "Party ID.")
        ("threads,t", bpo::value<size_t>()->default_value(6), "Number of threads (recommended 6).")
        ("seed", bpo::value<size_t>()->default_value(200), "Value of the random seed.")
        ("net-config", bpo::value<std::string>(), "Path to JSON file containing network details of all parties.")
        ("localhost", bpo::bool_switch(), "All parties are on same machine.")
        ("port", bpo::value<int>()->default_value(10000), "Base port for networking.")
        ("output,o", bpo::value<std::string>(), "File to save benchmarks.")
        ("repeat,r", bpo::value<size_t>()->default_value(1), "Number of times to run benchmarks.");

  return desc;
}
// clang-format on

int main(int argc, char* argv[]) {

    auto prog_opts(programOptions());
    bpo::options_description cmdline(
      "Benchmark online phase for multiplication gates.");
    
    cmdline.add(prog_opts);
    cmdline.add_options()(
      "config,c", bpo::value<std::string>(),
      "configuration file for easy specification of cmd line arguments")(
      "help,h", "produce help message");

    bpo::variables_map opts;
    bpo::store(bpo::command_line_parser(argc, argv).options(cmdline).run(), opts);
    if (opts.count("help") != 0) {
        std::cout << cmdline << std::endl;
        return 0;
    }

    if (opts.count("config") > 0) {
        std::string cpath(opts["config"].as<std::string>());
        std::ifstream fin(cpath.c_str());

        if (fin.fail()) {
            std::cerr << "Could not open configuration file at " << cpath << "\n";
            return 1;
        }

        bpo::store(bpo::parse_config_file(fin, prog_opts), opts);
    }
    try {
        bpo::notify(opts);

        if (!opts["localhost"].as<bool>() && (opts.count("net-config") == 0)) {
            throw std::runtime_error("Expected one of 'localhost' or 'net-config'");
        }
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return 1;
    }

    try {
        benchmark(opts);
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}
