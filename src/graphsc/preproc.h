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
  std::vector<Ring> a;
  std::vector<Ring> pi_ab;
  std::vector<int> pi;
  std::vector<int> pi_;


  PreprocShuffleGate() = default;
  PreprocShuffleGate(const std::vector<Ring>& a, const std::vector<Ring>& pi_ab, const std::vector<int>& pi, const std::vector<int>& pi_)
      : PreprocGate<R>(), a(a), pi_ab(pi_ab), pi(pi), pi_(pi_) {}
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