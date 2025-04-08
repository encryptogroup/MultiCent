#include "offline_evaluator.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <thread>

const bool SHUFFLE_VERBOSE = false;

namespace graphsc {
OfflineEvaluator::OfflineEvaluator(int my_id,
                                   std::shared_ptr<io::NetIOMP> network,
                                   common::utils::LevelOrderedCircuit circ,
                                   int threads, uint64_t seeds_h[5], uint64_t seeds_l[5])
    : id_(my_id),
      rgen_(my_id, 3, seeds_h, seeds_l), 
      network_(std::move(network)),
      circ_(std::move(circ)),
      preproc_(circ.num_gates)

      {
        tpool_ = std::make_shared<ThreadPool>(threads);
        pis_0 = std::vector<std::shared_ptr<std::vector<Ring>>> ();
        pis_1 = std::vector<std::shared_ptr<std::vector<Ring>>> ();
        rhos_0 = std::vector<std::shared_ptr<std::vector<Ring>>> ();
        rhos_1 = std::vector<std::shared_ptr<std::vector<Ring>>> ();
      }

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

void OfflineEvaluator::randomShareBin( int pid,
                                    RandGenPool& rgen,
                                    AddShare<Ring>& share) {
  
    Ring val1;
    Ring val2;

    if(pid == 0){
      rgen.p01().random_data(&val1, sizeof(Ring));
      rgen.p02().random_data(&val2, sizeof(Ring));
      share.pushValue(val1 ^ val2);
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
}

void OfflineEvaluator::randomShareSecretBin( int pid, 
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
    val2 = secret ^ val1;
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
}


void OfflineEvaluator::setWireMasksParty(const std::unordered_map<common::utils::wire_t, int>& input_pid_map, 
                    std::vector<Ring>& rand_sh_sec, std::vector<Ring>& rand_sh_sec_to_1, std::vector<BoolRing>& b_rand_sh_sec,
                    std::vector<Ring>& rand_sh_party, std::vector<BoolRing>& b_rand_sh_party) {

      size_t idx_rand_sh_sec = 0;
      size_t idx_rand_sh_sec_to_1 = 0;
      size_t b_idx_rand_sh_sec = 0;
    
      size_t idx_rand_sh_party = 0;
      size_t b_idx_rand_sh_party = 0;


    for (const auto& level : circ_.gates_by_level) {
    for (const auto& gate : level) {
      switch (gate->type) {

        case common::utils::GateType::kMul:
        case common::utils::GateType::kConvertB2A: {
          preproc_.gates[gate->gid] = std::make_unique<PreprocMultGate<Ring>>();
          

          AddShare<Ring> triple_a;
          AddShare<Ring> triple_b;
          AddShare<Ring> triple_c;
          randomShare(id_, rgen_, triple_a);
          randomShare(id_, rgen_, triple_b);
          
          Ring c =  triple_a.valueAt()*triple_b.valueAt();
          

 
          randomShareSecret(id_, rgen_, *network_, triple_c, c, rand_sh_sec, idx_rand_sh_sec);

          if (id_ != 0)
            preproc_.gates[gate->gid] = std::move(std::make_unique<PreprocMultGate<Ring>>
                              (triple_a, triple_b, triple_c));
          
          break;
        }

        case common::utils::GateType::kAnd:
        case common::utils::GateType::kEqualsZero: {
          preproc_.gates[gate->gid] = std::make_unique<PreprocMultGate<Ring>>();
          

          AddShare<Ring> triple_a;
          AddShare<Ring> triple_b;
          AddShare<Ring> triple_c;
          randomShareBin(id_, rgen_, triple_a);
          randomShareBin(id_, rgen_, triple_b);
          
          Ring c =  triple_a.valueAt() & triple_b.valueAt();
          

 
          randomShareSecretBin(id_, rgen_, *network_, triple_c, c, rand_sh_sec, idx_rand_sh_sec);

          if (id_ != 0)
            preproc_.gates[gate->gid] = std::move(std::make_unique<PreprocMultGate<Ring>>
                              (triple_a, triple_b, triple_c));
          
          break;
        }

        case common::utils::GateType::kGenCompaction: {
          auto *g = static_cast<common::utils::SIMDOGate *>(gate.get());
          preproc_.gates[gate->gid] = std::make_unique<PreprocGenCompactionGate<Ring>>();
          

          std::vector<AddShare<Ring>> triple_a;
          std::vector<AddShare<Ring>> triple_b;
          std::vector<AddShare<Ring>> triple_c;
          for (int j = 0; j < g->in1.size(); j++) {
            triple_a.push_back(AddShare<Ring>());
            triple_b.push_back(AddShare<Ring>());
            randomShare(id_, rgen_, triple_a[j]);
            randomShare(id_, rgen_, triple_b[j]);

            triple_c.push_back(AddShare<Ring>());
            Ring c = triple_a[j].valueAt() * triple_b[j].valueAt();
            randomShareSecret(id_, rgen_, *network_, triple_c[j], c, rand_sh_sec, idx_rand_sh_sec);
          }

          if (id_ != 0)
            preproc_.gates[gate->gid] = std::move(std::make_unique<PreprocGenCompactionGate<Ring>>
                              (triple_a, triple_b, triple_c));
          
          break;
        }

        case common::utils::GateType::kShuffle: {
          auto *g = static_cast<common::utils::ParamWithFlagSIMDOGate *>(gate.get());
          bool reverse = g->flag;
          if (reverse && SHUFFLE_VERBOSE) {
            std::cout << "Running in reverse mode" << std::endl;
          }

          preproc_.gates[gate->gid] = std::make_unique<PreprocShuffleGate<Ring>>();

          bool newPerm;
          if (g->param < pis_0.size()) {
            if (pis_0[g->param]->size() == 0 && pis_1[g->param]->size() == 0) {
              // Was added as a dummy element
              // need to check both as online parties only have one filled
              if (SHUFFLE_VERBOSE)
                std::cout << "Generating new permutation (prior filler)" << std::endl;
              newPerm = true;
            } else {
              if (SHUFFLE_VERBOSE)
                std::cout << "Reusing permutation" << std::endl;
              newPerm = false;
            }
          } else {
            // Problem: Due to automatic structuring to layers, maybe some higher ID occurs
            // before a smaller one does, so already generate dummy elements in between
            if (SHUFFLE_VERBOSE)
              std::cout << "Generating new permutation" << std::endl;
            while (g->param + 1 != pis_0.size()) {
              pis_0.push_back(std::shared_ptr<std::vector<Ring>>(new std::vector<Ring>()));
              pis_1.push_back(std::shared_ptr<std::vector<Ring>>(new std::vector<Ring>()));
              rhos_0.push_back(std::shared_ptr<std::vector<Ring>>(new std::vector<Ring>()));
              rhos_1.push_back(std::shared_ptr<std::vector<Ring>>(new std::vector<Ring>()));
              if (SHUFFLE_VERBOSE)
                std::cout << "added new permutations" << std::endl;
            }
            newPerm = true;
          }

          std::vector<Ring>& pi_0 = *pis_0[g->param];
          std::vector<Ring>& pi_1 = *pis_1[g->param];
          std::vector<Ring>& rho_0 = *rhos_0[g->param];
          std::vector<Ring>& rho_1 = *rhos_1[g->param];

          std::vector<Ring> b_0, b_1;

          if (newPerm) { // can skip this if old permutation is reused
            // Generate pi_0, pi_1
            for (int i = 0; i < 2; i++) {
              if ((i == 0 && id_ == 2) || (i == 1 && id_ == 1))
                continue;
              auto& perm = i == 1 ? pi_1 : pi_0;
              for (int j = 0; j < g->in1.size(); j++)
                perm.push_back(j);
              for (int j = 0; j < g->in1.size(); j++) {
                std::size_t k;
                if (i == 0)
                  rgen_.p01().random_data(&k, sizeof(std::size_t));
                else
                  rgen_.p02().random_data(&k, sizeof(std::size_t));
                k = k % (g->in1.size() - j);
                std::swap(perm[j], perm[k + j]);
              }
            }

            // Generate pi'_0
            if (id_ != 2) {
              auto& perm = rho_0;
              for (int j = 0; j < g->in1.size(); j++)
                perm.push_back(j);
              for (int j = 0; j < g->in1.size(); j++) {
                std::size_t k;
                rgen_.p01().random_data(&k, sizeof(std::size_t));
                k = k % (g->in1.size() - j);
                std::swap(perm[j], perm[k + j]);
              }
            }

            // Compute and send pi'_1 s.t. pi'_1 * pi'_0 = pi_0 * pi_1
            if (id_ == 0) {
              // Compute pi'_0^(-1)
              std::vector<int> inverse(g->in1.size());
              for (int j = 0; j < g->in1.size(); j++) {
                inverse[rho_0[j]] = j;
              }
              // Compute pi'_1 = pi_0 * pi_1 * pi'_0^(-1)
              auto& perm = rho_1;
              for (int j = 0; j < g->in1.size(); j++) {
                perm.push_back(pi_0[pi_1[inverse[j]]]);
              }

              for (int j = 0; j < g->in1.size(); j++) {
                rand_sh_sec.push_back((Ring) perm[j]);
              }
            } else if (id_ == 2) { // Receive pi'_1
              auto& perm = rho_1;
              for (int j = 0; j < g->in1.size(); j++) {
                perm.push_back(rand_sh_sec[idx_rand_sh_sec]);
                idx_rand_sh_sec++;
              }
            }
          }

          if (SHUFFLE_VERBOSE) {
            std::cout << "pi_0" << std::endl;
            if (id_ == 2) {
              std::cout << "not available" << std::endl;
            } else {
              for (int j = 0; j < g->in1.size(); j++) {
                std::cout << pi_0[j] << " ";
              }
              std::cout << std::endl;
            }
            std::cout << "pi_1" << std::endl;
            if (id_ == 1) {
              std::cout << "not available" << std::endl;
            } else {
              for (int j = 0; j < g->in1.size(); j++) {
                std::cout << pi_1[j] << " ";
              }
              std::cout << std::endl;
            }
            std::cout << "pi'_0" << std::endl;
            if (id_ == 2) {
              std::cout << "not available" << std::endl;
            } else {
              for (int j = 0; j < g->in1.size(); j++) {
                std::cout << rho_0[j] << " ";
              }
              std::cout << std::endl;
            }
            std::cout << "pi'_1" << std::endl;
            if (id_ == 1) {
              std::cout << "not available" << std::endl;
            } else {
              for (int j = 0; j < g->in1.size(); j++) {
                std::cout << rho_1[j] << " ";
              }
              std::cout << std::endl;
            }
          }

          // Sample R_0, R_1
          std::vector<Ring> mask_0, mask_1;
          if (id_ != 2) {
            for (int j = 0; j < g->in1.size(); j++) {
              Ring m;
              rgen_.p01().random_data(&m, sizeof(Ring));
              mask_0.push_back(m);
            }
          }
          if (id_ != 1) {
            for (int j = 0; j < g->in1.size(); j++) {
              Ring m;
              rgen_.p02().random_data(&m, sizeof(Ring));
              mask_1.push_back(m);
            }
          }

          // Compute B_0, B_1
          if (id_ == 0) {
            b_0.resize(g->in1.size());
            b_1.resize(g->in1.size());
            for (size_t j = 0; j < g->in1.size(); j++) {
              Ring randomizer;
              rgen_.self().random_data(&randomizer, sizeof(Ring));
              if (reverse) {
                  // B_i = pi^(-1)(R_i) +/- R
                  // pi^(-1) = (pi_0 * pi_1)^(-1) = (shuffle[0] * shuffle[2])^(-1)
                  b_0[j] = mask_0[pi_0[pi_1[j]]] - randomizer;
                  b_1[j] = mask_1[pi_0[pi_1[j]]] + randomizer;
              } else {
                  // B_i = pi(R_i) +/- R
                  // pi = pi_0 * pi_1 = shuffle[0] * shuffle[2]
                  b_0[pi_0[pi_1[j]]] = mask_0[j] - randomizer;
                  b_1[pi_0[pi_1[j]]] = mask_1[j] + randomizer;
              }
            }

            for (int j = 0; j < g->in1.size(); j++) {
              rand_sh_sec_to_1.push_back(b_0[j]);
              rand_sh_sec.push_back(b_1[j]);
            }
          } else if (id_ == 1) {
            for (int j = 0; j < g->in1.size(); j++) {
              b_0.push_back(rand_sh_sec_to_1[idx_rand_sh_sec_to_1]);
              idx_rand_sh_sec_to_1++;
            }
          } else {
            for (int j = 0; j < g->in1.size(); j++) {
              b_1.push_back(rand_sh_sec[idx_rand_sh_sec]);
              idx_rand_sh_sec++;
            }
          }

          if (id_ != 0)
            preproc_.gates[gate->gid] = std::move(std::make_unique<PreprocShuffleGate<Ring>>
                              (pis_0[g->param], pis_1[g->param], rhos_0[g->param], rhos_1[g->param], b_0, b_1, mask_0, mask_1));
          
          break;
        }

        case common::utils::GateType::kDoubleShuffle: {
          auto *g = static_cast<common::utils::ThreeParamSIMDOGate *>(gate.get());

          preproc_.gates[gate->gid] = std::make_unique<PreprocShuffleGate<Ring>>(); // can use standard and just set permutations differently

          bool newPerm;
          if (g->param1 < pis_0.size()) {
            if (pis_0[g->param1]->size() == 0 && pis_1[g->param1]->size() == 0) {
              // Was added as a dummy element
              // need to check both as online parties only have one filled
              if (SHUFFLE_VERBOSE)
                std::cout << "Generating new permutation (prior filler)" << std::endl;
              newPerm = true;
            } else {
              if (SHUFFLE_VERBOSE)
                std::cout << "Reusing permutation" << std::endl;
              newPerm = false;
            }
          } else {
            // Problem: Due to automatic structuring to layers, maybe some higher ID occurs
            // before a smaller one does, so already generate dummy elements in between
            if (SHUFFLE_VERBOSE)
              std::cout << "Generating new permutation" << std::endl;
            while (g->param1 + 1 != pis_0.size()) {
              pis_0.push_back(std::shared_ptr<std::vector<Ring>>(new std::vector<Ring>()));
              pis_1.push_back(std::shared_ptr<std::vector<Ring>>(new std::vector<Ring>()));
              rhos_0.push_back(std::shared_ptr<std::vector<Ring>>(new std::vector<Ring>()));
              rhos_1.push_back(std::shared_ptr<std::vector<Ring>>(new std::vector<Ring>()));
              if (SHUFFLE_VERBOSE)
                std::cout << "added new permutations" << std::endl;
            }
            newPerm = true;
          }

          if (newPerm) {
            if (g->param2 >= pis_0.size() || g->param3 >= pis_0.size() || 
                  (pis_0[g->param2]->size() == 0 && pis_1[g->param2]->size() == 0) ||
                  (pis_0[g->param3]->size() == 0 && pis_1[g->param3]->size() == 0)) {
              throw std::runtime_error("DoubleShuffle can only be prepared AFTER both underlying shuffles have been prepared in the layered circuit");
            }
          }
          

          std::vector<Ring>& pi_0 = *pis_0[g->param1];
          std::vector<Ring>& pi_1 = *pis_1[g->param1];
          std::vector<Ring>& rho_0 = *rhos_0[g->param1];
          std::vector<Ring>& rho_1 = *rhos_1[g->param1];

          std::vector<Ring> b_0, b_1;

          if (newPerm) { // can skip this if old permutation is reused
            // Generate pi_0
            if (id_ != 2) {
              for (int j = 0; j < g->in1.size(); j++)
                pi_0.push_back(j);
              for (int j = 0; j < g->in1.size(); j++) {
                std::size_t k;
                rgen_.p01().random_data(&k, sizeof(std::size_t));
                k = k % (g->in1.size() - j);
                std::swap(pi_0[j], pi_0[k + j]);
              }
            }

            // Generate rho_1 // load balancing
            if (id_ != 1) {
              for (int j = 0; j < g->in1.size(); j++)
                rho_1.push_back(j);
              for (int j = 0; j < g->in1.size(); j++) {
                std::size_t k;
                rgen_.p02().random_data(&k, sizeof(std::size_t));
                k = k % (g->in1.size() - j);
                std::swap(rho_1[j], rho_1[k + j]);
              }
            }

            // There are two underlying permutations: pi2_0 * pi2_1 = rho2_1 * rho2_0
            // and pi3_0 * pi3_1 = rho3_1 * rho3_0, specified by param2 and param3.
            // Our goal is to permute by the first in reverse order and then the second in forward order,
            // i.e., we wish to use pi3_0 * pi3_1 * pi2_1^(-1) * pi2_0^(-1) = pi_0 * pi_1 = rho_1 * rho_0.
            // We already have pi_0 and rho_1, so we need the dealer to compute and send
            // pi_1 = pi_0^(-1) * pi3_0 * pi3_1 * pi2_1^(-1) * pi2_0^(-1)
            // rho_0 = rho_1^(-1) * pi_0 * pi_1

            if (id_ == 0) {
              std::vector<Ring>& pi2_0 = *pis_0[g->param2];
              std::vector<Ring>& pi2_1 = *pis_1[g->param2];
              std::vector<Ring>& pi3_0 = *pis_0[g->param3];
              std::vector<Ring>& pi3_1 = *pis_1[g->param3];

              // pi_1 = pi_0^(-1) * pi3_0 * pi3_1 * pi2_1^(-1) * pi2_0^(-1) = pi_0^(-1) * pi3_0 * pi3_1 * (pi2_0 * pi2_1)^(-1)
              // Compute pi_0^(-1)
              std::vector<int> pi_0_inv(g->in1.size());
              for (int j = 0; j < g->in1.size(); j++) {
                pi_0_inv[pi_0[j]] = j;
              }
              // Compute (pi2_0 * pi2_1)^(-1)
              std::vector<int> pi2_comp_inv(g->in1.size());
              for (int j = 0; j < g->in1.size(); j++) {
                pi2_comp_inv[pi2_0[pi2_1[j]]] = j;
              }
              // Compose all
              for (int j = 0; j < g->in1.size(); j++) {
                pi_1.push_back(pi_0_inv[pi3_0[pi3_1[pi2_comp_inv[j]]]]);
              }
              for (int j = 0; j < g->in1.size(); j++) {
                rand_sh_sec.push_back((Ring) pi_1[j]);
              }

              // rho_0 = rho_1^(-1) * pi_0 * pi_1
              // Compute rho_1^(-1)
              std::vector<int> rho_1_inv(g->in1.size());
              for (int j = 0; j < g->in1.size(); j++) {
                rho_1_inv[rho_1[j]] = j;
              }
              // Compose all
              for (int j = 0; j < g->in1.size(); j++) {
                rho_0.push_back(rho_1_inv[pi_0[pi_1[j]]]);
              }
              for (int j = 0; j < g->in1.size(); j++) {
                rand_sh_sec_to_1.push_back((Ring) rho_0[j]);
              }
            } else if (id_ == 1) { // Receive rho_0
              for (int j = 0; j < g->in1.size(); j++) {
                rho_0.push_back(rand_sh_sec_to_1[idx_rand_sh_sec_to_1]);
                idx_rand_sh_sec_to_1++;
              }
            } else { // Receive pi_1
              for (int j = 0; j < g->in1.size(); j++) {
                pi_1.push_back(rand_sh_sec[idx_rand_sh_sec]);
                idx_rand_sh_sec++;
              }
            }
          }

          // TODO cleanup as from here, practically only duplicate code from standard shuffle
          if (SHUFFLE_VERBOSE) {
            std::cout << "pi_0" << std::endl;
            if (id_ == 2) {
              std::cout << "not available" << std::endl;
            } else {
              for (int j = 0; j < g->in1.size(); j++) {
                std::cout << pi_0[j] << " ";
              }
              std::cout << std::endl;
            }
            std::cout << "pi_1" << std::endl;
            if (id_ == 1) {
              std::cout << "not available" << std::endl;
            } else {
              for (int j = 0; j < g->in1.size(); j++) {
                std::cout << pi_1[j] << " ";
              }
              std::cout << std::endl;
            }
            std::cout << "pi'_0" << std::endl;
            if (id_ == 2) {
              std::cout << "not available" << std::endl;
            } else {
              for (int j = 0; j < g->in1.size(); j++) {
                std::cout << rho_0[j] << " ";
              }
              std::cout << std::endl;
            }
            std::cout << "pi'_1" << std::endl;
            if (id_ == 1) {
              std::cout << "not available" << std::endl;
            } else {
              for (int j = 0; j < g->in1.size(); j++) {
                std::cout << rho_1[j] << " ";
              }
              std::cout << std::endl;
            }
          }

          // Sample R_0, R_1
          std::vector<Ring> mask_0, mask_1;
          if (id_ != 2) {
            for (int j = 0; j < g->in1.size(); j++) {
              Ring m;
              rgen_.p01().random_data(&m, sizeof(Ring));
              mask_0.push_back(m);
            }
          }
          if (id_ != 1) {
            for (int j = 0; j < g->in1.size(); j++) {
              Ring m;
              rgen_.p02().random_data(&m, sizeof(Ring));
              mask_1.push_back(m);
            }
          }

          // Compute B_0, B_1
          if (id_ == 0) {
            b_0.resize(g->in1.size());
            b_1.resize(g->in1.size());
            for (size_t j = 0; j < g->in1.size(); j++) {
              Ring randomizer;
              rgen_.self().random_data(&randomizer, sizeof(Ring));
              // B_i = pi(R_i) +/- R
              // pi = pi_0 * pi_1 = shuffle[0] * shuffle[2]
              b_0[pi_0[pi_1[j]]] = mask_0[j] - randomizer;
              b_1[pi_0[pi_1[j]]] = mask_1[j] + randomizer;
            }

            for (int j = 0; j < g->in1.size(); j++) {
              rand_sh_sec_to_1.push_back(b_0[j]);
              rand_sh_sec.push_back(b_1[j]);
            }
          } else if (id_ == 1) {
            for (int j = 0; j < g->in1.size(); j++) {
              b_0.push_back(rand_sh_sec_to_1[idx_rand_sh_sec_to_1]);
              idx_rand_sh_sec_to_1++;
            }
          } else {
            for (int j = 0; j < g->in1.size(); j++) {
              b_1.push_back(rand_sh_sec[idx_rand_sh_sec]);
              idx_rand_sh_sec++;
            }
          }

          if (id_ != 0)
            preproc_.gates[gate->gid] = std::move(std::make_unique<PreprocShuffleGate<Ring>>
                              (pis_0[g->param1], pis_1[g->param1], rhos_0[g->param1], rhos_1[g->param1], b_0, b_1, mask_0, mask_1));
          
          break;
        }

        case common::utils::GateType::kInp: {
          preproc_.gates[gate->gid] = std::move(std::make_unique<PreprocInput<Ring>>
                              (input_pid_map.at(gate->out)));
          break;
        }

        case common::utils::GateType::kBinInp: {
          preproc_.gates[gate->gid] = std::move(std::make_unique<PreprocInput<Ring>>
                              (input_pid_map.at(gate->out)));
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
    const std::unordered_map<common::utils::wire_t, int>& input_pid_map) {
      
    std::vector<Ring> rand_sh_sec, rand_sh_sec_to_1;
    std::vector<BoolRing> b_rand_sh_sec;
    std::vector<Ring> rand_sh_party;
    std::vector<BoolRing> b_rand_sh_party;

  if (id_ == 0) {
    setWireMasksParty(input_pid_map, rand_sh_sec, rand_sh_sec_to_1, b_rand_sh_sec,
                        rand_sh_party, b_rand_sh_party);
  
    size_t rand_sh_sec_num = rand_sh_sec.size();
    size_t rand_sh_sec_to_1_num = rand_sh_sec_to_1.size();
    size_t b_rand_sh_sec_num = b_rand_sh_sec.size();
    size_t rand_sh_party_num = rand_sh_party.size();
    size_t b_rand_sh_party_num = b_rand_sh_party.size();
    size_t arith_comm = rand_sh_sec_num + rand_sh_party_num;
    size_t arith_comm_to_1 = rand_sh_sec_to_1_num;
    size_t bool_comm = b_rand_sh_sec_num + b_rand_sh_party_num;
    std::vector<size_t> lengths(6);

    lengths[0] = arith_comm;
    lengths[1] = rand_sh_sec_num;
    lengths[2] = rand_sh_party_num;
    lengths[3] = bool_comm;
    lengths[4] = b_rand_sh_sec_num;
    lengths[5] = b_rand_sh_party_num;


    network_->send(2, lengths.data(), sizeof(size_t) * 6);
    network_->send(1, &rand_sh_sec_to_1_num, sizeof(size_t));

    std::vector<Ring> offline_arith_comm(arith_comm);
    std::vector<Ring> offline_arith_comm_to_1(arith_comm);
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
    for(size_t i = 0; i < rand_sh_sec_to_1_num; i++) {
      offline_arith_comm_to_1[i] = rand_sh_sec_to_1[i];
    }

    auto net_data = BoolRing::pack(offline_bool_comm.data(), bool_comm);

    int num_comm = arith_comm/100000000;
    int last_comm = arith_comm%100000000;
    for(int i = 0; i < num_comm; i++){
      std::vector<Ring> data_send_i;
      data_send_i.reserve(100000000);
      for(int j = 0; j<100000000; j++){
        data_send_i[j] = offline_arith_comm[i*100000000 + j];   
      }

      network_->send(2, data_send_i.data(), sizeof(Ring) * 100000000);
    }
                    
    std::vector<Ring> data_send_last;
    data_send_last.reserve(last_comm);
    for(int j = 0; j<last_comm; j++){
      data_send_last[j] = offline_arith_comm[num_comm*100000000 + j];
    }
    network_->send(2, data_send_last.data(), sizeof(Ring) * last_comm);
    // network_->send(2, offline_arith_comm.data(), sizeof(Ring) * arith_comm);

    num_comm = arith_comm_to_1/100000000;
    last_comm = arith_comm_to_1%100000000;
    for(int i = 0; i < num_comm; i++){
      std::vector<Ring> data_send_i;
      data_send_i.reserve(100000000);
      for(int j = 0; j<100000000; j++){
        data_send_i[j] = offline_arith_comm_to_1[i*100000000 + j];   
      }

      network_->send(1, data_send_i.data(), sizeof(Ring) * 100000000);
    }
                    
    std::vector<Ring> data_send_last_to_1;
    data_send_last_to_1.reserve(last_comm);
    for(int j = 0; j<last_comm; j++){
      data_send_last_to_1[j] = offline_arith_comm_to_1[num_comm*100000000 + j];
    }
    network_->send(1, data_send_last_to_1.data(), sizeof(Ring) * last_comm);
    // network_->send(1, offline_arith_comm_to_1.data(), sizeof(Ring) * arith_comm_to_1);

    network_->send(2, net_data.data(), sizeof(uint8_t) * net_data.size());

    // network_->send(nP_, offline_bool_comm.data(), sizeof(BoolRing) * bool_comm);
    
  } else if (id_ == 1) {
    
    size_t arith_comm_to_1;
    network_->recv(0, &arith_comm_to_1, sizeof(size_t));

    int num_comm = arith_comm_to_1/100000000;
    int last_comm = arith_comm_to_1%100000000;
    std::vector<Ring> offline_arith_comm_to_1(arith_comm_to_1);
    for(int i = 0; i < num_comm; i++){
      std::vector<Ring> data_recv_i(100000000);

      network_->recv(0, data_recv_i.data(), sizeof(Ring) * 100000000);
      for(int j = 0; j<100000000; j++){
        offline_arith_comm_to_1[i*100000000 + j] = data_recv_i[j];
      }
    }
                    
    std::vector<Ring> data_recv_last(last_comm);
    network_->recv(0, data_recv_last.data(), sizeof(Ring) * last_comm);
    for(int j = 0; j<last_comm; j++){
      offline_arith_comm_to_1[num_comm*100000000 + j] = data_recv_last[j];
    }

    // network_->recv(0, offline_arith_comm_to_1.data(), sizeof(Ring) * arith_comm_to_1);

    rand_sh_sec_to_1.resize(arith_comm_to_1);
    for(int i = 0; i < arith_comm_to_1; i++) {
      rand_sh_sec_to_1[i] = offline_arith_comm_to_1[i];
    }

    setWireMasksParty(input_pid_map, rand_sh_sec, rand_sh_sec_to_1, b_rand_sh_sec,
                        rand_sh_party, b_rand_sh_party);

  } else if (id_ == 2) {
    std::vector<size_t> lengths(6);
    network_->recv(0, lengths.data(), sizeof(size_t) * 6);

    size_t arith_comm = lengths[0];
    size_t rand_sh_sec_num = lengths[1];
    size_t rand_sh_party_num = lengths[2];
    size_t bool_comm = lengths[3];
    size_t b_rand_sh_sec_num = lengths[4];
    size_t b_rand_sh_party_num = lengths[5];
    int num_comm = arith_comm/100000000;
    int last_comm = arith_comm%100000000;

    std::vector<Ring> offline_arith_comm(arith_comm);
    for(int i = 0; i < num_comm; i++){
      std::vector<Ring> data_recv_i(100000000);

      network_->recv(0, data_recv_i.data(), sizeof(Ring) * 100000000);
      for(int j = 0; j<100000000; j++){
        offline_arith_comm[i*100000000 + j] = data_recv_i[j];
      }
    }
                    
    std::vector<Ring> data_recv_last(last_comm);
    network_->recv(0, data_recv_last.data(), sizeof(Ring) * last_comm);
    for(int j = 0; j<last_comm; j++){
      offline_arith_comm[num_comm*100000000 + j] = data_recv_last[j];
    }
    // network_->recv(0, offline_arith_comm.data(), sizeof(Ring) * arith_comm);
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
    
    setWireMasksParty(input_pid_map, rand_sh_sec, rand_sh_sec_to_1, b_rand_sh_sec,
                        rand_sh_party, b_rand_sh_party);
  }
  
}




PreprocCircuit<Ring> OfflineEvaluator::getPreproc() {
  return std::move(preproc_);
}
  
PreprocCircuit<Ring> OfflineEvaluator::run(
    const std::unordered_map<common::utils::wire_t, int>& input_pid_map) {
  
  setWireMasks(input_pid_map);

  return std::move(preproc_);
  
}


};
