#pragma once

#include <emp-tool/emp-tool.h>

#include <array>
#include <vector>

#include "../utils/helpers.h"
#include "../utils/types.h"

using namespace common::utils;

namespace graphsc {

template <class R>
class AddShare {
  // key_sh is the additive share of the key used for the MAC
  // value_ will be additive share of my_id and tag_ will be the additive share of of the tag for my_id.
  R value_;
  
 public:
  AddShare() = default;
  explicit AddShare(R value)
      : value_{value} {}

  void randomize(emp::PRG& prg) {
    prg.random_data(value_.data(),  sizeof(R));
  }

  R& valueAt() { return value_; }

  void pushValue(R val) { value_ = val; } 
  
  R valueAt() const { return value_; }

  // Arithmetic operators.
  AddShare<R>& operator+=(const AddShare<R>& rhs) {
    value_ += rhs.value_;
    return *this;
  }

  // what is "friend"?
  friend AddShare<R> operator+(AddShare<R> lhs,
                                      const AddShare<R>& rhs) {
    lhs += rhs;
    return lhs;
  }

  AddShare<R>& operator-=(const AddShare<R>& rhs) {
    (*this) += (rhs * R(-1));
    return *this;
  }

  friend AddShare<R> operator-(AddShare<R> lhs,
                                      const AddShare<R>& rhs) {
    lhs -= rhs;
    return lhs;
  }

  AddShare<R>& operator*=(const R& rhs) {
    value_ *= rhs;
    return *this;
  }

  friend AddShare<R> operator*(AddShare<R> lhs, const R& rhs) {
    lhs *= rhs;
    return lhs;
  }

  AddShare<R>& operator<<=(const int& rhs) {
    uint64_t value = conv<uint64_t>(value_);
    value <<= rhs;
    value_ = value;
    return *this;
  }

  friend AddShare<R> operator<<(AddShare<R> lhs, const int& rhs) {
    lhs <<= rhs;
    return lhs;
  }

  AddShare<R>& operator>>=(const int& rhs) {
    uint64_t value = conv<uint64_t>(value_);
    value >>= rhs;
    value_ = value;
    return *this;
  }

  friend AddShare<R> operator>>(AddShare<R> lhs, const int& rhs) {
    lhs >>= rhs;
    return lhs;
  }

  AddShare<R>& add(R val, int pid) {
    if (pid == 1) {
      value_ += val;
    } 

    return *this;
  }

  AddShare<R>& addWithAdder(R val, int pid, int adder) {
    if (pid == adder) {
      value_ += val;
    } 

    return *this;
  }

  AddShare<R>& shift() {
    auto bits = bitDecomposeTwo(value_);
    if (bits[63] == 1)
      value_ = 1;
    else
      value_ = 0;

    return *this;
  }
  
};

template <>
void AddShare<BoolRing>::randomize(emp::PRG& prg);
//add the constructor above

}