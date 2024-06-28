#pragma once

#include <emp-tool/emp-tool.h>

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>

#include "../io/netmp.h"
#include "../utils/circuit.h"


#include "preproc.h"
#include "graphsc/rand_gen_pool.h"
#include "sharing.h"
#include "../utils/types.h"

using namespace common::utils;

namespace graphsc {

class OfflineEvaluator {
    int id_;
    RandGenPool rgen_;
    std::shared_ptr<io::NetIOMP> network_;
    common::utils::LevelOrderedCircuit circ_;
    std::shared_ptr<ThreadPool> tpool_;
    PreprocCircuit<Ring> preproc_;

     public:
  
    OfflineEvaluator( int my_id, std::shared_ptr<io::NetIOMP> network,
                   common::utils::LevelOrderedCircuit circ,
                   int threads, int seed = 200);

    // Generate sharing of a random unknown value.
    static void randomShare(int pid, RandGenPool& rgen,
                            AddShare<Ring>& share);


    // Generate sharing of a secret known to one party
    static void randomShareSecret(  int pid, RandGenPool& rgen, io::NetIOMP& network,
                                    AddShare<Ring>& share, Ring secret,
                                    std::vector<Ring>& rand_sh_sec, size_t& idx_rand_sh_sec);


    void setWireMasksParty(const std::unordered_map<common::utils::wire_t, int>& input_pid_map, 
          std::vector<Ring>& rand_sh_sec, std::vector<BoolRing>& b_rand_sh_sec,
          std::vector<Ring>& rand_sh_party, std::vector<BoolRing>& b_rand_sh_party,
          int vec_size);

    void setWireMasks(const std::unordered_map<common::utils::wire_t, int>& input_pid_map, int vec_size);


    PreprocCircuit<Ring> getPreproc();

    // Efficiently runs above subprotocols.
    PreprocCircuit<Ring> run(
      const std::unordered_map<common::utils::wire_t, int>& input_pid_map, int vec_size=0);

        
};


};

