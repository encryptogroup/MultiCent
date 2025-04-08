# MultiCent

This is an end-to-end implementation of Graphiti with the optimizations from MultiCent, also implementing the new centrality measure protocols
from MultiCent as well as prior reference ones for comparison purposes.

The protocol is implemented in C++17 and [CMake](https://cmake.org/) is used as the build system.
It has been tested on Arch Linux (plain and Manjaro Linux) with GCC 14.2.1

This repository is a fork of [Graphiti](https://github.com/Bhavishrg/Graphiti).
It adds multiple fixes to several issues in the circuit representation, communication channels and protocol implementations.
Furthermore, it extends the Graphiti codebase to an end-to-end implementation, also utilizing the optimizations provided in our paper.
Finally, it implements different centrality measure computations.

## External Dependencies
The following libraries need to be installed separately and should be available to the build system and compiler.

- [GMP](https://gmplib.org/)
- [NTL](https://www.shoup.net/ntl/) (11.0.0 or later)
- [Boost](https://www.boost.org/) (1.72.0 or later)
- [Nlohmann JSON](https://github.com/nlohmann/json)
- [OpenSSL](https://github.com/openssl/openssl)
- [EMP Tool](https://github.com/emp-toolkit/emp-tool)

All except for EMP Tool should be possible to install from standard repositiories.
For EMP Tool, follow the instructions from their github page.


## Compilation
To compile, run the following commands from the root directory of the repository:

```sh
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..

make -j8 benchmarks
```

For testing purposes, the build type can be changed to Debug.

## Running the Protocols
All binaries will be written to build/benchmarks.
Use the ```-h``` flag to show the required CLI arguments.

**Important note**: The naming scheme of used metrics here differs from the paper. pi_3 refers to pi_M, i.e., multilayer Katz; pi_2 refers to pi_K, i.e., truncated Katz; and pi_1 refers to pi_R, i.e., reach score.

Example:
```sh
./pi_3_benchmark --localhost --depth 2 --nodes 10 --size 20 --pid [PID]
```
will start a benchmark for pi_3^D with all servers on the same machine, with D=2, 10 nodes, size 20 (nodes + edges, i.e., 10 directed edges in addition
to the nodes) and for PID which has to be 0 for the helper, 1 for one server and 2 for the other, all of which need to be started on their own.

## Structure

The benchmarks include the following targets:
* pi_3, pi_2, pi_1 are the different centrality measures
* the _ref version corresponds to the prior WWW'17 protocol which we implemented in our setting for a fair comparison
* the _test prefix corresponds to a test instance where the correctness of the output and the communication is checked
* the _benchmark prefix corresponds to a benchmark instance for variable sized graphs, where only the communication is checked
* there also are additional tests test, shuffle, doubleshuffle, compaction, sort, equalszero to test some of the used primitives standalone

In addition, we provide the following scripts:
* run_LAN_benchmarks.sh was used by us to run all benchmarks featured in the paper for the LAN setting (the network setting has to be configures separately)
* run_WAN_benchmarks.sh is the same for the WAN setting (the network setting has to be configures separately)
* run_test takes as arguments the PID and then some instance and was just used for quick testing of the different protoocols when built in Debug mode.

The implementation of our protocols can be found in the following places:
* the benchmark directory contains examples in the tests and benchmarks of how our protocols can be instantiated and used
* the same directory also contains subcircuits.cpp/subcircuits.h. These contain generators for the subcircuits used for message-passing, sorting, etc.; hence they are of central relevance for all implemented protocols
* in addition, the protocols rely on low-level primitives such as shuffling, multiplying, ... These can be found mainly in src/graphsc in online_evaluator_load_balanced.cpp and offline_evaluator.cpp that define the exact behaviour of these low-level gates.
