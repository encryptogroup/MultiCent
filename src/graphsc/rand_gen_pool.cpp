#include "rand_gen_pool.h"

#include <algorithm>

#include "../utils/helpers.h"

namespace graphsc {

  RandGenPool::RandGenPool(int my_id, int num_parties,  uint64_t seed) 
    : id_{my_id}, k_i(num_parties + 1) { 
  auto seed_block = emp::makeBlock(seed, 0); 
  k_self.reseed(&seed_block, 0);
  k_all.reseed(&seed_block, 0);
  k_01.reseed(&seed_block, 0);
  k_02.reseed(&seed_block, 0);
  k_12.reseed(&seed_block, 0);
  for(int i = 1; i <= num_parties; i++) {k_i[i].reseed(&seed_block, 0);}
  }
  //all keys will be the same.  for different keys look at emp toolkit

emp::PRG& RandGenPool::self() { return k_self; }

emp::PRG& RandGenPool::all() { return k_all; }

emp::PRG& RandGenPool::p01() { return k_01; }
emp::PRG& RandGenPool::p02() { return k_02; }
emp::PRG& RandGenPool::p12() { return k_12; }

emp::PRG& RandGenPool::pi( int i) {
  return k_i[i];
}
}  // namespace asterisk
