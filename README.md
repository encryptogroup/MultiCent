# MultiCent

This is the implementation of MultiCent, a framework of MPC protocols to compute centrality measures on (multi)graphs.
The framework enables to run message-passing algorithms with a linear aggregation function on (multi)graphs.
It includes the instantiation of three specific centrality measures within the framework, but allows for relatively easy extension.

**Check out [our paper](https://eprint.iacr.org/2025/652), accepted at PETs'25, for details!**

:warning: This code is a prototype implementation and should not be used in production.

### Framework for Message-Passing Algorithms

This repository is a fork of [Graphiti](https://github.com/Bhavishrg/Graphiti) ([paper](https://eprint.iacr.org/2024/1756)).
Graphiti introduces a framework for efficiently evaluating message-passing algorithms with a linear aggregation function in MPC.
In such algorithms, nodes hold some state and multiple iterations then
- propagate these states to outgoing edges,
- linearly gather/aggregate data over all incoming edges to any node,
- and use the gathered data to update the node's state according to a customizable update function.
Compared to Graphiti, this framework
- fixes many correctness and security issues in the circuit representation, communication channels, and protocol implementations,
- provides a full solution whereas Graphiti's code is only usable for micro-benchmarking of some components,
- improves the protocol by instantiating required sorting in a more suitable and scalable fashion,
- and supports multigraphs whereas Graphiti was designed with only standard graphs in mind.

### Centrality measures

Using this framework, we instantiate three centrality measures (computed on all nodes v):
- Reach Score (pi_R/pi_1): Nodes reachable in at most D hops from v
- Truncated Katz Score (pi_K/pi_2): Paths up to length D, starting at v, weighted depending on their length.
- Multilayer Truncated Katz Score (pi_M/pi_3): Like pi_K, but counting paths multiple times if they use different edges between the same nodes.
:warning: [Our paper](https://eprint.iacr.org/2025/652) uses naming pi_R/pi_K/pi_M whereas [this prior work](https://doi.org/10.1145/3038912.3052602) uses pi_1/pi_2/pi_3. Throughout the code, we also use naming pi_1/pi_2/pi_3 as the switch to pi_R/pi_K/pi_M came after the code.
We also translate the protocols from [the prior work](https://doi.org/10.1145/3038912.3052602) to our setting and provide implementation within this repository.


# Table of Contents

* [TL;DR](:tl-dr): Quick walkthrough, demonstrating how to run benchmarks on smaller graph instances, plot them, etc.
* [Environment](:environment): Setting up the environment to compile and run MultiCent, either using Docker or standalone
* [Compiling](:compiling): Compiling MultiCent
* [Running the Protocols](:running-the-protocols): Guide to run individual protocols
* [Reproducing our Benchmarks](:reproducing-our-benchmarks): Guide to reproduce benchmarks as done in the paper
* [Parsing and Processing Benchmark Data](:parsing-and-processing-benchmark-data): Guide to parse and display benchmark data as plot and tables
* [Running Tests](:running-tests): Guide on running test cases
* [Repository Content](:repository-content): Outline of which code parts to find where


# TL;DR

This section provides a brief walkthrough, installing MultiCent inside a Docker container and running benchmarks locally on your machine.
You should have 12 GB RAM available for that to ensure that no benchmarks run out of memory.
```sh
# Setting up and running container
sudo docker buildx build -t multicent .
sudo docker run -it -v $(pwd):$(pwd) -w $(pwd) --cap-add=NET_ADMIN multicent
# Proceed working inside the container...

# Compiling MultiCent
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j8 benchmarks
cd benchmarks

# Testing that installation successful
./pi_3_benchmark --localhost --depth 2 --nodes 10 --size 20 --pid 0 > /dev/null &
./pi_3_benchmark --localhost --depth 2 --nodes 10 --size 20 --pid 2 > /dev/null &
./pi_3_benchmark --localhost --depth 2 --nodes 10 --size 20 --pid 1

# Running small benchmarks
./small_LAN_benchmarks.sh # Will take approx. TODO TIME
./small_WAN_benchmarks.sh # Will take approx. TODO TIME

# Plotting benchmark results
cd ../../evaluation_scripts
python3 dataset_plots.py
python3 scalability_plots_betterspacing.py altScale
python3 extended_scalability_plots.py altScale
python3 network_plots.py altScale
# Results are written to PDFs inside evaluation_scripts
```

For plotting original benchmark results our outputting tables with full data, further information can be found [here](:parsing-and-processing-benchmark-data).
If you wish to run tests, go [here](:running-tests).


# Environment

We provide information how to set up the environment for MultiCent using Docker or standalone.
Please note that our benchmarks were conducted using the standalone method, running each party on a separate server.

## Using Docker

We provide a Dockerfile to set up the environment in a more simple manner.
Run the following command to build the image:
```sh
sudo docker buildx build -t multicent .
```
Then, run the image as follows (```--cap-add=NET_ADMIN``` is required for emulating realistic network behavior later):
```sh
sudo docker run -it -v $(pwd):$(pwd) -w $(pwd) --cap-add=NET_ADMIN multicent
```

## Standalone

The protocol is implemented in C++17 and [CMake](https://cmake.org/) is used as the build system.
It has been tested on Arch Linux (plain and Manjaro Linux) with GCC 14.2.1 and on x86_64 architecture.

The following libraries need to be installed separately and should be available to the build system and compiler:
- [GMP](https://gmplib.org/)
- [NTL](https://www.shoup.net/ntl/) (11.0.0 or later)
- [Boost](https://www.boost.org/) (1.72.0 or later)
- [Nlohmann JSON](https://github.com/nlohmann/json)
- [OpenSSL](https://github.com/openssl/openssl)
- [EMP Tool](https://github.com/emp-toolkit/emp-tool)
- To parse benchmark output data and reproduce the plots from our paper, we additionally require python3 and matplotlib.

All except for EMP Tool should be possible to install from standard repositiories.
For EMP Tool, please follow the instructions from their github page.

Note that for testing different network settings, depending on your testbed, it might be required to simulate network behaviour.
This can be achieved using [tc](https://www.man7.org/linux/man-pages/man8/tc.8.html).
[local_demo](local_demo) contains examples to achieve our considered network settings if all parties are running on the same server.
For setups using multiple servers, these scripts would need to be modified according to the specific networking interfaces etc.


# Compiling

To compile (inside a docker or standalone environment), run the following commands from the root directory of the repository.

```sh
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..

make -j8 benchmarks
```

For testing purposes, the build type should be changed from ```Release``` to ```Debug```.


# Running the Protocols

All binaries will be written to build/benchmarks.
Set up any network environment simulation if needed, e.g., using [tc](https://www.man7.org/linux/man-pages/man8/tc.8.html).
If running all parties on the same machine, you can use the scripts inside [local_demo](local_demo) to do that.
For any of the binaries, use the ```-h``` flag to show the required CLI arguments.

Example:
```sh
./pi_3_benchmark --localhost --depth 2 --nodes 10 --size 20 --pid [PID]
```
will start a benchmark for pi_3^D as party PID (0: helper, 1/2: other parties), expecting the other parties to run on the same machine (```--localhost```), and using depth D=2, 10 nodes, size 20 (nodes + edges, i.e., 10 directed edges in addition to the nodes).
**Please note that this command needs to be executed for PIDs 0 1 and 2, so either run each instance with ```&``` appended to run in the background, or use multiple shell windows.**
If one PID is missing, the other processes will stall until all parties are available.

If you run the parties on different machines, copy [net_config.json](net_config.json) into build/benchmarks and replace the placeholders inside by the actual IPs of the three parties.
This can then be used automatically as follows:
```sh
./pi_3_benchmark --net-config netconfig.json --depth 2 --nodes 10 --size 20 --pid [PID]
```

The benchmarks include the following targets:
* pi_3, pi_2, pi_1 are the different centrality measures
* the _ref version corresponds to the prior [WWW'17 protocol]((https://doi.org/10.1145/3038912.3052602)) which we implemented in our setting for a fair comparison
* the _test prefix corresponds to a test instance where the correctness of the output and the communication is checked
* the _benchmark prefix corresponds to a benchmark instance for variable sized graphs, where only the communication is checked
* there also are additional tests test, shuffle, doubleshuffle, compaction, sort, equalszero to test some of the used primitives standalone


# Reproducing our Benchmarks

In our paper, we provide benchmarks of our protocols.
These were done on one dedicated server per party, each server equipped with an Intel Core i9-7960X CPU @ 2.8 GHz, 128 GB of DDR4 RAM @ 2666 MHz.
We considered a LAN setting with 1 ms RTT and 1 GBit/s bandwidth, and a WAN setting with 100 ms RTT and 100 MBit/s bandwidth.

## Datasets

Note that our MPC protocols are parametrized by the number of nodes and total graph size only.
They are oblivious of the exact graph structure which must not be leaked by the control flow.
Hence, it is not required to provide actual graph instances as input, any graph with the same parameters will lead to the same performance.
For the datasets in Table 2 of the paper, we simply generate a synthetic graph with the same dimensions.
For undirected graphs, for each edge {v,w} we insert directed edges (v,w) and (w,v), doubling the number of edges.

## Smaller Local Demo Benchmarks

As our full benchmarks require up to 120 GB RAM per party and to be run on three separate machines, we provide a downscaled demo.
This demo is carefully designed so that RAM utilization per party remains below 4 GB.
Hence, with 12 GB RAM available for the benchmarks, all three parties can be started on the same machine, locally emulating a network.
Also, it just does 3 iterations for each configuration.

Before running the benchmarks, please ensure that build/benchmarks/p0, build/benchmarks/p1, and build/benchmarks/p2 do not exist or are empty, potentially delete all files inside.
These directories may still contain files from past benchmark runs if this is not your first benchmark of MultiCent.
This will not directly influence benchmarking, but if there is an overlap of benchmarks, new data is just appended.
Our provided scripts may then later be unable to correctly parse the benchmark data or do so incorrectly:
```sh
rm -rf p0
rm -rf p1
rm -rf p2
```

Then, run the LAN benchmarks.
Our script will automatically enable the network simulation and disable it in the end again.
We recommend working in the Docker container as this will momentarily change the behavior of your loopback interface.
Running this script will take approx. TODO TIME.
```sh
./small_LAN_benchmarks.sh
```
After the LAN benchmarks finished, you can proceed with the WAN benchmarks.
Do **not** delete build/benchmarks/p0 etc. if previously generated from the LAN benchmarks.
Running this script will take approx. TODO TIME.
```sh
./small_WAN_benchmarks.sh
```

## Full Benchmarks as Done in the Paper

First, we strongly advise **not** to use our full benchmark scripts in their current state as they may swiftly exhaust system resources.
We measures **up to 120 GB RAM utilization on each server** by testing the limits of our scalability, these numbers can also be found in Appendix C of [our paper](https://eprint.iacr.org/2025/652).
Instead, the full scripts document how exactly we conducted the benchmarks and can serve as a basis for modified benchmark scripts tailored to your system resources.

The scripts are [evaluation_scripts/run_LAN_benchmarks.sh](evaluation_scripts/run_LAN_benchmarks.sh) and [evaluation_scripts/run_WAN_benchmarks.sh](evaluation_scripts/run_WAN_benchmarks.sh), designed for using three different machines for the evaluation.
They are automatically copied into your build directory when compiling the project and need to be run from there.
Please ensure first that the network setting is simulated correctly and that ```net_config.json``` has been extended by the IPs of all servers and copied into build/benchmarks.
Then, party PID (0, 1, 2) can be started as follow from within build/benchmarks:
```sh
./run_LAN_benchmarks.sh [PID]
```
The WAN benchmarks work the same, swapping out the respective script.
Note that both scripts are protected by a warning and subsequent termination as a safeguard due to high RAM utilization.
Hence, the first line of each script needs to be removed before running the full benchmarks.


# Parsing and Processing Benchmark Data

Running the benchmarks will create and populate directories p0, p1, p2 inside of build/benchmarks.
If you run local benchmarking as documented [here](:Smaller-Local-Demo-Benchmarks), these should afterwards all exist.
If you run different parties on different machines, you have to collect these directories inside build/benchmarks of one of the machines first from which you then also follow the next steps.

Navigate into directory evaluation_scripts.
This contains multiple python scripts intended to parse and process the generated benchmark data, aggregating over the numbers collected by each individual party.

The plots are generated as follows:
- ```python3 dataset_plots.py``` will draw the plots from Fig. 7 of the paper, displaying efficiency for different values D and different datasets.
- ```python3 scalability_plots_betterspacing.py``` will draw the plots from Fig. 8 of the paper, displaying efficiency for different graph sizes.
- ```python3 extended_scalability_plots.py``` will draw the plots from Fig. 9 of the paper, displaying efficiency for different larger graph sizes for pi_3, divided into total and online cost.
- ```python3 network_plots.py``` will draw the plots from Fig. 21 (Appendix C) of the paper, comparing LAN to WAN performance.
If used from within the Docker container, the plots will not be displayed, but anyway written to a pdf in the same directory which can then be opened from the host system.
All of these scripts draw run time depending on which data points were included in the benchmarks.
Using the [smaller demo benchmarks](:Smaller-Local-Demo-Benchmarks), parts of the plots may hence be missing if the RAM utilization otherwise would be too high.
Communication is computed analytically according to Table 4 in the paper to show communication even where benchmarks could not be run.
By calling, e.g., ```python3 scalability_plots_betterspacing.py altScale```, the scale of the plot can be changed from the original used in the paper to whatever fits the actually benchmarked datapoints best.
```altScale``` is supported for all scripts except for ```python3 dataset_plots.py```.
We recommend using it if the scaling otherwise makes it hard to see the plots.

Furthermore, the data also shown in the tables in Appendix C of the paper can be viewed using:
- ```python3 analyze.py datasets_large```: Data from Table 5
- ```python3 analyze.py datasets_small```: Data from Table 6
- ```python3 analyze.py scalability_pi3```: Data from Table 7
- ```python3 analyze.py scalability_pi3_WAN```: Data from Table 8
- ```python3 analyze.py scalability_pi2```: Data from Table 9
- ```python3 analyze.py scalability_pi1```: Data from Table 10

Any of the plotting and table scripts also allows to add another flag ```original```, e.g., ```python3 dataset_plots.py original```.
This will ignore your benchmarking data and instead use the data from evaluation_scripts/raw_benchmark_outputs.
This data is the output of our original benchmarks that all reported benchmarking data in the paper is based on.


# Running Tests

To run tests, first [recompile](:Compiling) for debug mode.
Then, navigate to build/benchmarks and run
```sh
./run_test.sh [id]
```
Depending on the value of [id], this will run one of the following tests (0-5 are for building blocks):
0. test: Small test circuit with *, +, XOR, AND gates; test for correct output and communication
1. shuffle: Test shuffle implementation for vectors of size 10, test for correct output and communication
2. doubleshuffle: Test modified 'doubleshuffle' implementation for vectors of size 10, test for correct output and communication
3. compaction: Compute compaction of vector of size 10, test for correct output and communication
4. sort: Sort vector of size 10, test for correct output and communication
5. equalszero: Compute EQZ and then Bit2A on multiple values, test for correct output and communication
6. pi_3_test: pi_3 (D=4) on concrete graph instance with 4 nodes and 6 edges, test for correct output and communication
7. pi_3_benchmark: pi_3 (D=0) on synthetic graph instance with 10 nodes, total size 20, test for correct communication only
8. pi_3_benchmark: pi_3 (D=1) on synthetic graph instance with 10 nodes, total size 20, test for correct communication only
9. pi_3_benchmark: pi_3 (D=2) on synthetic graph instance with 10 nodes, total size 20, test for correct communication only
10. pi_3_ref_test: reference pi_3 (D=4) on concrete graph instance with 4 nodes and 6 edges, 3 layers, test for correct output and communication
11. pi_3_ref_benchmark: pi_3 (D=1) on synthetic graph instance with 10 nodes, 3 layers, test for correct communication only
12. pi_3_ref_benchmark: pi_3 (D=2) on synthetic graph instance with 10 nodes, 3 layers, test for correct communication only
13. pi_2_test: like above, but for pi_2
14. pi_2_benchmark: like above, but for pi_2
15. pi_2_benchmark: like above, but for pi_2
16. pi_2_benchmark: like above, but for pi_2
17. pi_2_ref_test: like above, but for pi_2
18. pi_2_ref_benchmark: like above, but for pi_2
19. pi_2_ref_benchmark: like above, but for pi_2
20. pi_1_test: like above, but for pi_1
21. pi_1_benchmark: like above, but for pi_1
22. pi_1_benchmark: like above, but for pi_1
23. pi_1_benchmark: like above, but for pi_1
24. pi_1_ref_test: like above, but for pi_1
25. pi_1_ref_benchmark: like above, but for pi_1
26. pi_1_ref_benchmark: like above, but for pi_1


# Repository Content

Note that the overall structure of the repository stems from the structure of [Graphiti](https://github.com/Bhavishrg/Graphiti).
In the following, we briefly outline which essential parts of MultiCent implementation can be found where: 
* benchmark/subcircuits.h and benchmark/subcircuits.cpp generate the circuits for all centrality measures. **Here, it can be found how our different high-level protocols such as computation of centrality measures, sorting, etc. are composed of low-level atomic building blocks such as shuffling, multiplications, etc.**
* benchmark contains all benchmarks and test cases. Hence, it also demonstrates how our protocols can be instantiated and used. The benchmarks essentially define the inputs and parameters for the circuit and then let benchmark/subcircuits.cpp generate the circuit itself.
* src/graphsc/offline_evaluator.cpp contains the preprocessing for all atomic building blocks. In contrast to Graphiti, building blocks such as shuffling are fixed here and further building blocks like double shuffling etc. are introduced.
* src/graphsc/offline_evaluator.cpp contains the online phase for all atomic building blocks. In contrast to Graphiti, building blocks such as shuffling are fixed here and further building blocks like double shuffling and more efficient propagation etc. are introduced.
