#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "../io/netmp.h"
#include "../utils/circuit.h"
#include "preproc.h"
#include "rand_gen_pool.h"
#include "sharing.h"
#include "../utils/types.h"

using namespace common::utils;

namespace graphsc
{
  class OnlineEvaluator
  {
    int id_;
    RandGenPool rgen_;
    std::shared_ptr<io::NetIOMP> network_;
    PreprocCircuit<Ring> preproc_;
    common::utils::LevelOrderedCircuit circ_;
    std::vector<Ring> wires_;
    std::vector<AddShare<Ring>> q_sh_;
    std::vector<Ring> q_val_;
    std::shared_ptr<ThreadPool> tpool_;

    // write reconstruction function
  public:
    OnlineEvaluator(int id, std::shared_ptr<io::NetIOMP> network,
                    PreprocCircuit<Ring> preproc,
                    common::utils::LevelOrderedCircuit circ,
                    int threads, uint64_t seeds_h[5], uint64_t seeds_l[5]);

    OnlineEvaluator(int id, std::shared_ptr<io::NetIOMP> network,
                    PreprocCircuit<Ring> preproc,
                    common::utils::LevelOrderedCircuit circ,
                    std::shared_ptr<ThreadPool> tpool, uint64_t seeds_h[5], uint64_t seeds_l[5]);

    void setInputs(const std::unordered_map<common::utils::wire_t, Ring> &inputs);

    void evaluateGatesAtDepthPartySend(size_t depth,
                                       std::vector<Ring> &mult_vals, std::vector<Ring> &and_vals, std::vector<Ring> &shuffle_vals, std::vector<Ring> &reveal_vals);

    void evaluateGatesAtDepthPartyRecv(size_t depth,
                                       std::vector<Ring> &mult_vals, std::vector<Ring> &and_vals, std::vector<Ring> &shuffle_vals, std::vector<Ring> &reveal_vals);

    void evaluateGatesAtDepth(size_t depth);

    std::vector<Ring> getOutputs();

    // Reconstruct an authenticated additive shared value
    // combining multiple values might be more effficient
    // CHECK
    // Ring reconstruct(AddShare<Ring> &shares);

    // Evaluate online phase for circuit
    std::vector<Ring> evaluateCircuit(
        const std::unordered_map<common::utils::wire_t, Ring> &inputs);
  };

}; // namespace graphsc
