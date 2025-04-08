#include "rand_gen_pool.h"

#include <algorithm>

#include "../utils/helpers.h"

namespace graphsc {

  RandGenPool::RandGenPool(int my_id, int num_parties, uint64_t seeds_high[5], uint64_t seeds_low[5])
    : id_{my_id} { 
  auto seed_block = emp::makeBlock(seeds_high[0], seeds_low[0]); 
  k_self.reseed(&seed_block, 0);
  seed_block = emp::makeBlock(seeds_high[1], seeds_low[1]); 
  k_all.reseed(&seed_block, 0);
  seed_block = emp::makeBlock(seeds_high[2], seeds_low[2]); 
  k_01.reseed(&seed_block, 0);
  seed_block = emp::makeBlock(seeds_high[3], seeds_low[3]); 
  k_02.reseed(&seed_block, 0);
  seed_block = emp::makeBlock(seeds_high[4], seeds_low[4]); 
  k_12.reseed(&seed_block, 0);
}

emp::PRG& RandGenPool::self() { return k_self; }
emp::PRG& RandGenPool::all() { return k_all; }

emp::PRG& RandGenPool::p01() { return k_01; }
emp::PRG& RandGenPool::p02() { return k_02; }
emp::PRG& RandGenPool::p12() { return k_12; }

}  // namespace asterisk
