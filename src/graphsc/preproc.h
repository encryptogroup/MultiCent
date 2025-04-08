#pragma once

#include "../utils/circuit.h"
#include "sharing.h"
#include "../utils/types.h"

using namespace common::utils;

namespace graphsc {
// Preprocessed data for a gate.
template <class R>
struct PreprocGate {

  PreprocGate() = default;


  virtual ~PreprocGate() = default;
};


template <class R>
using preprocg_ptr_t = std::unique_ptr<PreprocGate<R>>;

template <class R>
struct PreprocInput : public PreprocGate<R> {
  // ID of party providing input on wire.
  int pid{};
  // party with id 'pid'.

  PreprocInput() = default;
  PreprocInput(int pid) 
      : PreprocGate<R>(), pid(pid) {}
};

template <class R>
struct PreprocMultGate : public PreprocGate<R> {
  // Secret shared product of inputs masks.
  AddShare<R> triple_a{};
  AddShare<R> triple_b{};
  AddShare<R> triple_c{};


  PreprocMultGate() = default;
  PreprocMultGate(const AddShare<R>& triple_a, const AddShare<R>& triple_b, const AddShare<R>& triple_c)
      : PreprocGate<R>(), triple_a(triple_a), triple_b(triple_b), triple_c(triple_c) {}
};

template <class R>
struct PreprocShuffleGate : public PreprocGate<R> {
  // Secret shared product of inputs masks.
  std::vector<Ring> b_0, b_1, mask_0, mask_1;
  std::shared_ptr<std::vector<Ring>> pi_0, pi_1, rho_0, rho_1; // Same permutation might used by multiple invocations of same shuffle

  PreprocShuffleGate() = default;
  PreprocShuffleGate(const std::shared_ptr<std::vector<Ring>>& pi_0, const std::shared_ptr<std::vector<Ring>>& pi_1, const std::shared_ptr<std::vector<Ring>>& rho_0, const std::shared_ptr<std::vector<Ring>>& rho_1, const std::vector<Ring>& b_0, const std::vector<Ring>& b_1, const std::vector<Ring>& mask_0, const std::vector<Ring>& mask_1)
      : PreprocGate<R>(), pi_0(pi_0), pi_1(pi_1), rho_0(rho_0), rho_1(rho_1), b_0(b_0), b_1(b_1), mask_0(mask_0), mask_1(mask_1) {}
};

template <class R>
struct PreprocGenCompactionGate : public PreprocGate<R> {
  // Secret shared product of inputs masks.
  std::vector<AddShare<R>> triple_a, triple_b, triple_c;

  PreprocGenCompactionGate() = default;
  PreprocGenCompactionGate(const std::vector<AddShare<R>>& triple_a, const  std::vector<AddShare<R>>& triple_b, const  std::vector<AddShare<R>>& triple_c)
      : PreprocGate<R>(), triple_a(triple_a), triple_b(triple_b), triple_c(triple_c) {}
};



// Preprocessed data for the circuit.
template <class R>
struct PreprocCircuit {
  std::vector<preprocg_ptr_t<R>> gates;

  PreprocCircuit() = default;
  PreprocCircuit(size_t num_gates)
      : gates(num_gates) {}
        
};


} // namespace graphsc