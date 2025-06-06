# Artifact Appendix

Paper title: **MultiCent: Secure and Scalable Computation of Centrality Measures on Multilayer Graphs**

Artifacts HotCRP Id: **#11**

Requested Badge: all: **Reproduced**, **Functional**, **Available**

## Description
This artifact is the implementation of the protocols in the paper, used for running benchmarks for these protocols.
It contains implementation for the graph analysis protocols from the paper, additional implementation from the prior reference
protocols from Asharov et al. (WWW'17), and scripts to run benchmarks and retrieve the plots and further performance data used in the paper.
The repository also contains the raw output of the benchmark run that we have used for the paper.

### Security/Privacy Issues and Ethical Concerns (All badges)
None

## Basic Requirements (Only for Functional and Reproduced badges)
The software is only tested on x86_64 architecture.
For accurate benchmark results, a CPU with at least 4 cores should be used.
Main bottleneck is RAM, we provide scaled down benchmarks that should run on a machine with 16GB RAM (we need up to 12GB free inside a Docker container).
As the original benchmarks were done distributed with a total of >300GB RAM, the full benchmarks for all input sizes will likely not be feasible.
We think that the scaled down benchmarks suffice to verify our improvement and scalability claims sufficiently, but should more benchmarks
for larger graphs be requested, please let us know how much free RAM is available and we are happy to provide additional benchmark scripts doing all benchmarks possible within this updated constraint.
For more information on the exact RAM use (per one of the three parties), we refer to Appendix C of the paper.
More information can also be found at the end of this document under *Limitations*.
Our benchmarks take less than 2 hours in their current state.
If we scale them up again for more RAM, they will also take more time, but this strongly depends on how much RAM exactly is available.
Required disk space is at most a couple GB, mostly for a Docker image.

### Hardware Requirements
No special hardware is required, yet, an x86_64 should be used. RAM requirements see above.

### Software Requirements
We provide a Docker image in which our code can be run.

### Estimated Time and Storage Consumption
RAM requirements see above under Basic Requirements.
Our small LAN benchmarks run in approx 1 hour and the small WAN benchmarks in approx 40 minutes.
Should larger benchmarks be requested (see above), time would of course increase.
Disk space beyond for installing the Docker image is negligible.

## Environment
Simply clone the git-repository and then follow the next steps.
Everything will work inside a docker container.
Hence, only git and docker (including docker-buildx) need to be available to access and use the artifact.

### Accessibility (All badges)
Repository publicly available on GitHub: [https://github.com/encryptogroup/MultiCent](https://github.com/encryptogroup/MultiCent).
Specifically, the version to review is [release v0.1.0](https://github.com/encryptogroup/MultiCent/releases/tag/v0.1.0) which,
as an asset, also comes with a ready-to-use docker image.

### Set up the environment (Only for Functional and Reproduced badges)
Docker (including docker-buildx) and git need to be available on the host system.
Note that for setting up the container, you have to pick one of the choices presented in the script below:
Either build the container yourself, or download the [image provided as a release asset on GitHub](https://github.com/encryptogroup/MultiCent/releases/download/v0.1.0/multicent_image.tar.gz) and simply load it.
The image was build on x86_64 and *should* work on most common and sufficiently modern CPUs of that architecture.
```sh
git clone git@github.com:encryptogroup/MultiCent.git 
cd MultiCent
git checkout v0.1.0

# Setting up and running container:

# Either build docker image yourself from the provided dockerfile:
sudo docker buildx build -t multicent .
# Or download prepared image from the release assets on GitHub to the MultiCent directory and load it:
sudo docker load < multicent_image.tar.gz 

sudo docker run -it -v $(pwd):$(pwd) -w $(pwd) --cap-add=NET_ADMIN multicent
# Proceed working inside the container...

# Compiling MultiCent
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j8 benchmarks
cd benchmarks # Not cd benchmark (without the s)!
```

### Testing the Environment (Only for Functional and Reproduced badges)
```sh
# Testing that installation successful by starting all three parties (with their pids 0, 1, 2)
# for testing pi_3 with D=2, 10 nodes and a total graph size of 20
./pi_3_benchmark --localhost --depth 2 --nodes 10 --size 20 --pid 0 > /dev/null &
./pi_3_benchmark --localhost --depth 2 --nodes 10 --size 20 --pid 2 > /dev/null &
./pi_3_benchmark --localhost --depth 2 --nodes 10 --size 20 --pid 1
```
Now, the CLI output of party 1 should be visible which contains a couple parameters, circuit information, and measured performance.
It should report setup time, setup sent (outgoing traffic), time (online), and sent (outgoing online traffic).
In particular, the output may appear like the following:
```
[...]
--- Repetition 1 ---
setup time: 2.320152 ms
setup sent: 0 bytes
OnlineEvaluator constructed
Circuit evaluated
time: 1.620136 ms
sent: 5880 bytes

--- Overall Statistics ---
peak_resident_set_size: 11392
peak_virtual_memory: 912068
```
Following is the interpretation of this output, but note that this is not important for the later benchmarks as the data is also written to files that
can later be parsed and visualized with Python scripts procided by us.
The output indicates that party 1 has sent 0 bytes during setup/preprocessing which took 2.320152 ms,
while it has sent 5880 bytes in the online phase that took 1.620136 ms.
Peak virtual memory was at 912068 KB while the peak resident set size was 11392 KB.
Of course, the memory utilization in the overall statistics and the measured time can vary and hence be different, but traffic is expected to always be the same.

## Artifact Evaluation (Only for Functional and Reproduced badges)

### Main Results and Claims
MultiCent provides scalable and secure computation of centrality measures w.r.t. graph size and depth.
It significantly outperforms the prior state of the art by Asharov et al. (WWW'17) on smaller graphs while enabling computation on larger graphs
that cannot be handled by the work by Asharov et al.

#### Main Result 1: Scalability w.r.t. Graph Size and Depth
The protocols for pi_M and pi_K (called pi_3 and pi_2 within the code) have run time scaling well with O(n log n) for n nodes on sparse graphs (number edges in O(n)) (see Fig. 8 and Fig. 9 in the paper), for both preprocessing and online as also shown for pi_M in Fig. 9.
Considering the depth parameter D, they scale well with O(D) on graphs of fixed size (see Fig. 7 in the paper).
The protocol for pi_R (called pi_1 within the code) has run time scaling O(n^2) respective O(D) (see Fig. 7, 8, 9 in the paper).
Graph analysis using pi_M or pi_K is possible with graph sizes (#nodes + #edges) in the millions.

#### Main Result 2: Our Protocols Significantly Outperform the prior State of the Art
The reference protocol by Asharov et al. (WWW'17) is significantly outperformed by our protocols except for negligible graph sizes (Fig. 7 and 8 in the paper).
Furthermore, its memory constraints prevent it from being run on larger graphs where our protocol still works and provides good run times (Fig. 7 and 8 in the paper, memory constraints as depicted in tables inside Appendix C of the paper).

### Experiments 
As the main constraint for running our protocols is RAM, our benchmarks can be run depending on how much RAM is available.
We originally benchmarked on three machines with 128 GB RAM each, exhausting nearly all of that to test the limits of our system.
Note that our Docker setup runs all three parties on the same machine, essentially tripling the local RAM requirements.
Hence, the domain of graph sizes we can run the benchmarks on critically depends on the available RAM.
We provide scripts for running experiments using at most 12 GB of RAM.
We think that this is sufficient to verify our claims, but should this not be the case, we kindly ask the artifact reviewer how much available RAM they have.
We are happy to then provide additional benchmark scripts doing all benchmarks possible within this constraint, allowing to reproduce the experiments on larger graphs too, confirming that scalability indeed proceeds as to be expected from the smaller benchmarks.

**Note regarding considered datasets**:
Note that our MPC protocols are parametrized by the number of nodes and total graph size only.
They are oblivious of the exact graph structure which must not be leaked by the control flow.
Hence, it is not required to provide actual graph instances as input, any graph with the same parameters will lead to the same performance.
For the datasets in Table 2 of the paper, we simply generate a synthetic graph with the same dimensions.
For undirected graphs, for each edge {v,w} we insert directed edges (v,w) and (w,v), doubling the number of edges.
This is only for benchmarking purposes; our code of course also supports providing customized graphs as inputs, as can be seen,
e.g., [here](benchmark/pi_3_test.cpp), starting at line 102, for a small graph instance example.

#### Experiment 1: LAN Benchmarks
Most benchmarks in our paper are done in a LAN setting (Fig. 7, 8, 9, (in Appendix C:) Table 5, 6, 7, 9, 10).
The following will do LAN benchmarks for all of our protocols as well as the reference from Asharov et al.
They can be started as follows, assuming one is still in the docker container and inside the **build/benchmarks** directory:
```sh
./small_LAN_benchmarks.sh
```
Note that this will take approx 1 hour while consuming negligible additional disk space.
Shell outputs show that the benchmarks still are running, but can otherwise be ignored as all relevant data is also saved to files.

If this script is interrupted at some point, all obtained benchmark data should be removed (by deleting directories p0, p1, p2 inside build/benchmarks) before restarting the script.
Otherwise, problems might occur later when trying to parse the benchmark data.

For easy parsing and processing, we provide scripts.
They assume that the aforementioned benchmarks were executed completely, if this is not the case, you may get a missing file error.
Proceed as follows:
```sh
cd ../../evaluation_scripts
python3 dataset_plots.py altScale
python3 scalability_plots_betterspacing.py altScale
python3 extended_scalability_plots.py altScale
```
This will generate PDF files inside the evaluation_scripts directory on the host machine that were used for Fig. 7, 8, 9 in the paper.
Only run times for smaller graphs are benchmarked and hence plotted to require less RAM,
still, communication is analytically computed to show scalability beyond possible benchmarks.

Furthermore, the verbose data as provided in Appendix C, Table 5, 6, 7, 9, 10, can be printed as follows:
- ```python3 analyze.py datasets_large```: Data from Table 5
- ```python3 analyze.py datasets_small```: Data from Table 6
- ```python3 analyze.py scalability_pi3```: Data from Table 7
- ```python3 analyze.py scalability_pi2```: Data from Table 9
- ```python3 analyze.py scalability_pi1```: Data from Table 10

The results should match the data provided by the plots and tables in the paper for the graph sizes where benchmarks were possible.
Regarding the comparison to the reference protocol from the prior work (pi_M and pi_K), while its one-time cost might not be the highest among all datarows using the limited, smaller benchmarks, it should clearly be visible how it is scaling quadratically with |V|.
Hence, it can be extrapolated how this will become the highest for higher |V| as also depicted in the paper for larger graphs.
Still, the exact hardware and other environment factors may slightly influence the run times; this should not obstruct verifying the claims.
We also note that there might be fluctuations regarding allocated RAM, especially for smaller sizes, and for cost per iteration data if significantly smaller than the one-time cost.
This is caused by cost per iteration being deducted from the difference between runs with more and less iterations so that fluctuations of the one-time cost may yield deviations of measured per iteration cost.
Finally, we note that our provided benchmark scripts only uses 3 iterations for each data point whereas with more time, our original benchmarks used up to 10 iterations.

Overall, this experiment supports both of our prior claims as it includes benchmarks for our and the reference protocols.

We also provide the full outputs of our original benchmark runs using three machines inside evaluation_scripts/raw_benchmark_outputs.
These can also be parsed and visualized, using the python scripts for plotting and tables as above, but discarding of the ```altScale``` flag where used and instead using an ```original``` flag, e.g.,
```sh
python3 analyze.py scalability_pi2 original
```

#### Experiment 2: WAN Benchmarks
Appendix C also contains WAN benchmarks for pi_M, comparing to the LAN benchmarks, to attest that the prior scalability results also generalize to slower networks, being slowed down by some factor due to lower network performance.
This is connected to Fig. 21 and Table 8 in the paper.

The following assumes that the LAN benchmarks from Experiment 1 were already done and the generated benchmark data kept as it is also used here.
First, **navigate to build/benchmarks** in the docker container.
Then, start the WAN benchmarks as follows:
```sh
./small_WAN_benchmarks.sh
```
Note that this will take approx 40 minutes while consuming negligible additional disk space.
Shell outputs show that the benchmarks still are running, but can otherwise be ignored as all relevant data is also saved to files.

If this script is interrupted at some point, all obtained benchmark data should be removed (by deleting directories p0, p1, p2 inside build/benchmarks) before restarting the script.
Note that this would also require to run the LAN benchmarks again.
An alternative is to just delete all files within the directories that are prefixed by 'WAN_'
Otherwise, problems might occur later when trying to parse the benchmark data.

For easy parsing and processing, we again provide scripts.
They assume that the aforementioned benchmarks were executed completely, if this is not the case, you may get a missing file error.
Proceed as follows:
```sh
cd ../../evaluation_scripts
python3 network_plots.py altScale
```
This will generate a PDF file inside the evaluation_scripts directory on the host machine that was used for Fig. 21 in the paper.
Only run times for smaller graphs are benchmarked and hence plotted to require less RAM,
still, communication is analytically computed to show scalability beyond possible benchmarks.

Furthermore, the verbose data as provided in Table 8 can be printed as follows:
```sh
python3 analyze.py scalability_pi3_WAN
```

The results should match the data provided by the plot and table in the paper for the graph sizes where benchmarks were possible.
Still, yet again the exact hardware and other environment factors may slightly influence the run times and allocated RAM; this should not obstruct verifying the claims.
Also, we decreased the number of iterations per data point to 1 here.
Regarding the reference protocol, note that while one-time cost now is lower where it is able to run (as it requires no communication), this will be overshadowed by the deficits in cost per iteration.

We also provide the full outputs of our original benchmark runs using three machines inside evaluation_scripts/raw_benchmark_outputs.
These can also be parsed and visualized, using the python scripts as above, but discarding of the ```altScale``` flag where used and instead using an ```original``` flag, e.g.,
```sh
python3 network_plots.py original
```

#### Experiment 3: Optional Additional Testing
To test the correctness of our code, we provide some test cases.
If needed, these can be started as follows:

First, navigate to the **build directory** from within the Docker container.
Then, recompile in debug mode so that assertions become active:
```sh
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j8 benchmarks
cd benchmarks # Not cd benchmark (without the s)!
```
Now, run
```sh
./run_test.sh [id]
```
after replacing ```[id]``` by a number 0,1,...,26, each id corresponding to a specific test.
Documentation of what each test does can be found [here](README.md#running-tests).

The tests are expected to finish within a second and print some (circuit) parameters and performance numbers.
These can be ignored. 
If tests take longer, it might be the case that network restrictions still are being emulated which can be corrected by running ```./network_off.sh```.
Normal termination indicates that the test succeeded as all checks are done with assertions.
Hence, in case an error is detected, an assertion error will be raised instead which will be clearly visible in the test output.


## Limitations (Only for Functional and Reproduced badges)
As documented before at the start of the Experiments section, our benchmarks can be run for arbitrarily large graphs as long as sufficient RAM is available. 
We conducted benchmarking on three machines with 128 GB RAM each, trying to benchmark everything that was possible.
Given that our Docker setup runs all three parties on the same machine, tripling the local RAM requirement, it likely is needed to restrict the graph size in the benchmarks more.
Our experiment scripts as discussed before do so by excluding all experiments using more than 12 GB RAM total.
The tables in Appendix C provide maximal RAM utilization per party, so we multiplied by 3 and filtered out anything expected to then use >12 GB.

We argue that even for reduced graph sizes in the resulting benchmarks, the same scaling behaviour still is perfectly observable.
Furthermore, we provide all original benchmark outputs by our benchmark runs inside evaluation_scripts/raw_benchmark_outputs.
Above, it has been documented how to easily load this data using our provided scripts.

Finally, should the artifact reviewer like to benchmark on larger graphs, we kindly ask them how much available RAM they have.
We are happy to then provide additional benchmark scripts doing all benchmarks possible within this constraint, allowing to reproduce the experiments on larger graphs too, confirming that scalability proceeds as to be expected from the smaller benchmarks.
Note that this will also consume more time.


## Notes on Reusability (Only for Functional and Reproduced badges)
This artifact could be used for many other applications which stems from the fact that is a generic framework for message-passing algorithms in MPC.
While we specifically instantiate three centrality measures, [benchmark/subcircuits.cpp](benchmark/subcircuits.cpp) could be extended to similarly
build circuits to everything else which can be expressed as message-passing algorithm with a linear aggregation function.
We also note that this extends to the area of the prior work [Graphiti](https://eprint.iacr.org/2024/1756) which does not feature a fully working implementation.
