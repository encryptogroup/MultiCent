#include "online_evaluator.h"

#include <array>

#include "../utils/helpers.h"

namespace graphsc
{
    OnlineEvaluator::OnlineEvaluator(int id, std::shared_ptr<io::NetIOMP> network,
                                     PreprocCircuit<Ring> preproc,
                                     common::utils::LevelOrderedCircuit circ,
                                     int threads, int seed)
        : id_(id),
          rgen_(id, seed),
          network_(std::move(network)),
          preproc_(std::move(preproc)),
          circ_(std::move(circ)),
          wires_(circ.num_wires),
          q_sh_(circ.num_gates),
          q_val_(circ.num_gates)
    {
        tpool_ = std::make_shared<ThreadPool>(threads);
    }

    OnlineEvaluator::OnlineEvaluator(int id, std::shared_ptr<io::NetIOMP> network,
                                     PreprocCircuit<Ring> preproc,
                                     common::utils::LevelOrderedCircuit circ,
                                     std::shared_ptr<ThreadPool> tpool, int seed)
        : id_(id),
          rgen_(id, seed),
          network_(std::move(network)),
          preproc_(std::move(preproc)),
          circ_(std::move(circ)),
          tpool_(std::move(tpool)),
          wires_(circ.num_gates),
          q_sh_(circ.num_gates),
          q_val_(circ.num_gates) {}

    void OnlineEvaluator::setInputs(const std::unordered_map<common::utils::wire_t, Ring> &inputs)
    {
        // Input gates have depth 0
        for (auto &g : circ_.gates_by_level[0])
        {
            if (g->type == common::utils::GateType::kInp)
            {
                auto *pre_input = static_cast<PreprocInput<Ring> *>(preproc_.gates[g->out].get());
                auto pid = pre_input->pid;

                if (id_ != 0)
                {
                    if (pid == id_)
                    {   
                        Ring val;
                        rgen_.p12().random_data(&val, sizeof(Ring));
                        wires_[g->out] = inputs.at(g->out) - val;
                    }
                    else
                    {
                        Ring val;
                        rgen_.p12().random_data(&val, sizeof(Ring));
                        wires_[g->out] = val;
                    }
                }
            }
        }
    }

    void OnlineEvaluator::setRandomInputs()
    {
        // Input gates have depth 0.
        for (auto &g : circ_.gates_by_level[0])
        {
            if (g->type == common::utils::GateType::kInp)
            {
                
                Ring val;
                rgen_.pi(id_).random_data(&val, sizeof(Ring));
                wires_[g->out] = val;
                
                
            }
        }
    }

    void OnlineEvaluator::evaluateGatesAtDepthPartySend(size_t depth,
                                                        std::vector<Ring> &mult_vals, std::vector<Ring> &shuffle_vals)
    {

        for (auto &gate : circ_.gates_by_level[depth])
        {
            switch (gate->type)
            {
            case common::utils::GateType::kMul:
            {
                // All parties excluding TP sample a common random value r_in
                auto *g = static_cast<common::utils::FIn2Gate *>(gate.get());

                if (id_ != 0)
                {

                    auto *pre_out =
                        static_cast<PreprocMultGate<Ring> *>(preproc_.gates[g->out].get());
                    auto xa = pre_out->triple_a.valueAt() + wires_[g->in2];
                    auto yb = pre_out->triple_b.valueAt() + wires_[g->in2];
                    mult_vals.push_back(xa);
                    mult_vals.push_back(yb);

                }

                break;
            }

            case common::utils::GateType::kShuffle:
            {
                // All parties excluding TP sample a common random value r_in
                auto *g = static_cast<common::utils::SIMDOGate *>(gate.get());

                if (id_ != 0)
                {

                    auto *pre_out =
                        static_cast<PreprocShuffleGate<Ring> *>(preproc_.gates[g->out].get());
                    std::vector<int> pi;
                    if(id_ == 1){
                        pi = pre_out->pi_;
                    }
                    else{
                        pi = pre_out->pi;
                    }
                    for (size_t i = 0; i < g->in1.size(); i++) {
                        auto xa = g->in1[pi[i]] + pre_out->pi_ab[pi[i]];
                        shuffle_vals.push_back(xa);
                    }

                }

                break;
            }

            case ::common::utils::GateType::kAdd:
            case ::common::utils::GateType::kSub:
            case ::common::utils::GateType::kConstAdd:
            case ::common::utils::GateType::kConstMul:
            {
                break;
            }


            default:
                break;
            }
        }
    }

    void OnlineEvaluator::evaluateGatesAtDepthPartyRecv(size_t depth,
                                                        std::vector<Ring> &mult_vals, std::vector<Ring> &shuffle_vals)
    {
        size_t idx_mult = 0;
        size_t idx_shuffle = 0;

        for (auto &gate : circ_.gates_by_level[depth])
        {
            switch (gate->type)
            {
            case common::utils::GateType::kAdd:
            {
                auto *g = static_cast<common::utils::FIn2Gate *>(gate.get());
                if (id_ != 0)
                    wires_[g->out] = wires_[g->in1] + wires_[g->in2];
                break;
            }

            case common::utils::GateType::kSub:
            {
                auto *g = static_cast<common::utils::FIn2Gate *>(gate.get());
                if (id_ != 0)
                    wires_[g->out] = wires_[g->in1] - wires_[g->in2];
                break;
            }

            case common::utils::GateType::kConstAdd:
            {
                auto *g = static_cast<common::utils::ConstOpGate<Ring> *>(gate.get());
                wires_[g->out] = wires_[g->in] + g->cval;
                break;
            }

            case common::utils::GateType::kConstMul:
            {
                auto *g = static_cast<common::utils::ConstOpGate<Ring> *>(gate.get());
                if (id_ != 0)
                    wires_[g->out] = wires_[g->in] * g->cval;
                break;
            }

            case common::utils::GateType::kMul:
            {
                auto *g = static_cast<common::utils::FIn2Gate *>(gate.get());
                auto *pre_out =
                        static_cast<PreprocMultGate<Ring> *>(preproc_.gates[g->out].get());
                auto a = pre_out->triple_a.valueAt();
                auto b = pre_out->triple_b.valueAt();
                auto c = pre_out->triple_c.valueAt();
                if (id_ != 0)
                {
                    wires_[g->out] = mult_vals[2*idx_mult]*mult_vals[2*idx_mult + 1]*(id_-1) - mult_vals[2*idx_mult]*b - mult_vals[2*idx_mult+1]*a + c;
                }
                idx_mult++;
                break;
            }

            case common::utils::GateType::kShuffle:
            {
                auto *g = static_cast<common::utils::SIMDOGate *>(gate.get());
                auto *pre_out =
                        static_cast<PreprocShuffleGate<Ring> *>(preproc_.gates[g->out].get());
                auto pi_ab = pre_out->pi_ab;
                auto pi = pre_out->pi;
                auto pi_ = pre_out->pi_;
                if (id_ != 0)
                {
                    std::vector<int> pi;
                    auto pi_ab = pre_out->pi_ab;
                    if(id_ == 1){
                        pi = pre_out->pi;
                    }
                    else{
                        pi = pre_out->pi_;
                    }
                    for (int i = 0; i < g->in1.size(); i++) {
                        wires_[g->outs[i]] = shuffle_vals[i];
                    }
                    
                }
                idx_shuffle += g->in1.size();
                break;
            }

            default:
                break;
            }
        }
    }

    void OnlineEvaluator::evaluateGatesAtDepth(size_t depth)
    {
        size_t mult_num = 0;
        size_t shuffle_num = 0;
        size_t vec_size = 0;

        std::vector<Ring> mult_vals;
        std::vector<Ring> shuffle_vals;

        for (auto &gate : circ_.gates_by_level[depth])
        {
            switch (gate->type)
            {
            case common::utils::GateType::kInp:
            case common::utils::GateType::kAdd:
            case common::utils::GateType::kSub:
            {
                break;
            }
            case common::utils::GateType::kMul:
            {
                mult_num++;
                break;
            }

            case common::utils::GateType::kShuffle:
            {

                auto *g = static_cast<common::utils::SIMDOGate *>(gate.get());
                shuffle_num += g->in1.size();

                break;
            }

            }
        }


        size_t total_comm = 2*mult_num + shuffle_num;

        if (id_ != 0)
        {
            evaluateGatesAtDepthPartySend(depth, mult_vals, shuffle_vals);
            
            std::vector<Ring> data_send = mult_vals;
            data_send.reserve(total_comm);
            data_send.insert(data_send.end(), mult_vals.begin(), mult_vals.end());
            data_send.insert(data_send.end(), shuffle_vals.begin(), shuffle_vals.end());

            int num_comm = total_comm/100000;
            int last_comm = total_comm%100000;
            std::vector<Ring> data_recv(total_comm);


            if (id_ == 1)
                {   

                    for(int i = 0; i < num_comm; i++){
                       std::vector<Ring> data_send_i;
                       data_send_i.reserve(100000);
                       for(int j = 0; j<100000; j++){
                            data_send_i[j] = data_send[i*100000 + j];   
                       }

                       network_->send(2, data_send_i.data(), sizeof(Ring) * 100000);
                       std::vector<Ring> data_recv_i(100000);

                       network_->recv(2, data_recv_i.data(), sizeof(Ring) * 100000);
                       for(int j = 0; j<100000; j++){
                            data_recv[i*100000 + j] = data_recv_i[j];
                       }

                    }
                    

                    std::vector<Ring> data_send_last;
                    data_send_last.reserve(last_comm);
                    for(int j = 0; j<last_comm; j++){
                        data_send_last[j] = data_send[num_comm*100000 + j];
                    }
                    network_->send(2, data_send_last.data(), sizeof(Ring) * last_comm);
                    std::vector<Ring> data_recv_last(last_comm);
                    network_->recv(2, data_recv_last.data(), sizeof(Ring) * last_comm);
                    for(int j = 0; j<last_comm; j++){
                        data_recv[num_comm*100000 + j] = data_recv_last[j];
                    }

                }
            if (id_ == 2)
                {

                    for(int i = 0; i < num_comm; i++){

                       std::vector<Ring> data_send_i;
                       data_send_i.reserve(100000);
                       for(int j = 0; j<100000; j++){
                            data_send_i[j] = data_send[i*100000 + j];
                       }

                       network_->send(1, data_send_i.data(), sizeof(Ring) * 100000);
                       std::vector<Ring> data_recv_i(100000);

                       network_->recv(1, data_recv_i.data(), sizeof(Ring) * 100000);

                       for(int j = 0; j<100000; j++){
                            data_recv[i*100000 + j] = data_recv_i[j];
                       }
                    }

                    std::vector<Ring> data_send_last;
                    data_send_last.reserve(last_comm);
                    for(int j = 0; j<last_comm; j++){
                        data_send_last[j] = data_send[num_comm*100000 + j];
                    }
                    network_->send(1, data_send_last.data(), sizeof(Ring) * last_comm);
                    std::vector<Ring> data_recv_last(last_comm);
                    network_->recv(1, data_recv_last.data(), sizeof(Ring) * last_comm);
                    for(int j = 0; j<last_comm; j++){
                        data_recv[num_comm*100000 + j] = data_recv_last[j];
                    }
                    
                }


            evaluateGatesAtDepthPartyRecv(depth,
                                          mult_vals, shuffle_vals);
        }
    }


    std::vector<Ring> OnlineEvaluator::getOutputs()
    {   
        std::vector<Ring> outvals(circ_.outputs.size());
        if (circ_.outputs.empty())
        {
            return outvals;
        }

        if (id_ != 0)
        {
            std::vector<Ring> output_share_my(circ_.outputs.size());
            std::vector<Ring> output_share_other(circ_.outputs.size());
            for (size_t i = 0; i < circ_.outputs.size(); ++i)
            {
                auto wout = circ_.outputs[i];
                output_share_my[i] = wires_[wout];
                
            }

            if(id_ == 1){
                network_->send(2, output_share_my.data(), output_share_my.size() * sizeof(Ring));
                network_->recv(2, output_share_other.data(), output_share_other.size() * sizeof(Ring));
            }
            if(id_ == 2){
                network_->send(1, output_share_my.data(), output_share_my.size() * sizeof(Ring));
                network_->recv(1, output_share_other.data(), output_share_other.size() * sizeof(Ring));
            }

            for (size_t i = 0; i < circ_.outputs.size(); ++i)
            {
                Ring outmask = output_share_other[i];
                outvals[i] = output_share_my[i] + outmask ;
            }
            return outvals;
        }
        else
        {
            std::vector<Ring> output_masks(circ_.outputs.size());
            network_->recv(0, output_masks.data(), output_masks.size() * sizeof(Field));
            for (size_t i = 0; i < circ_.outputs.size(); ++i)
            {
                Ring outmask = output_masks[i];
                auto wout = circ_.outputs[i];
                outvals[i] = wires_[wout] - outmask;
            }
            return outvals;
        }
    }

    std::vector<Ring> OnlineEvaluator::evaluateCircuit(const std::unordered_map<common::utils::wire_t, Ring> &inputs)
    {
        setRandomInputs();
        
        for (size_t i = 0; i < circ_.gates_by_level.size(); ++i)
        {
            evaluateGatesAtDepth(i);
        }

        return getOutputs();

    }

   
}; // namespace graphsc
