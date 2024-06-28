#pragma once
#include <emp-tool/emp-tool.h>

#include <vector>
#include <algorithm>
#include "../utils/helpers.h"

// #include "../utils/helpers.h"

using namespace common::utils;

namespace graphsc {

// Collection of PRGs.
class RandGenPool {
  int id_;

  emp::PRG k_self;
  emp::PRG k_all;
  emp::PRG k_01;
  emp::PRG k_02;
  emp::PRG k_12;
  std::vector<emp::PRG> k_i;
  

 public:
  explicit RandGenPool(int my_id, int num_parties, uint64_t seed = 200);
  
  emp::PRG& self();// { return k_self; }
  emp::PRG& all();//{ return k_all; }
  emp::PRG& p01();// { return k_p0; }
  emp::PRG& p02();// { return k_p0; }
  emp::PRG& p12();// { return k_p0; }
  emp::PRG& pi( int i);
};

};  // namespace asterisk
