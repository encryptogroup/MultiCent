#include "offline_evaluator.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <thread>

namespace graphsc {
OfflineEvaluator::OfflineEvaluator(int my_id,
                                   std::shared_ptr<io::NetIOMP> network,
                                   common::utils::LevelOrderedCircuit circ,
                                   int threads, int seed)
    : id_(my_id),
      rgen_(my_id, seed), 
      network_(std::move(network)),
      circ_(std::move(circ)),
      preproc_(circ.num_gates)

      {tpool_ = std::make_shared<ThreadPool>(threads);}

void OfflineEvaluator::randomShare( int pid,
                                    RandGenPool& rgen,
                                    AddShare<Ring>& share) {
  
    Ring val1;
    Ring val2;

    if(pid == 0){
      rgen.p01().random_data(&val1, sizeof(Ring));
      rgen.p02().random_data(&val2, sizeof(Ring));
      share.pushValue(val1+val2);
    }
    else if(pid == 1){
      rgen.p01().random_data(&val1, sizeof(Ring));
      share.pushValue(val1);

    }
    else{
      rgen.p02().random_data(&val2, sizeof(Ring));
      share.pushValue(val2);
    } 
}


void OfflineEvaluator::randomShareSecret( int pid, 
                                          RandGenPool& rgen,
                                          io::NetIOMP& network,
                                          AddShare<Ring>& share,
                                          Ring secret, 
                                          std::vector<Ring>& rand_sh_sec, 
                                          size_t& idx_rand_sh_sec) {
  Ring val1;
  Ring val2;
  
  if(pid == 0) {
    rgen.p01().random_data(&val1, sizeof(Ring));
    val2 = secret - val1;
    share.pushValue(secret);
    rand_sh_sec.push_back(val2);
  }
  else if(pid == 1) {
    rgen.p01().random_data(&val1, sizeof(Ring));
    share.pushValue(val1);
  }
  else{
    val2 = rand_sh_sec[idx_rand_sh_sec];
    idx_rand_sh_sec++;
    share.pushValue(val2);
  }
  // std::cout << "randomShareSecret ends" << std::endl;
}


void OfflineEvaluator::setWireMasksParty(const std::unordered_map<common::utils::wire_t, int>& input_pid_map, 
                    std::vector<Ring>& rand_sh_sec, std::vector<BoolRing>& b_rand_sh_sec,
                    std::vector<Ring>& rand_sh_party, std::vector<BoolRing>& b_rand_sh_party,
                    int vec_size) {

      size_t idx_rand_sh_sec = 0;
      size_t b_idx_rand_sh_sec = 0;
    
      size_t idx_rand_sh_party = 0;
      size_t b_idx_rand_sh_party = 0;


    for (const auto& level : circ_.gates_by_level) {
    for (const auto& gate : level) {
      switch (gate->type) {

        case common::utils::GateType::kMul: {
          preproc_.gates[gate->out] = std::make_unique<PreprocMultGate<Ring>>();
          

          AddShare<Ring> triple_a;
          AddShare<Ring> triple_b;
          AddShare<Ring> triple_c;
          randomShare(id_, rgen_, triple_a);
          randomShare(id_, rgen_, triple_b);
          
          Ring c =  triple_a.valueAt()*triple_b.valueAt();
          

 
          randomShareSecret(id_, rgen_, *network_, triple_c, c, rand_sh_sec, idx_rand_sh_sec);

          preproc_.gates[gate->out] = std::move(std::make_unique<PreprocMultGate<Ring>>
                              (triple_a, triple_b, triple_c));
          
          break;
        }

        case common::utils::GateType::kShuffle: {
          preproc_.gates[gate->out] = std::make_unique<PreprocShuffleGate<Ring>>();


          std::vector<Ring> a(vec_size);
          std::vector<Ring> pi_ab(vec_size);
          std::vector<int> pi(vec_size);
          std::vector<int> pi_(vec_size);


          // all permutations are identity permutations
          // (todo: change to random permutations)

          for(int i = 0; i<vec_size; i++){
            pi[i] = i;
            pi_[i] = i;
            a[i] = 0;
            pi_ab[i] = 0;
          }          

          preproc_.gates[gate->out] = std::move(std::make_unique<PreprocShuffleGate<Ring>>
                              (a, pi_ab, pi, pi_));
          
          break;
        }


        default: {
          break;
        }
      }
    }
  }
}


void OfflineEvaluator::setWireMasks(
    const std::unordered_map<common::utils::wire_t, int>& input_pid_map, int vec_size) {
      
    std::vector<Ring> rand_sh_sec;
    std::vector<BoolRing> b_rand_sh_sec;
    std::vector<Ring> rand_sh_party;
    std::vector<BoolRing> b_rand_sh_party;

      
  if(id_ != 2) {
    std::cout << "--- here6 ---\n";
    setWireMasksParty(input_pid_map, rand_sh_sec, b_rand_sh_sec,
                        rand_sh_party, b_rand_sh_party, vec_size);
    
    std::cout << "--- here17 ---\n";
  
    if(id_ == 0) {
      size_t rand_sh_sec_num = rand_sh_sec.size();
      size_t b_rand_sh_sec_num = b_rand_sh_sec.size();
      size_t rand_sh_party_num = rand_sh_party.size();
      size_t b_rand_sh_party_num = b_rand_sh_party.size();
      size_t arith_comm = rand_sh_sec_num + rand_sh_party_num;
      size_t bool_comm = b_rand_sh_sec_num + b_rand_sh_party_num;
      std::vector<size_t> lengths(6);

      lengths[0] = arith_comm;
      lengths[1] = rand_sh_sec_num;
      lengths[2] = rand_sh_party_num;
      lengths[3] = bool_comm;
      lengths[4] = b_rand_sh_sec_num;
      lengths[5] = b_rand_sh_party_num;


      network_->send(2, lengths.data(), sizeof(size_t) * 6);

      std::vector<Ring> offline_arith_comm(arith_comm);
      std::vector<BoolRing> offline_bool_comm(bool_comm);
      for(size_t i = 0; i < rand_sh_sec_num; i++) {
        offline_arith_comm[i] = rand_sh_sec[i];
      }
      for(size_t i = 0; i < rand_sh_party_num; i++) {
        offline_arith_comm[rand_sh_sec_num + i] = rand_sh_party[i];
      }
      for(size_t i = 0; i < b_rand_sh_sec_num; i++) {
        offline_bool_comm[i] = b_rand_sh_sec[i];
      }
      for(size_t i = 0; i < b_rand_sh_party_num; i++) {
        offline_bool_comm[b_rand_sh_sec_num + i] = b_rand_sh_party[i];
      }

      auto net_data = BoolRing::pack(offline_bool_comm.data(), bool_comm);
      network_->send(2, offline_arith_comm.data(), sizeof(Ring) * arith_comm);
      network_->send(2, net_data.data(), sizeof(uint8_t) * net_data.size());

      // network_->send(nP_, offline_bool_comm.data(), sizeof(BoolRing) * bool_comm);
    
    }
  }
  else if(id_ == 2 ) {
    std::vector<size_t> lengths(6);
    network_->recv(0, lengths.data(), sizeof(size_t) * 6);
    size_t arith_comm = lengths[0];
    size_t rand_sh_sec_num = lengths[1];
    size_t rand_sh_party_num = lengths[2];
    size_t bool_comm = lengths[3];
    size_t b_rand_sh_sec_num = lengths[4];
    size_t b_rand_sh_party_num = lengths[5];

    std::vector<Ring> offline_arith_comm(arith_comm);
    network_->recv(0, offline_arith_comm.data(), sizeof(Ring) * arith_comm);
    size_t nbytes = (bool_comm + 7) / 8;
    std::vector<uint8_t> net_data(nbytes);
    network_->recv(0, net_data.data(), nbytes * sizeof(uint8_t));
    auto offline_bool_comm = BoolRing::unpack(net_data.data(), bool_comm);
    // std::vector<BoolRing> offline_bool_comm(bool_comm);
    // network_->recv(0, offline_bool_comm.data(), sizeof(BoolRing) * bool_comm);
    
    
    rand_sh_sec.resize(rand_sh_sec_num);
    for(int i = 0; i < rand_sh_sec_num; i++) {
      rand_sh_sec[i] = offline_arith_comm[i];
    }
    
    rand_sh_party.resize(rand_sh_party_num);
    for(int i = 0; i < rand_sh_party_num; i++) {
      rand_sh_party[i] = offline_arith_comm[rand_sh_sec_num + i];
    }
    
    b_rand_sh_sec.resize(b_rand_sh_sec_num);
    for(int i = 0; i < b_rand_sh_sec_num; i++) {
      b_rand_sh_sec[i] = offline_bool_comm[i];
    }
    
    b_rand_sh_party.resize(b_rand_sh_party_num);
    for(int i = 0; i < b_rand_sh_party_num; i++) {
      b_rand_sh_party[i] = offline_bool_comm[b_rand_sh_sec_num + i];
    }
    
    setWireMasksParty(input_pid_map, rand_sh_sec, b_rand_sh_sec,
                        rand_sh_party, b_rand_sh_party, vec_size);
  }
  
}




PreprocCircuit<Ring> OfflineEvaluator::getPreproc() {
  return std::move(preproc_);
}
  
PreprocCircuit<Ring> OfflineEvaluator::run(
    const std::unordered_map<common::utils::wire_t, int>& input_pid_map, int vec_size) {
  
  setWireMasks(input_pid_map, vec_size);

  return std::move(preproc_);
  
}


};