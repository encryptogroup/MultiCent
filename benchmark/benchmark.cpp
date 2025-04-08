#include <algorithm>
#include <boost/program_options.hpp>
#include <cmath>
#include <iostream>
#include <memory>
#include <fstream>

#include <io/netmp.h>

#include "utils.h"
#include "benchmark.h"

// clang-format off
bpo::options_description bench::programOptions() {
    bpo::options_description desc("Following options are supported by config file too. Regarding seeds, all, 01, 02, and 12 need to be equal among the parties using them. Other parties, e.g., party 2 for seed 01 can take any value and in fact must not know the value that 0 and 1 use");
    desc.add_options()
        ("pid,p", bpo::value<size_t>()->required(), "Party ID.")
        ("threads,t", bpo::value<size_t>()->default_value(6), "Number of threads (recommended 6).")
        ("seed_self_h", bpo::value<size_t>()->default_value(100), "Value of the private random seed, high bits.")
        ("seed_self_l", bpo::value<size_t>()->default_value(0), "Value of the random seed, low bits (will add pid if 0/default).")
        ("seed_all_h", bpo::value<size_t>()->default_value(4), "Value of the global random seed, high bits.")
        ("seed_all_l", bpo::value<size_t>()->default_value(8), "Value of the global random seed, low bits.")
        ("seed_01_h", bpo::value<size_t>()->default_value(15), "Value of the 01-shared random seed, high bits.")
        ("seed_01_l", bpo::value<size_t>()->default_value(16), "Value of the 01-shared random seed, low bits.")
        ("seed_02_h", bpo::value<size_t>()->default_value(23), "Value of the 02-shared random seed, high bits.")
        ("seed_02_l", bpo::value<size_t>()->default_value(42), "Value of the 02-shared random seed, low bits.")
        ("seed_12_h", bpo::value<size_t>()->default_value(108), "Value of the 12-shared random seed, high bits.")
        ("seed_12_l", bpo::value<size_t>()->default_value(1337), "Value of the 12-shared random seed, low bits.")
        ("net-config", bpo::value<std::string>(), "Path to JSON file containing network details of all parties.")
        ("localhost", bpo::bool_switch(), "All parties are on same machine.")

        ("certificate_path", bpo::value<std::string>()->default_value("certs/cert1.pem"), "Path to full certificate chain file for TLS server connections")
        ("private_key_path", bpo::value<std::string>()->default_value("certs/key1.pem"), "Path to private key for TLS server connections")
        ("trusted_cert_path", bpo::value<std::string>()->default_value("certs/cert_ca.pem"), "Path with trusted certificate for TLS client connections")

        ("port", bpo::value<int>()->default_value(10000), "Base port for networking.")
        ("output,o", bpo::value<std::string>(), "File to save benchmarks.")
        ("repeat,r", bpo::value<size_t>()->default_value(1), "Number of times to run benchmarks.");

  return desc;
}
// clang-format on

bpo::variables_map bench::parseOptions(bpo::options_description& cmdline, bpo::options_description& prog_opts, int argc, char* argv[]){
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
            throw std::runtime_error("Conf file missing");
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
        throw std::runtime_error("Error occured");
    }

    return opts;
}

using json = nlohmann::json;

void bench::setupBenchmark(const bpo::variables_map& opts, size_t& pid, size_t& repeat, size_t& threads,
        std::shared_ptr<io::NetIOMP>& network, uint64_t* seeds_h, uint64_t* seeds_l, bool& save_output, std::string& save_file) {
    save_output = false;
    if (opts.count("output") != 0) {
        save_output = true;
        save_file = opts["output"].as<std::string>();
    }

    pid = opts["pid"].as<size_t>();
    threads = opts["threads"].as<size_t>();

    seeds_h[0] = opts["seed_self_h"].as<uint64_t>();
    seeds_h[1] = opts["seed_all_h"].as<uint64_t>();
    seeds_h[2] = opts["seed_01_h"].as<uint64_t>();
    seeds_h[3] = opts["seed_02_h"].as<uint64_t>();
    seeds_h[4] = opts["seed_12_h"].as<uint64_t>();
    seeds_l[0] = opts["seed_self_l"].as<uint64_t>();
    if (seeds_l[0] == 0) {
        // Default value, but as this is own seed, make unique per party
        seeds_l[0] = pid;
    }
    seeds_l[1] = opts["seed_all_l"].as<uint64_t>();
    seeds_l[2] = opts["seed_01_l"].as<uint64_t>();
    seeds_l[3] = opts["seed_02_l"].as<uint64_t>();
    seeds_l[4] = opts["seed_12_l"].as<uint64_t>();

    repeat = opts["repeat"].as<size_t>();
    auto port = opts["port"].as<int>();

    auto certificate_path = opts["certificate_path"].as<std::string>();
    auto private_key_path = opts["private_key_path"].as<std::string>();
    auto trusted_cert_path = opts["trusted_cert_path"].as<std::string>();

    if (opts["localhost"].as<bool>()) {
        network = std::make_shared<io::NetIOMP>(pid, 3, port, nullptr, certificate_path, private_key_path, trusted_cert_path, true);
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

        network = std::make_shared<io::NetIOMP>(pid, 3, port, ip.data(), certificate_path, private_key_path, trusted_cert_path, false);
    }
}
