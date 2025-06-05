#include "online_evaluator.h"

#include <array>

#include "../utils/helpers.h"

namespace graphsc
{
    OnlineEvaluator::OnlineEvaluator(int id, std::shared_ptr<io::NetIOMP> network,
                                     PreprocCircuit<Ring> preproc,
                                     common::utils::LevelOrderedCircuit circ,
                                     int threads, uint64_t seeds_h[5], uint64_t seeds_l[5])
        : id_(id),
          rgen_(id, 3, seeds_h, seeds_l),
          network_(std::move(network)),
          preproc_(std::move(preproc)),
          circ_(std::move(circ)),
          wires_(circ.num_wires),
          q_sh_(circ.num_gates), // TODO shouldn't this be num_wires??? but also, appears to be unused
          q_val_(circ.num_gates) // TODO shouldn't this be num_wires??? but also, appears to be unused
    {
        tpool_ = std::make_shared<ThreadPool>(threads);
    }

    OnlineEvaluator::OnlineEvaluator(int id, std::shared_ptr<io::NetIOMP> network,
                                     PreprocCircuit<Ring> preproc,
                                     common::utils::LevelOrderedCircuit circ,
                                     std::shared_ptr<ThreadPool> tpool, uint64_t seeds_h[5], uint64_t seeds_l[5])
        : id_(id),
          rgen_(id, 3, seeds_h, seeds_l),
          network_(std::move(network)),
          preproc_(std::move(preproc)),
          circ_(std::move(circ)),
          tpool_(std::move(tpool)),
          wires_(circ.num_wires),
          q_sh_(circ.num_gates), // TODO shouldn't this be num_wires??? but also, appears to be unused
          q_val_(circ.num_gates) {} // TODO shouldn't this be num_wires??? but also, appears to be unused

    void OnlineEvaluator::setInputs(const std::unordered_map<common::utils::wire_t, Ring> &inputs)
    {
        if (id_ == 0) return;
        // Input gates have depth 0
        for (auto &g : circ_.gates_by_level[0])
        {
            if (g->type == common::utils::GateType::kInp)
            {
                auto *pre_input = static_cast<PreprocInput<Ring> *>(preproc_.gates[g->gid].get());
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
            else if (g->type == common::utils::GateType::kBinInp)
            {
                auto *pre_input = static_cast<PreprocInput<Ring> *>(preproc_.gates[g->gid].get());
                auto pid = pre_input->pid;

                if (id_ != 0)
                {
                    if (pid == id_)
                    {   
                        Ring val;
                        rgen_.p12().random_data(&val, sizeof(Ring));
                        wires_[g->out] = inputs.at(g->out) ^ val;
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

    void OnlineEvaluator::evaluateGatesAtDepthPartySend(size_t depth,
                                                        std::vector<Ring> &mult_vals, std::vector<Ring> &and_vals, std::vector<Ring> &shuffle_vals, std::vector<Ring> &reveal_vals)
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
                        static_cast<PreprocMultGate<Ring> *>(preproc_.gates[g->gid].get());
                    auto xa = pre_out->triple_a.valueAt() + wires_[g->in1];
                    auto yb = pre_out->triple_b.valueAt() + wires_[g->in2];
                    mult_vals.push_back(xa);
                    mult_vals.push_back(yb);

                }

                break;
            }

            case common::utils::GateType::kConvertB2A:
            {
                // All parties excluding TP sample a common random value r_in
                auto *g = static_cast<common::utils::FIn1Gate *>(gate.get());

                if (id_ != 0)
                {

                    auto *pre_out =
                        static_cast<PreprocMultGate<Ring> *>(preproc_.gates[g->gid].get());

                    // perform a multiplication of Boolean shares x_0 and x_1
                    //
                    // through (x_0 + 0) * (0 + x_1)
                    //
                    // where P0 sets the share of the second input to 0 and
                    // P1 sets the share of the first input to 0
                    auto xa = pre_out->triple_a.valueAt() + (wires_[g->in] & 1) * (id_ == 1 ? 1 : 0);
                    auto yb = pre_out->triple_b.valueAt() + (wires_[g->in] & 1) * (id_ == 1 ? 0 : 1);

                    mult_vals.push_back(xa);
                    mult_vals.push_back(yb);

                }

                break;
            }

            case common::utils::GateType::kAnd:
            {
                // All parties excluding TP sample a common random value r_in
                auto *g = static_cast<common::utils::FIn2Gate *>(gate.get());

                if (id_ != 0)
                {

                    auto *pre_out =
                        static_cast<PreprocMultGate<Ring> *>(preproc_.gates[g->gid].get());
                    auto xa = pre_out->triple_a.valueAt() ^ wires_[g->in1];
                    auto yb = pre_out->triple_b.valueAt() ^ wires_[g->in2];
                    and_vals.push_back(xa);
                    and_vals.push_back(yb);
                }

                break;
            }

            case common::utils::GateType::kEqualsZero:
            {
                // All parties excluding TP sample a common random value r_in
                auto *g = static_cast<common::utils::ParamFIn1Gate *>(gate.get());

                if (id_ != 0)
                {

                    auto *pre_out =
                        static_cast<PreprocMultGate<Ring> *>(preproc_.gates[g->gid].get());

                    auto my_share = wires_[g->in];

                    // if first layer and we are id_ = 2, negate our share
                    //
                    // [0] = x_1 + x_2 <==> x_1 = -x_2
                    if (g->param == 0 && id_ == 2) {
                      my_share = -my_share;
                    }
                    
                    auto in1 = my_share;
                    auto in2 = my_share;

                    // always do a | b with different inputs depending on layer
                    //
                    // layer 0: width = 16
                    //   wire in := aaaaaaaaaaaaaaaabbbbbbbbbbbbbbbb
                    //       in1 := 0000000000000000aaaaaaaaaaaaaaaa
                    //       in2 := 0000000000000000bbbbbbbbbbbbbbbb
                    //        out = 0000000000000000cccccccccccccccc
                    //
                    // layer 1: width = 8
                    //   wire in := 0000000000000000aaaaaaaabbbbbbbb
                    //       in1 := 000000000000000000000000aaaaaaaa
                    //       in2 := 000000000000000000000000bbbbbbbb
                    //        out = 000000000000000000000000cccccccc
                    //
                    // ...
                    //
                    // layer 4: width = 1
                    //   wire in := 000000000000000000000000000000ab
                    //       in1 := 0000000000000000000000000000000a
                    //       in2 := 0000000000000000000000000000000b
                    //        out = 0000000000000000000000000000000c
                    size_t width = (1 << (4 - g->param));
                    in1 >>= width;

                    // clear leftmost bits to 0 for better debugging
                    // not needed for correctness, as these bits are ignored later
                    //
                    in1 <<= (32 - width);
                    in1 >>= (32 - width);
                    in2 <<= (32 - width);
                    in2 >>= (32 - width);

                    // use de morgan and implement a | b using ~(~a & ~b)
                    if (id_ == 1) {
                      in1 = ~in1;
                      in2 = ~in2;
                    }

                    auto xa = pre_out->triple_a.valueAt() ^ in1;
                    auto yb = pre_out->triple_b.valueAt() ^ in2;

                    and_vals.push_back(xa);
                    and_vals.push_back(yb);
                }

                break;
            }

            case common::utils::GateType::kShuffle:
            {
                // All parties excluding TP sample a common random value r_in
                auto *g = static_cast<common::utils::ParamWithFlagSIMDOGate *>(gate.get());
                bool reverse = g->flag;

                if (id_ != 0) {
                    auto *pre_out = static_cast<PreprocShuffleGate<Ring> *>(preproc_.gates[g->gid].get());
                    std::vector<Ring> *mask;
                    if (id_ == 1)
                        mask = &pre_out->mask_0;
                    else
                        mask = &pre_out->mask_1;
                    vector<Ring> to_send(g->in1.size());
                    for (size_t j = 0; j < g->in1.size(); j++) {
                        if (reverse) {
                            // P0 sends pi_0^(-1)(share + R_0) and P1 sends pi'_1^(-1)(share + R_1) where R_i = masks[0]
                            // pi_0 is shuffle[0] and pi'_1 is shuffle[3], i.e.,
                            // use shuffle[i * 3].
                            to_send[j] = wires_[g->in1[(id_ == 1 ? *(pre_out->pi_0) : *(pre_out->rho_1))[j]]]
                                                        + (*mask)[(id_ == 1 ? *(pre_out->pi_0) : *(pre_out->rho_1))[j]];
                        } else {
                            // P0 sends pi'_0(share + R_0) and P1 sends pi_1(share + R_1) where R_i = masks[0]
                            // pi'_0 is shuffle[1] and pi_1 is shuffle[2], i.e.,
                            // use shuffle[i + 1].
                            to_send[(id_ == 1 ? *(pre_out->rho_0) : *(pre_out->pi_1))[j]] = wires_[g->in1[j]] + (*mask)[j];
                        }
                    }
                    for (size_t j = 0; j < g->in1.size(); j++) {
                        shuffle_vals.push_back(to_send[j]);
                        // std::cout << "d1" << wires_[g->in1[j]] << " " << to_send[j] << std::endl;
                    }
                }

                break;
            }

            case common::utils::GateType::kDoubleShuffle: // TODO cleanup as mostly copy&paste
            {
                // All parties excluding TP sample a common random value r_in
                auto *g = static_cast<common::utils::ThreeParamSIMDOGate *>(gate.get());

                if (id_ != 0) {
                    auto *pre_out = static_cast<PreprocShuffleGate<Ring> *>(preproc_.gates[g->gid].get());
                    std::vector<Ring> *mask;
                    if (id_ == 1)
                        mask = &pre_out->mask_0;
                    else
                        mask = &pre_out->mask_1;
                    vector<Ring> to_send(g->in1.size());
                    for (size_t j = 0; j < g->in1.size(); j++) {
                        // P0 sends pi'_0(share + R_0) and P1 sends pi_1(share + R_1) where R_i = masks[0]
                        // pi'_0 is shuffle[1] and pi_1 is shuffle[2], i.e.,
                        // use shuffle[i + 1].
                        to_send[(id_ == 1 ? *(pre_out->rho_0) : *(pre_out->pi_1))[j]] = wires_[g->in1[j]] + (*mask)[j];
                    }
                    for (size_t j = 0; j < g->in1.size(); j++) {
                        shuffle_vals.push_back(to_send[j]);
                        // std::cout << "d1" << wires_[g->in1[j]] << " " << to_send[j] << std::endl;
                    }
                }

                break;
            }

            case common::utils::GateType::kGenCompaction:
            {
                // All parties excluding TP sample a common random value r_in
                auto *g = static_cast<common::utils::SIMDOGate *>(gate.get());

                if (id_ != 0)
                {

                    std::vector<Ring> f_0;
                    // set f_0 to 1 - input and f_1 to input, we just immediately use input instead of f_1
                    for (size_t j = 0; j < g->in1.size(); j++) {
                        f_0.push_back(- wires_[g->in1[j]]);
                        if (id_ == 1) {
                            f_0[j] += 1; // 1 as constant only to one share
                        }
                    }

                    std::vector<Ring> s_0, s_1;
                    Ring s = 0;
                    // Set s_0 to prefix sum of f_0 and s_1 to prefix sum of f_1/input continuing from the prior final value
                    for (size_t j = 0; j < g->in1.size(); j++) {
                        s += f_0[j];
                        s_0.push_back(s);
                    }
                    for (size_t j = 0; j < g->in1.size(); j++) {
                        s += wires_[g->in1[j]];
                        s_1.push_back(s - s_0[j]); // s_0[j] see below
                    }

                    // We now have to compute s_0 + input * (s_1 - s_0) (element-wise multiplication).
                    // Previously, we already subtracted s_0 from s_1, so we just compute s_0 + input * s_1
                    // s_0 is added after the communication though, here, we just multiply.

                    auto *pre_out =
                        static_cast<PreprocGenCompactionGate<Ring> *>(preproc_.gates[g->gid].get());
                    for (size_t j = 0; j < g->in1.size(); j++) {
                        auto xa = pre_out->triple_a[j].valueAt() + wires_[g->in1[j]];
                        auto yb = pre_out->triple_b[j].valueAt() + s_1[j];
                        mult_vals.push_back(xa);
                        mult_vals.push_back(yb);
                    }

                }

                break;
            }

            case common::utils::GateType::kReveal:
            {
                auto *g = static_cast<common::utils::SIMDOGate *>(gate.get());

                if (id_ != 0)
                {

                    for (size_t j = 0; j < g->in1.size(); j++) {
                        reveal_vals.push_back(wires_[g->in1[j]]);
                    }

                }

                break;
            }

            case ::common::utils::GateType::kAdd:
            case ::common::utils::GateType::kAddVec:
            case ::common::utils::GateType::kXor:
            case ::common::utils::GateType::kSub:
            case ::common::utils::GateType::kConstAdd:
            case ::common::utils::GateType::kConstMul:
            case ::common::utils::GateType::kInp:
            case ::common::utils::GateType::kBinInp:
            case ::common::utils::GateType::kReorder:
            case ::common::utils::GateType::kReorderInverse:
            case ::common::utils::GateType::kFlip:
            case ::common::utils::GateType::kAddConstToVec:
            case ::common::utils::GateType::kPreparePropagate:
            case ::common::utils::GateType::kPropagate:
            case ::common::utils::GateType::kPrepareGather:
            case ::common::utils::GateType::kGather:
            case ::common::utils::GateType::kCompose:
            {
                break;
            }


            default:
                std::cout << gate->type << std::endl;
                throw std::runtime_error("UNSUPPORTED GATE discovered during protocol execution (see above)");
            }
        }
    }

    void OnlineEvaluator::evaluateGatesAtDepthPartyRecv(size_t depth,
                                                        std::vector<Ring> &mult_vals, std::vector<Ring> &and_vals, std::vector<Ring> &shuffle_vals, std::vector<Ring> &reveal_vals)
    {
        size_t idx_mult = 0;
        size_t idx_and = 0;
        size_t idx_shuffle = 0;
        size_t idx_reveal = 0;

        for (auto &gate : circ_.gates_by_level[depth])
        {
            switch (gate->type)
            {
            case common::utils::GateType::kAdd:
            {
                auto *g = static_cast<common::utils::FIn2Gate *>(gate.get());
                if (id_ != 0) {
                    wires_[g->out] = wires_[g->in1] + wires_[g->in2];
                }
                break;
            }
            case common::utils::GateType::kAddVec:
            {
                auto *g = static_cast<common::utils::SIMDODoubleInGate *>(gate.get());
                if (id_ != 0) {
                    for (size_t j = 0; j < g->in1.size(); j++) {
                        wires_[g->outs[j]] = wires_[g->in1[j]] + wires_[g->in2[j]];
                    }
                }
                break;
            }
            case common::utils::GateType::kXor:
            {
                
                auto *g = static_cast<common::utils::FIn2Gate *>(gate.get());
                if (id_ != 0){
                    wires_[g->out] = wires_[g->in1] ^ wires_[g->in2];
                }
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

            case common::utils::GateType::kFlip:
            {
                auto *g = static_cast<common::utils::SIMDOGate *>(gate.get());
                if (id_ != 0) {
                    for (size_t j = 0; j < g->in1.size(); j++) {
                        // 1 - (x + y) = (1-x) + (-y)
                        if (id_ == 1)
                            wires_[g->outs[j]] = 1 - wires_[g->in1[j]];
                        else
                            wires_[g->outs[j]] = -wires_[g->in1[j]];
                    }
                }
                break;
            }

            case common::utils::GateType::kAddConstToVec:
            {
                auto *g = static_cast<common::utils::TwoParamSIMDOGate *>(gate.get());
                if (id_ != 0) {
                    for (size_t j = 0; j < g->param2; j++) {
                        if (id_ == 1)
                            wires_[g->outs[j]] = wires_[g->in1[j]] + g->param1;
                        else
                            wires_[g->outs[j]] = wires_[g->in1[j]];
                    }
                    for (size_t j = g->param2; j < g->in1.size(); j++) {
                        wires_[g->outs[j]] = wires_[g->in1[j]];
                    }
                }
                break;
            }

            case common::utils::GateType::kPreparePropagate:
            {
                auto *g = static_cast<common::utils::ParamSIMDOGate *>(gate.get());
                if (id_ != 0) {
                    for (size_t j = g->param - 1; j > 0; j--) {
                        // param -1, ..., 1
                        wires_[g->outs[j]] = wires_[g->in1[j]] - wires_[g->in1[j - 1]];
                    }
                    wires_[g->outs[0]] = wires_[g->in1[0]];
                    for (size_t j = g->param; j < g->in1.size(); j++) {
                        wires_[g->outs[j]] = wires_[g->in1[j]];
                    }
                }
                break;
            }

            case common::utils::GateType::kPropagate:
            {
                auto *g = static_cast<common::utils::SIMDODoubleInGate *>(gate.get());
                if (id_ != 0) {
                    Ring accu = 0;
                    for (size_t j = 0; j < g->in1.size(); j++) {
                        accu += wires_[g->in1[j]];
                        wires_[g->outs[j]] = accu - wires_[g->in2[j]];
                    }
                }
                break;
            }

            case common::utils::GateType::kPrepareGather:
            {
                auto *g = static_cast<common::utils::SIMDOGate *>(gate.get());
                if (id_ != 0) {
                    Ring accu = 0;
                    for (size_t j = 0; j < g->in1.size(); j++) {
                        accu += wires_[g->in1[j]];
                        wires_[g->outs[j]] = accu;
                    }
                }
                break;
            }

            case common::utils::GateType::kGather:
            {
                auto *g = static_cast<common::utils::ParamSIMDOGate *>(gate.get());
                if (id_ != 0) {
                    Ring accu = 0;
                    for (size_t j = 0; j < g->param; j++) {
                        wires_[g->outs[j]] = wires_[g->in1[j]] - accu;
                        accu += wires_[g->outs[j]];
                    }
                    for (size_t j = g->param; j < g->in1.size(); j++) {
                        wires_[g->outs[j]] = 0;
                    }
                }
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
                        static_cast<PreprocMultGate<Ring> *>(preproc_.gates[g->gid].get());
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

            case common::utils::GateType::kConvertB2A:
            {
                auto *g = static_cast<common::utils::FIn1Gate *>(gate.get());
                auto *pre_out =
                        static_cast<PreprocMultGate<Ring> *>(preproc_.gates[g->gid].get());
                auto a = pre_out->triple_a.valueAt();
                auto b = pre_out->triple_b.valueAt();
                auto c = pre_out->triple_c.valueAt();
                if (id_ != 0)
                {
                    // x_0 + x_1 - 2 * x_0 * x_1
                    // ---------       ----------\
                    //  original Boolean share   |
                    //    as-is in arithmetic     multiplication result
                    auto mult_result = mult_vals[2*idx_mult]*mult_vals[2*idx_mult + 1]*(id_-1) - mult_vals[2*idx_mult]*b - mult_vals[2*idx_mult+1]*a + c;
                    auto original_share = wires_[g->in] & 1;

                    wires_[g->out] = original_share - 2 * mult_result;
                }
                idx_mult++;
                break;
            }

            case common::utils::GateType::kAnd:
            {
                auto *g = static_cast<common::utils::FIn2Gate *>(gate.get());
                auto *pre_out =
                        static_cast<PreprocMultGate<Ring> *>(preproc_.gates[g->gid].get());
                auto a = pre_out->triple_a.valueAt();
                auto b = pre_out->triple_b.valueAt();
                auto c = pre_out->triple_c.valueAt();
                if (id_ != 0)
                {
                    wires_[g->out] = (and_vals[2*idx_and] & and_vals[2*idx_and + 1])*(id_-1) ^ and_vals[2*idx_and] & b ^ and_vals[2*idx_and+1] & a ^ c;
                }
                idx_and++;
                break;
            }

            case common::utils::GateType::kEqualsZero:
            {
                auto *g = static_cast<common::utils::ParamFIn1Gate *>(gate.get());
                auto *pre_out =
                        static_cast<PreprocMultGate<Ring> *>(preproc_.gates[g->gid].get());
                auto a = pre_out->triple_a.valueAt();
                auto b = pre_out->triple_b.valueAt();
                auto c = pre_out->triple_c.valueAt();
                if (id_ != 0)
                {
                    auto result = (and_vals[2*idx_and] & and_vals[2*idx_and + 1])*(id_-1) ^ and_vals[2*idx_and] & b ^ and_vals[2*idx_and+1] & a ^ c;

                    // de morgan: a | b = ~(~a & ~b)
                    //
                    // if last round, do not flip output
                    if (id_ == 1 && g->param < 4) {
                      result = ~result;
                    }

                    // if last layer, then preserve only LSB
                    if (g->param == 4) {
                      result <<= 31;
                      result >>= 31;
                    }

                    wires_[g->out] = result;
                }
                idx_and++;
                break;
            }

            case common::utils::GateType::kShuffle:
            {
                auto *g = static_cast<common::utils::ParamWithFlagSIMDOGate *>(gate.get());
                bool reverse = g->flag;
                auto *pre_out = static_cast<PreprocShuffleGate<Ring> *>(preproc_.gates[g->gid].get());
                std::vector<Ring> *b;
                if (id_ == 1)
                    b = &pre_out->b_0;
                else
                    b = &pre_out->b_1;
                if (id_ != 0) {
                    // Apply remaining permutation
                    for (size_t j = 0; j < g->in1.size(); j++) {
                        if (reverse) {
                            // pi'_0^(-1) for P0, pi_1^(-1) for P1, i.e.,
                            // use shuffle[i + 1].
                            // After that, subtract B_i = masks[1]
                            wires_[g->outs[j]] = shuffle_vals[idx_shuffle + (id_ == 1 ? *(pre_out->rho_0) : *(pre_out->pi_1))[j]]- (*b)[j];
                        } else {
                            // pi_0 for P0, pi'_1 for P1, i.e.,
                            // use shuffle[i * 3].
                            // After that, subtract B_i = masks[1]
                            wires_[g->outs[(id_ == 1 ? *(pre_out->pi_0) : *(pre_out->rho_1))[j]]] = shuffle_vals[idx_shuffle + j]
                                                                    - (*b)[(id_ == 1 ? *(pre_out->pi_0) : *(pre_out->rho_1))[j]];
                        }
                    }
                    // for (size_t j = 0; j < g->in1.size(); j++)
                    //     std::cout << "d2 " << wires_[g->outs[j]] << std::endl;
                }

                idx_shuffle += g->in1.size();

                break;
            }

            case common::utils::GateType::kDoubleShuffle: // TODO cleanup as mostly copy paste
            {
                auto *g = static_cast<common::utils::ThreeParamSIMDOGate *>(gate.get());
                auto *pre_out = static_cast<PreprocShuffleGate<Ring> *>(preproc_.gates[g->gid].get());
                std::vector<Ring> *b;
                if (id_ == 1)
                    b = &pre_out->b_0;
                else
                    b = &pre_out->b_1;
                if (id_ != 0) {
                    // Apply remaining permutation
                    for (size_t j = 0; j < g->in1.size(); j++) {
                        // pi_0 for P0, pi'_1 for P1, i.e.,
                        // use shuffle[i * 3].
                        // After that, subtract B_i = masks[1]
                        wires_[g->outs[(id_ == 1 ? *(pre_out->pi_0) : *(pre_out->rho_1))[j]]] = shuffle_vals[idx_shuffle + j]
                                                                - (*b)[(id_ == 1 ? *(pre_out->pi_0) : *(pre_out->rho_1))[j]];
                    }
                    // for (size_t j = 0; j < g->in1.size(); j++)
                    //     std::cout << "d2 " << wires_[g->outs[j]] << std::endl;
                }

                idx_shuffle += g->in1.size();

                break;
            }

            case common::utils::GateType::kGenCompaction:
            {
                auto *g = static_cast<common::utils::SIMDOGate *>(gate.get());
                if (id_ != 0)
                {
                    // We have to compute s_0 + input * (s_1 - s_0) (element-wise multiplication).
                    // input * (s_1 - s_0) is already being computed.
                    // Recompute s_0 to not have to save this somewhere from when it was computed before sending.
                    std::vector<Ring> f_0;
                    // set f_0 to 1 - input
                    for (size_t j = 0; j < g->in1.size(); j++) {
                        f_0.push_back(- wires_[g->in1[j]]);
                        if (id_ == 1) {
                            f_0[j] += 1; // 1 as constant only to one share
                        }
                    }

                    std::vector<Ring> s_0;
                    Ring s = 0;
                    // Set s_0 to prefix sum of f_0
                    for (size_t j = 0; j < g->in1.size(); j++) {
                        s += f_0[j];
                        s_0.push_back(s);
                    }

                    // Now, finalize the multiplications and add vector s_0.
                    auto *pre_out =
                        static_cast<PreprocGenCompactionGate<Ring> *>(preproc_.gates[g->gid].get());
                    for (size_t j = 0; j < g->in1.size(); j++) {
                        auto a = pre_out->triple_a[j].valueAt();
                        auto b = pre_out->triple_b[j].valueAt();
                        auto c = pre_out->triple_c[j].valueAt();

                        wires_[g->outs[j]] = s_0[j] + mult_vals[2*idx_mult]*mult_vals[2*idx_mult + 1]*(id_-1) - mult_vals[2*idx_mult]*b - mult_vals[2*idx_mult+1]*a + c;
                        idx_mult++;
                    }
                } else {
                    idx_mult += g->in1.size();
                }
                break;
            }

            case common::utils::GateType::kReveal:
            {
                auto *g = static_cast<common::utils::SIMDOGate *>(gate.get());
                if (id_ != 0) {
                    for (size_t j = 0; j < g->in1.size(); j++) {
                        wires_[g->outs[j]] = reveal_vals[idx_reveal + j];
                    }
                }

                idx_reveal += g->in1.size();

                break;
            }

            case common::utils::GateType::kReorder:
            {
                auto *g = static_cast<common::utils::SIMDODoubleInGate *>(gate.get());
                if (id_ != 0) {
                    for (size_t j = 0; j < g->in1.size(); j++) {
                        // wires_[g->in2[j]] is the location where wires_[g->in1[j]] should go.
                        // Also, these locations are 1-indexed.
                        wires_[g->outs[wires_[g->in2[j]] - 1]] = wires_[g->in1[j]];
                    }
                }

                break;
            }

            case common::utils::GateType::kReorderInverse:
            {
                auto *g = static_cast<common::utils::SIMDODoubleInGate *>(gate.get());
                if (id_ != 0) {
                    for (size_t j = 0; j < g->in1.size(); j++) {
                        // See case above for kReorder, but here we reverse
                        wires_[g->outs[j]] = wires_[g->in1[wires_[g->in2[j]] - 1]];
                    }
                }

                break;
            }

            case common::utils::GateType::kCompose:
            {
                auto *g = static_cast<common::utils::SIMDSingleOutGate *>(gate.get());
                if (id_ != 0) {
                    auto out = wires_[g->in1[0]];
                    for (size_t j = 1; j < g->in1.size(); j++) {
                        out += (wires_[g->in1[j]] << j);
                    }
                    wires_[g->out] = out;
                }

                break;
            }

            case ::common::utils::GateType::kInp:
            case ::common::utils::GateType::kBinInp:
            {
                break;
            }

            default:
                std::cout << gate->type << std::endl;
                throw std::runtime_error("UNSUPPORTED GATE discovered during protocol execution (see above)");
            }
        }
    }

    void OnlineEvaluator::evaluateGatesAtDepth(size_t depth)
    {
        size_t mult_num = 0;
        size_t and_num = 0;
        size_t shuffle_num = 0;
        size_t reveal_num = 0;

        std::vector<Ring> mult_vals;
        std::vector<Ring> and_vals;
        std::vector<Ring> shuffle_vals;
        std::vector<Ring> reveal_vals;

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
            case common::utils::GateType::kConvertB2A:
            {
                mult_num++;
                break;
            }
            case common::utils::GateType::kAnd:
            case common::utils::GateType::kEqualsZero:
            {
                and_num++;
                break;
            }

            case common::utils::GateType::kShuffle:
            {

                auto *g = static_cast<common::utils::ParamWithFlagSIMDOGate *>(gate.get());
                shuffle_num += g->in1.size();

                break;
            }

            case common::utils::GateType::kDoubleShuffle:
            {

                auto *g = static_cast<common::utils::ThreeParamSIMDOGate *>(gate.get());
                shuffle_num += g->in1.size();

                break;
            }

            case common::utils::GateType::kGenCompaction:
            {
                // as this consists of multiple multiplications...
                auto *g = static_cast<common::utils::SIMDOGate *>(gate.get());
                mult_num += g->in1.size();

                break;
            }

            case common::utils::GateType::kReveal:
            {
                auto *g = static_cast<common::utils::SIMDOGate *>(gate.get());
                reveal_num += g->in1.size();

                break;
            }

            }
        }


        size_t total_comm = 2*mult_num + 2*and_num + shuffle_num + reveal_num;

        if (id_ != 0)
        {
            evaluateGatesAtDepthPartySend(depth, mult_vals, and_vals, shuffle_vals, reveal_vals);
            
            std::vector<Ring> data_send = mult_vals;
            data_send.reserve(total_comm);
            //data_send.insert(data_send.end(), mult_vals.begin(), mult_vals.end());
            data_send.insert(data_send.end(), and_vals.begin(), and_vals.end());
            data_send.insert(data_send.end(), shuffle_vals.begin(), shuffle_vals.end());
            data_send.insert(data_send.end(), reveal_vals.begin(), reveal_vals.end());
            
            int seg_factor = 100000; // 100000000; // Was 100000 during LAN benchmarks as per Graphiti, optimized to less rounds for WAN here!
            int num_comm = total_comm/seg_factor;
            int last_comm = total_comm%seg_factor;
            std::vector<Ring> data_recv(total_comm);


            if (id_ == 1)
                {   

                    for(int i = 0; i < num_comm; i++){
                       std::vector<Ring> data_send_i;
                       data_send_i.reserve(seg_factor);
                       for(int j = 0; j<seg_factor; j++){
                            data_send_i.push_back(data_send[i*seg_factor + j]);   
                       }

                       network_->send(2, data_send_i.data(), sizeof(Ring) * seg_factor);
                       std::vector<Ring> data_recv_i(seg_factor);

                       network_->recv(2, data_recv_i.data(), sizeof(Ring) * seg_factor);
                       for(int j = 0; j<seg_factor; j++){
                            data_recv[i*seg_factor + j] = data_recv_i[j];
                       }

                    }
                    

                    std::vector<Ring> data_send_last;
                    data_send_last.reserve(last_comm);
                    for(int j = 0; j<last_comm; j++){
                        data_send_last.push_back(data_send[num_comm*seg_factor + j]);
                    }
                    network_->send(2, data_send_last.data(), sizeof(Ring) * last_comm);
                    std::vector<Ring> data_recv_last(last_comm);
                    network_->recv(2, data_recv_last.data(), sizeof(Ring) * last_comm);
                    for(int j = 0; j<last_comm; j++){
                        data_recv[num_comm*seg_factor + j] = data_recv_last[j];
                    }

                }
            if (id_ == 2)
                {

                    for(int i = 0; i < num_comm; i++){

                       std::vector<Ring> data_send_i;
                       data_send_i.reserve(seg_factor);
                       for(int j = 0; j<seg_factor; j++){
                            data_send_i.push_back(data_send[i*seg_factor + j]);
                       }

                       network_->send(1, data_send_i.data(), sizeof(Ring) * seg_factor);
                       std::vector<Ring> data_recv_i(seg_factor);

                       network_->recv(1, data_recv_i.data(), sizeof(Ring) * seg_factor);

                       for(int j = 0; j<seg_factor; j++){
                            data_recv[i*seg_factor + j] = data_recv_i[j];
                       }
                    }

                    std::vector<Ring> data_send_last;
                    data_send_last.reserve(last_comm);
                    for(int j = 0; j<last_comm; j++){
                        data_send_last.push_back(data_send[num_comm*seg_factor + j]);
                    }
                    network_->send(1, data_send_last.data(), sizeof(Ring) * last_comm);
                    std::vector<Ring> data_recv_last(last_comm);

                    network_->recv(1, data_recv_last.data(), sizeof(Ring) * last_comm);
                    for(int j = 0; j<last_comm; j++){
                        data_recv[num_comm*seg_factor + j] = data_recv_last[j];
                    }
                    
                }

            for(int i = 0; i < mult_vals.size(); i++) {
                mult_vals[i] += data_recv[i];
            }
            for(int i = 0; i < and_vals.size(); i++) {
                
                and_vals[i] ^= data_recv[mult_vals.size() + i];
            }
            for(int i = 0; i < shuffle_vals.size(); i++) {
                shuffle_vals[i] = data_recv[mult_vals.size() + and_vals.size() + i];
            }
            for (int i = 0; i < reveal_vals.size(); i++) {
                reveal_vals[i] += data_recv[mult_vals.size() + and_vals.size() + shuffle_vals.size() + i];
            }
            evaluateGatesAtDepthPartyRecv(depth,
                                          mult_vals, and_vals, shuffle_vals, reveal_vals);
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
                if (circ_.output_bin[i]) {
                    outvals[i] = output_share_my[i] ^ outmask ;
                } else {
                    outvals[i] = output_share_my[i] + outmask ;
                }
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
        setInputs(inputs);
        for (size_t i = 0; i < circ_.gates_by_level.size(); ++i)
        {
            evaluateGatesAtDepth(i);
        }
        return getOutputs();

    }

   
}; // namespace graphsc
