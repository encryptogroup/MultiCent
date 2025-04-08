#include <vector>

#include <graphsc/offline_evaluator.h>
#include <graphsc/online_evaluator.h>
#include <utils/circuit.h>

#include "subcircuits.h"

std::vector<common::utils::wire_t> subcirc::applyPerm(std::vector<common::utils::wire_t> &perm, std::vector<common::utils::wire_t> &data,
                common::utils::Circuit<Ring> &circ, size_t &next_free_shuffle_id) {
    // See https://eprint.iacr.org/2022/1595.pdf
    auto shuffled_perm = circ.addParamWithOptMGate(common::utils::GateType::kShuffle, perm, next_free_shuffle_id);
    auto shuffled_data = circ.addParamWithOptMGate(common::utils::GateType::kShuffle, data, next_free_shuffle_id);
    next_free_shuffle_id++;
    auto clear_shuffled_perm = circ.addMGate(common::utils::GateType::kReveal, shuffled_perm);
    return circ.addMDoubleInGate(common::utils::GateType::kReorder, shuffled_data, clear_shuffled_perm);
}

std::vector<common::utils::wire_t> subcirc::compose(std::vector<common::utils::wire_t> &outer, std::vector<common::utils::wire_t> &inner,
                common::utils::Circuit<Ring> &circ, size_t &next_free_shuffle_id) {
    // See https://eprint.iacr.org/2022/1595.pdf
    auto shuffled_inner = circ.addParamWithOptMGate(common::utils::GateType::kShuffle, inner, next_free_shuffle_id);
    auto clear_shuffled_inner = circ.addMGate(common::utils::GateType::kReveal, shuffled_inner);
    auto shuffled_result = circ.addMDoubleInGate(common::utils::GateType::kReorderInverse, outer, clear_shuffled_inner);
    auto result = circ.addParamWithOptMGate(common::utils::GateType::kShuffle, shuffled_result, next_free_shuffle_id, true);
    next_free_shuffle_id++;
    return result;
}

std::vector<common::utils::wire_t> optimizedSortingIteration(   std::vector<common::utils::wire_t> &sigma,
                                                                std::vector<common::utils::wire_t> &keys,
                                                                common::utils::Circuit<Ring> &circ,
                                                                size_t &next_free_shuffle_id) {
    // Unoptimized would be:
    // auto sorted_bit_j = applyPerm(sigma, keys[j], circ, next_free_shuffle_id); // 20n + 12n 2 rounds
    // auto rho_j = circ.addMGate(common::utils::GateType::kGenCompaction, sorted_bit_j); // 4n + 8n 1 round
    // sigma = compose(rho_j, sigma, circ, next_free_shuffle_id);
    //              20n + 12n 3 rounds but 2 rounds start at beginning of loop already, so practically only 1
    // Total cost of 44n + 32n in 4 rounds
    // Instead, use optimization A.1.3 from https://eprint.iacr.org/2022/1595.pdf,
    // i.e., observe that applyPerm and compose will both permute and open sigma

    // 32n + 24n in 4 rounds

    size_t opt_shuffle_id = next_free_shuffle_id;
    next_free_shuffle_id++;

    // Apply current sigma to bit j
    auto shuffled_sigma = circ.addParamWithOptMGate(common::utils::GateType::kShuffle, sigma, opt_shuffle_id); // 12n + 4n 1 round
    auto sorted_bit_j = circ.addParamWithOptMGate(common::utils::GateType::kShuffle, keys, opt_shuffle_id); // 8n + 4n same round
    shuffled_sigma = circ.addMGate(common::utils::GateType::kReveal, shuffled_sigma); // 0 + 4n 1 round
    sorted_bit_j = circ.addMDoubleInGate(common::utils::GateType::kReorder, sorted_bit_j, shuffled_sigma); // local

    // Compute permutation rho_j to stable sort sorted_bit_j
    auto rho_j = circ.addMGate(common::utils::GateType::kGenCompaction, sorted_bit_j); // 4n + 8n 1 round

    // Return new sigma = rho_j * sigma
    auto shuffled_result = circ.addMDoubleInGate(common::utils::GateType::kReorderInverse, rho_j, shuffled_sigma); // local
    return circ.addParamWithOptMGate(common::utils::GateType::kShuffle, shuffled_result, opt_shuffle_id, true); // 8n + 4n 1 round
}

std::vector<common::utils::wire_t> subcirc::genSortingPerm(std::vector<std::vector<common::utils::wire_t>> &keys,
                                                    common::utils::Circuit<Ring> &circ, size_t &next_free_shuffle_id, size_t nmbr_bits) {
    // See https://eprint.iacr.org/2022/1595.pdf

    // 32*nmbr_bits*n - 28n setup
    // 24*nmbr_bits*n - 16n online
    // 4*nmbr_bits - 3 rounds

    // LSB (bit 0):
    // Compute permutation rho_0 to stable sort LSBs
    auto rho_0 = circ.addMGate(common::utils::GateType::kGenCompaction, keys[0]); // 4n + 8n 1 round
    // Output permutation sigma currently is rho_0
    auto sigma = rho_0;

    for (size_t j = 1; j < nmbr_bits; j++) {
        // bit j:
        sigma = optimizedSortingIteration(sigma, keys[j], circ, next_free_shuffle_id); // 32n + 24n in 4 rounds
    }

    return sigma;
}

std::tuple< std::vector<common::utils::wire_t>,size_t,
            std::vector<common::utils::wire_t>,size_t,
            std::vector<common::utils::wire_t>,size_t> prepare_message_passing_shufflings(
                                                    std::vector<common::utils::wire_t> &perm_vertex,
                                                    std::vector<common::utils::wire_t> &perm_source,
                                                    std::vector<common::utils::wire_t> &perm_dest,
                                                    common::utils::Circuit<Ring> &circ,
                                                    size_t &next_free_shuffle_id) {
    // 36t + 24t in 2 rounds
    // Shuffle ID-s for going to the different orderings, will be reused in each iteration
    size_t shuffle_vertex = next_free_shuffle_id;
    size_t shuffle_source = next_free_shuffle_id + 1;
    size_t shuffle_dest = next_free_shuffle_id + 2;
    next_free_shuffle_id += 3;
    // Prepare shufflings
    auto shuffled_perm_vertex = circ.addParamWithOptMGate(common::utils::GateType::kShuffle, perm_vertex, shuffle_vertex); // 12t + 4t in 1 round
    auto clear_shuffled_perm_vertex = circ.addMGate(common::utils::GateType::kReveal, shuffled_perm_vertex); // 0 + 4t in 1 round
    auto shuffled_perm_source = circ.addParamWithOptMGate(common::utils::GateType::kShuffle, perm_source, shuffle_source); // 12t + 4t in 1 round, but during previous
    auto clear_shuffled_perm_source = circ.addMGate(common::utils::GateType::kReveal, shuffled_perm_source); // 0 + 4t in 1 round, but during previous
    auto shuffled_perm_dest = circ.addParamWithOptMGate(common::utils::GateType::kShuffle, perm_dest, shuffle_dest); // 12t + 4t in 1 round, but during previous
    auto clear_shuffled_perm_dest = circ.addMGate(common::utils::GateType::kReveal, shuffled_perm_dest); // 0 + 4t in 1 round, but during previous

    return {clear_shuffled_perm_vertex, shuffle_vertex, clear_shuffled_perm_source, shuffle_source, clear_shuffled_perm_dest, shuffle_dest};
}

std::vector<common::utils::wire_t> run_message_passing( std::vector<common::utils::wire_t> &clear_shuffled_perm_vertex,
                                                        size_t shuffle_vertex,
                                                        std::vector<common::utils::wire_t> &clear_shuffled_perm_source,
                                                        size_t shuffle_source,
                                                        std::vector<common::utils::wire_t> &clear_shuffled_perm_dest,
                                                        size_t shuffle_dest,
                                                        std::vector<common::utils::wire_t> &payload,
                                                        common::utils::Circuit<Ring> &circ,
                                                        size_t &next_free_shuffle_id,
                                                        size_t n,
                                                        std::vector<Ring> &weights) {

    // 32t*D + 32t setup
    // 16t*D + 4t online
    // 3*D rounds

    size_t dshuffle_v_to_s = next_free_shuffle_id;
    size_t dshuffle_s_to_d = next_free_shuffle_id + 1;
    size_t dshuffle_d_to_v = next_free_shuffle_id + 2;
    next_free_shuffle_id += 3;

    // Go to vertex order
    auto shuffled_payload = circ.addParamWithOptMGate(common::utils::GateType::kShuffle, payload, shuffle_vertex); // 8t + 4t in 1 round, but during previous during prepare_message_passing_shufflings()
    auto payload_vertex = circ.addMDoubleInGate(common::utils::GateType::kReorder, shuffled_payload, clear_shuffled_perm_vertex); // local
    for (size_t i = 0; i < weights.size(); i++) {
        // Total: 32t(+once 24t) + 16t in 3 rounds
        // Add next weights to vertices before sending data
        payload_vertex = circ.addTwoParamMGate(common::utils::GateType::kAddConstToVec, payload_vertex, weights[weights.size() - 1 - i], n); // local
        // Start propagate
        auto payload_vertex_prop = circ.addParamMGate(common::utils::GateType::kPreparePropagate, payload_vertex, n); // local
        // bring back to standard order and then go to source order
        shuffled_payload = circ.addMDoubleInGate(common::utils::GateType::kReorderInverse, payload_vertex_prop, clear_shuffled_perm_vertex); // local
        auto shuffled_payload_correction = circ.addMDoubleInGate(common::utils::GateType::kReorderInverse, payload_vertex, clear_shuffled_perm_vertex); // local
        shuffled_payload = circ.addThreeParamMGate(common::utils::GateType::kDoubleShuffle, shuffled_payload, dshuffle_v_to_s, shuffle_vertex, shuffle_source); // 16t(first iter)/8t(else) + 4t in 1 round
        shuffled_payload_correction = circ.addThreeParamMGate(common::utils::GateType::kDoubleShuffle, shuffled_payload_correction, dshuffle_v_to_s, shuffle_vertex, shuffle_source); // 8t + 4t in 1 round, but during previous
        auto payload_source = circ.addMDoubleInGate(common::utils::GateType::kReorder, shuffled_payload, clear_shuffled_perm_source); // local
        auto payload_correction = circ.addMDoubleInGate(common::utils::GateType::kReorder, shuffled_payload_correction, clear_shuffled_perm_source); // local
        // Finalize propagate
        payload_source = circ.addMDoubleInGate(common::utils::GateType::kPropagate, payload_source, payload_correction); // local
        // bring back to standard order and then go to destination order
        shuffled_payload = circ.addMDoubleInGate(common::utils::GateType::kReorderInverse, payload_source, clear_shuffled_perm_source); // local
        shuffled_payload = circ.addThreeParamMGate(common::utils::GateType::kDoubleShuffle, shuffled_payload, dshuffle_s_to_d, shuffle_source, shuffle_dest); // 16t(first iter)/8t(else) + 4t in 1 round
        auto payload_dest = circ.addMDoubleInGate(common::utils::GateType::kReorder, shuffled_payload, clear_shuffled_perm_dest); // local
        // Start gather
        payload_dest = circ.addMGate(common::utils::GateType::kPrepareGather, payload_dest); // local
        // bring back to standard order and then go to vertex order
        shuffled_payload = circ.addMDoubleInGate(common::utils::GateType::kReorderInverse, payload_dest, clear_shuffled_perm_dest); // local
        shuffled_payload = circ.addThreeParamMGate(common::utils::GateType::kDoubleShuffle, shuffled_payload, dshuffle_d_to_v, shuffle_dest, shuffle_vertex); // 16t(first iter)/8t(else) + 4t in 1 round
        payload_vertex = circ.addMDoubleInGate(common::utils::GateType::kReorder, shuffled_payload, clear_shuffled_perm_vertex); // local
        // Finalize Gather
        payload_vertex = circ.addParamMGate(common::utils::GateType::kGather, payload_vertex, n); // local
    }

    return payload_vertex;
}

std::vector<common::utils::wire_t> run_message_passing_bfs( std::vector<common::utils::wire_t> &clear_shuffled_perm_vertex,
                                                            size_t shuffle_vertex,
                                                            std::vector<common::utils::wire_t> &clear_shuffled_perm_source,
                                                            size_t shuffle_source,
                                                            std::vector<common::utils::wire_t> &clear_shuffled_perm_dest,
                                                            size_t shuffle_dest,
                                                            size_t dshuffle_v_to_s,
                                                            size_t dshuffle_s_to_d,
                                                            size_t dshuffle_d_to_v,
                                                            std::vector<common::utils::wire_t> &payload,
                                                            common::utils::Circuit<Ring> &circ,
                                                            size_t n,
                                                            size_t depth) {

    // 32t*D + 32t setup (24t less if doubleshuffles were used before)
    // 16t*D + 4t online
    // 3*D rounds

    // Go to vertex order
    auto shuffled_payload = circ.addParamWithOptMGate(common::utils::GateType::kShuffle, payload, shuffle_vertex); // 8t + 4t in 1 round, but during previous during prepare_message_passing_shufflings()
    auto payload_vertex = circ.addMDoubleInGate(common::utils::GateType::kReorder, shuffled_payload, clear_shuffled_perm_vertex); // local
    for (size_t i = 0; i < depth; i++) {
        // Total: 32t(+once 24t) + 16t in 3 rounds
        auto prior_payload_vertex = payload_vertex;
        // Start propagate
        auto payload_vertex_prop = circ.addParamMGate(common::utils::GateType::kPreparePropagate, payload_vertex, n); // local
        // bring back to standard order and then go to source order
        shuffled_payload = circ.addMDoubleInGate(common::utils::GateType::kReorderInverse, payload_vertex_prop, clear_shuffled_perm_vertex); // local
        auto shuffled_payload_correction = circ.addMDoubleInGate(common::utils::GateType::kReorderInverse, payload_vertex, clear_shuffled_perm_vertex); // local
        shuffled_payload = circ.addThreeParamMGate(common::utils::GateType::kDoubleShuffle, shuffled_payload, dshuffle_v_to_s, shuffle_vertex, shuffle_source); // 16t(first iter)/8t(else) + 4t in 1 round
        shuffled_payload_correction = circ.addThreeParamMGate(common::utils::GateType::kDoubleShuffle, shuffled_payload_correction, dshuffle_v_to_s, shuffle_vertex, shuffle_source); // 8t + 4t in 1 round, but during previous
        auto payload_source = circ.addMDoubleInGate(common::utils::GateType::kReorder, shuffled_payload, clear_shuffled_perm_source); // local
        auto payload_correction = circ.addMDoubleInGate(common::utils::GateType::kReorder, shuffled_payload_correction, clear_shuffled_perm_source); // local
        // Finalize propagate
        payload_source = circ.addMDoubleInGate(common::utils::GateType::kPropagate, payload_source, payload_correction); // local
        // bring back to standard order and then go to destination order
        shuffled_payload = circ.addMDoubleInGate(common::utils::GateType::kReorderInverse, payload_source, clear_shuffled_perm_source); // local
        shuffled_payload = circ.addThreeParamMGate(common::utils::GateType::kDoubleShuffle, shuffled_payload, dshuffle_s_to_d, shuffle_source, shuffle_dest); // 16t(first iter)/8t(else) + 4t in 1 round
        auto payload_dest = circ.addMDoubleInGate(common::utils::GateType::kReorder, shuffled_payload, clear_shuffled_perm_dest); // local
        // Start gather
        payload_dest = circ.addMGate(common::utils::GateType::kPrepareGather, payload_dest); // local
        // bring back to standard order and then go to vertex order
        shuffled_payload = circ.addMDoubleInGate(common::utils::GateType::kReorderInverse, payload_dest, clear_shuffled_perm_dest); // local
        shuffled_payload = circ.addThreeParamMGate(common::utils::GateType::kDoubleShuffle, shuffled_payload, dshuffle_d_to_v, shuffle_dest, shuffle_vertex); // 16t(first iter)/8t(else) + 4t in 1 round
        payload_vertex = circ.addMDoubleInGate(common::utils::GateType::kReorder, shuffled_payload, clear_shuffled_perm_vertex); // local
        // Finalize Gather
        payload_vertex = circ.addParamMGate(common::utils::GateType::kGather, payload_vertex, n); // local
        // Add up with prior payload, node that had been reached remains reached
        payload_vertex = circ.addMDoubleInGate(common::utils::GateType::kAddVec, payload_vertex, prior_payload_vertex);
    }

    return payload_vertex;
}

std::vector<common::utils::wire_t> subcirc::pi_3(   std::vector<std::vector<common::utils::wire_t>> &source_bits,
                                                    std::vector<std::vector<common::utils::wire_t>> &destination_bits,
                                                    std::vector<common::utils::wire_t>              &vertex_flags,
                                                    std::vector<common::utils::wire_t>              &payload,
                                                    common::utils::Circuit<Ring> &circ, 
                                                    size_t &next_free_shuffle_id, 
                                                    size_t n,
                                                    size_t nmbr_bits,
                                                    std::vector<Ring> &weights) {

    // Need sorting by source bits and appended (LSB) flipped vertex flags so that a vertex comes before its outgoing edges
    std::vector<common::utils::wire_t> flipped_vertex_flags = circ.addMGate(common::utils::GateType::kFlip, vertex_flags); // local

    // genSortingPerm cannot be expanded in first iteration unfortunately, so do manually:
    // (32t*nmrb_bits + 4t) + (24t*nmbr_bits + 8t) in 4*nmbr_bits + 1 rounds
    auto rho_0_source = circ.addMGate(common::utils::GateType::kGenCompaction, flipped_vertex_flags);
    auto perm_source = rho_0_source;
    for (size_t j = 0; j < nmbr_bits; j++) {
        perm_source = optimizedSortingIteration(perm_source, source_bits[j], circ, next_free_shuffle_id);
    }

    // Need sorting by destination bits and appended (LSB) vertex flags so that a vertex comes after its incoming edges
    // genSortingPerm cannot be expanded in first iteration unfortunately, so do manually:
    // (32t*nmrb_bits + 4t) + (24t*nmbr_bits + 8t) in 4*nmbr_bits + 1 rounds, but during previous (both run in parallel)
    auto rho_0_dest = circ.addMGate(common::utils::GateType::kGenCompaction, vertex_flags);
    auto perm_dest = rho_0_dest;
    for (size_t j = 0; j < nmbr_bits; j++) {
        perm_dest = optimizedSortingIteration(perm_dest, destination_bits[j], circ, next_free_shuffle_id);
    }

    // For vertex order, go from source order that already has vertices in ascending order and then compact by flipped vertex flags
    // Works as the used radix sort/compaction is stable
    // 32t + 24t in 4 rounds
    auto perm_vertex = optimizedSortingIteration(perm_dest, flipped_vertex_flags, circ, next_free_shuffle_id);

    // Shuffle ID-s for going to the different orderings, will be reused in each iteration
    // 36t + 24t in 2 rounds
    auto [clear_shuffled_perm_vertex, shuffle_vertex, clear_shuffled_perm_source, shuffle_source, clear_shuffled_perm_dest, shuffle_dest]
            = prepare_message_passing_shufflings(perm_vertex, perm_source, perm_dest, circ, next_free_shuffle_id);

    // 32t*D + 32t setup
    // 16t*D + 4t online
    // 3*D rounds
    return run_message_passing( clear_shuffled_perm_vertex, shuffle_vertex,
                                clear_shuffled_perm_source, shuffle_source,
                                clear_shuffled_perm_dest, shuffle_dest,
                                payload,
                                circ, next_free_shuffle_id,
                                n, weights);
}

std::vector<common::utils::wire_t> subcirc::pi_3_reference( std::vector<std::vector<std::vector<common::utils::wire_t>>> &adj_matrices,
                                                            common::utils::Circuit<Ring> &circ,
                                                            size_t n,
                                                            std::vector<Ring> &weights) {
    std::vector<std::vector<common::utils::wire_t>> B;
    for (size_t i = 0; i < n; i++) {
        B.push_back(std::vector<common::utils::wire_t>());
        for (size_t j = 0; j < n; j++) {
            auto accu = adj_matrices[0][i][j];
            for (size_t l = 1; l < adj_matrices.size(); l++) {
                accu = circ.addGate(common::utils::GateType::kAdd, accu, adj_matrices[l][i][j]);
            }
            B[i].push_back(accu);
        }
    }
    std::vector<common::utils::wire_t> s;
    std::vector<common::utils::wire_t> s_accu;
    for (size_t i = 0; i < n; i++) {
        auto accu = B[i][0];
        for (size_t j = 1; j < n; j++) {
            accu = circ.addGate(common::utils::GateType::kAdd, accu, B[i][j]);
        }
        s.push_back(accu);
        s_accu.push_back(circ.addConstOpGate(common::utils::GateType::kConstMul, s[i], weights[0]));
    }
    for (size_t k = 1; k < weights.size(); k++) {
        std::vector<std::vector<common::utils::wire_t>> products;
        for (size_t i = 0; i < n; i++) {
            products.push_back(std::vector<common::utils::wire_t>());
            for (size_t j = 0; j < n; j++) {
                products[i].push_back(circ.addGate(common::utils::GateType::kMul, B[i][j], s[j]));
            }
        }
        for (size_t i = 0; i < n; i++) {
            auto accu = products[i][0];
            for (size_t j = 1; j < n; j++) {
                accu = circ.addGate(common::utils::GateType::kAdd, accu, products[i][j]);
            }
            s[i] = accu;
            auto with_weight =  circ.addConstOpGate(common::utils::GateType::kConstMul, s[i], weights[k]);
            s_accu[i] = circ.addGate(common::utils::GateType::kAdd, s_accu[i], with_weight);
        }
    }
    return s_accu;
}

std::vector<common::utils::wire_t> subcirc::pi_2(   std::vector<std::vector<common::utils::wire_t>> &source_bits,
                                                    std::vector<std::vector<common::utils::wire_t>> &destination_bits,
                                                    std::vector<common::utils::wire_t>              &vertex_flags,
                                                    std::vector<common::utils::wire_t>              &payload,
                                                    common::utils::Circuit<Ring> &circ, 
                                                    size_t &next_free_shuffle_id, 
                                                    size_t n,
                                                    size_t nmbr_bits,
                                                    std::vector<Ring> &weights) {

    // Approach: We reduce to pi_3 by identifying duplicates and setting the appending a bit to all sources and destinations
    // as MSB so that these are edges (x,x,0) corresponding to no vertex that will be at the back of the list in source,
    // destination and vertex ordering so that they do not influence any of the other data.

    // First, we sort everything by destination, then source, so that duplicates are in one block

    /*
    Optimization:
    If we first sort by destination, we can reuse this sort later for pi_3 too.
    For that, we now also have to first sort by the vertex flags which is required later in pi_3.

    Further idea: We could then to a vertex compaction again before sorting source.
    Then, the result would be sourcesort * destsort.
    We can then also extract sourcesort by computing sourcesort * destsort * destsort^(-1).
    But currently no idea how to invert permutations, does not directly follow from the techniques presented
    in https://eprint.iacr.org/2022/1595.pdf
    */
    
    // (32t*nmrb_bits + 4t) + (24t*nmbr_bits + 8t) in 4*nmbr_bits + 1 rounds
    auto rho_0_dest = circ.addMGate(common::utils::GateType::kGenCompaction, vertex_flags);
    auto perm = rho_0_dest;
    for (size_t j = 0; j < nmbr_bits; j++) {
        perm = optimizedSortingIteration(perm, destination_bits[j], circ, next_free_shuffle_id);
    }
    auto perm_dest = perm;
    // Continue from here with source sorting
    // (32t*nmrb_bits) + (24t*nmbr_bits) in 4*nmbr_bits rounds
    for (size_t j = 0; j < nmbr_bits; j++) {
        perm = optimizedSortingIteration(perm, source_bits[j], circ, next_free_shuffle_id);
    }

    // Compose all source and dest elements to single arithmetic sharings
    // non-interactive
    std::vector<common::utils::wire_t> source_composed, destination_composed;
    for (size_t j = 0; j < payload.size(); j++) {
        std::vector<common::utils::wire_t> src_bits, dest_bits;
        for (size_t k = 0; k < nmbr_bits; k++) {
            src_bits.push_back(source_bits[k][j]);
            dest_bits.push_back(destination_bits[k][j]);
        }
        source_composed.push_back(circ.addMSingleOutGate(common::utils::GateType::kCompose, src_bits));
        destination_composed.push_back(circ.addMSingleOutGate(common::utils::GateType::kCompose, dest_bits));
    }

    // Apply sorting to composed elements (don't call applyPerm as we do this joined for two vectors for higher efficiency)
    // 28t + 16t in 2 rounds
    perm = circ.addParamWithOptMGate(common::utils::GateType::kShuffle, perm, next_free_shuffle_id); // 12t + 4t in 1 round
    perm = circ.addMGate(common::utils::GateType::kReveal, perm); // 0 + 4t in 1 round
    source_composed = circ.addParamWithOptMGate(common::utils::GateType::kShuffle, source_composed, next_free_shuffle_id); // 8t + 4t in parallel
    destination_composed = circ.addParamWithOptMGate(common::utils::GateType::kShuffle, destination_composed, next_free_shuffle_id); // 8t + 4t in parallel
    source_composed = circ.addMDoubleInGate(common::utils::GateType::kReorder, source_composed, perm); // local
    destination_composed = circ.addMDoubleInGate(common::utils::GateType::kReorder, destination_composed, perm); // local

    // Find duplicates, set vector entry to 1 if corresponding entry equals the prior one
    std::vector<common::utils::wire_t> duplicate(payload.size());
    duplicate[0] = circ.addConstOpGate(common::utils::GateType::kConstMul, payload[0], 0); // Just set to 0 as this is no duplicate
    // (48t - 48) + (96t - 96) in 7 rounds
    for (size_t j = 1; j < payload.size(); j++) {
        // per parallel iter: 48 + 96 in 7 rounds

        // Equal prior iff source[j] - source[j-1] == 0 and dest[j] - dest[j-1] == 0
        auto src_dupl = circ.addGate(common::utils::GateType::kSub, source_composed[j], source_composed[j - 1]);
        auto dest_dupl = circ.addGate(common::utils::GateType::kSub, destination_composed[j], destination_composed[j - 1]);
        // Multi-layer EQZ approach
        src_dupl = circ.addParamGate(common::utils::GateType::kEqualsZero, src_dupl, 0); // 4 + 8 in 1 round (inefficient bit packing)
        src_dupl = circ.addParamGate(common::utils::GateType::kEqualsZero, src_dupl, 1); // 4 + 8 in 1 round (inefficient bit packing)
        src_dupl = circ.addParamGate(common::utils::GateType::kEqualsZero, src_dupl, 2); // 4 + 8 in 1 round (inefficient bit packing)
        src_dupl = circ.addParamGate(common::utils::GateType::kEqualsZero, src_dupl, 3); // 4 + 8 in 1 round (inefficient bit packing)
        src_dupl = circ.addParamGate(common::utils::GateType::kEqualsZero, src_dupl, 4); // 4 + 8 in 1 round (inefficient bit packing)
        dest_dupl = circ.addParamGate(common::utils::GateType::kEqualsZero, dest_dupl, 0); // 4 + 8 in parallel (inefficient bit packing)
        dest_dupl = circ.addParamGate(common::utils::GateType::kEqualsZero, dest_dupl, 1); // 4 + 8 in parallel (inefficient bit packing)
        dest_dupl = circ.addParamGate(common::utils::GateType::kEqualsZero, dest_dupl, 2); // 4 + 8 in parallel (inefficient bit packing)
        dest_dupl = circ.addParamGate(common::utils::GateType::kEqualsZero, dest_dupl, 3); // 4 + 8 in parallel (inefficient bit packing)
        dest_dupl = circ.addParamGate(common::utils::GateType::kEqualsZero, dest_dupl, 4); // 4 + 8 in parallel (inefficient bit packing)
        // final AND and conversion
        auto dupl_bin = circ.addGate(common::utils::GateType::kAnd, src_dupl, dest_dupl); // 4 + 8 in 1 round (inefficient bit packing)
        duplicate[j] = circ.addGate(common::utils::GateType::kConvertB2A, dupl_bin); // 4 + 8 in 1 round
    }

    // We now append duplicate as MSB to all src and dest so that duplicates always end up at the end with invalid vertex ids
    // First, need to bring into original order
    // 8t + 4t in 1 round
    duplicate = circ.addMDoubleInGate(common::utils::GateType::kReorderInverse, duplicate, perm);
    duplicate = circ.addParamWithOptMGate(common::utils::GateType::kShuffle, duplicate, next_free_shuffle_id, true); // 8t + 4t in 1 round
    next_free_shuffle_id++;

    // Now, append MSB
    source_bits.push_back(duplicate);
    destination_bits.push_back(duplicate);

    // So far:
    // 32t*nmrb_bits + 4t + 32t*nmrb_bits + 28t + 48t - 48 + 8t = 64t*nmrb_bits + 88t - 48 setup
    // 24t*nmbr_bits + 8t + 24t*nmbr_bits + 16t + 96t - 96 + 4t = 48t*nmbr_bits + 124t - 96 online
    // 8 * nmbr_bits + 11 rounds

    // The following is a copy of pi_3 with some changes given that perm_dest was already computed

    // Need sorting by source bits and appended (LSB) flipped vertex flags so that a vertex comes before its outgoing edges
    std::vector<common::utils::wire_t> flipped_vertex_flags = circ.addMGate(common::utils::GateType::kFlip, vertex_flags); // local
    // genSortingPerm cannot be expanded in first iteration unfortunately, so do manually:
    // (32t*nmbr_bits + 36t) + (24t*nmbr_bits + 32t) in 4*nmbr_bits + 5 rounds
    // BUT: all but last iteration already before in parallel, i.e., just 4 rounds remaining
    // AND: perm_source as input known earlier, so 1 round of optimizedSortingIteration() already earlier => 3 rounds
    auto rho_0_source = circ.addMGate(common::utils::GateType::kGenCompaction, flipped_vertex_flags); // 4t + 8t 1 round
    auto perm_source = rho_0_source;
    for (size_t j = 0; j < nmbr_bits + 1; j++) {
        perm_source = optimizedSortingIteration(perm_source, source_bits[j], circ, next_free_shuffle_id); // 32t + 24t in 4 rounds
    }

    // Need sorting by destination bits and appended (LSB) vertex flags so that a vertex comes after its incoming edges
    // CHANGE: For that, we use perm_dest and just include the new MSB now (j = nmbr_bits)
    // 32t + 24t in 4 rounds, but during previous
    perm_dest = optimizedSortingIteration(perm_dest, destination_bits[nmbr_bits], circ, next_free_shuffle_id);

    // For vertex order, go from source order that already has vertices in ascending order and then compact by flipped vertex flags
    // Works as the used radix sort/compaction is stable
    // 32t + 24t in 4 rounds
    auto perm_vertex = optimizedSortingIteration(perm_dest, flipped_vertex_flags, circ, next_free_shuffle_id);

    // Shuffle ID-s for going to the different orderings, will be reused in each iteration
    // 36t + 24t in 2 rounds
    auto [clear_shuffled_perm_vertex, shuffle_vertex, clear_shuffled_perm_source, shuffle_source, clear_shuffled_perm_dest, shuffle_dest]
            = prepare_message_passing_shufflings(perm_vertex, perm_source, perm_dest, circ, next_free_shuffle_id);

    // 32t*D + 32t setup
    // 16t*D + 4t online
    // 3*D rounds
    return run_message_passing( clear_shuffled_perm_vertex, shuffle_vertex,
                                clear_shuffled_perm_source, shuffle_source,
                                clear_shuffled_perm_dest, shuffle_dest,
                                payload,
                                circ, next_free_shuffle_id,
                                n, weights);
}

common::utils::wire_t mult_tree(std::vector<common::utils::wire_t> &vals,
                                common::utils::Circuit<Ring> &circ,
                                size_t first_layer, size_t int_length) {
    // 4*int_length-4 + 8*int_length-8 bytes in ceil(log2(int_length)) rounds
    assert(int_length >= 1);
    if (int_length == 1) {
        return vals[first_layer];
    } else {
        size_t first_half = int_length / 2;
        size_t second_half = int_length - first_half;
        auto res_first = mult_tree(vals, circ, first_layer, first_half);
        auto res_second = mult_tree(vals, circ, first_layer + first_half, second_half);
        return circ.addGate(common::utils::GateType::kMul, res_first, res_second);
    }
}

std::vector<common::utils::wire_t> subcirc::pi_2_reference( std::vector<std::vector<std::vector<common::utils::wire_t>>> &adj_matrices,
                                                            common::utils::Circuit<Ring> &circ,
                                                            size_t n,
                                                            std::vector<Ring> &weights) {
    std::vector<std::vector<common::utils::wire_t>> A;
    for (size_t i = 0; i < n; i++) {
        A.push_back(std::vector<common::utils::wire_t>());
        for (size_t j = 0; j < n; j++) {
            // Compute OR over all = NOT(AND over all (NOT x))
            // OT x
            std::vector<common::utils::wire_t> inputs;
            for (size_t l = 0; l < adj_matrices.size(); l++) {
                inputs.push_back(adj_matrices[l][i][j]);
            }
            inputs = circ.addMGate(common::utils::GateType::kFlip, inputs);
            // AND(NOT x)
            // 4*l-4 + 8*l-8 bytes in ceil(log2(l)) rounds
            auto res = mult_tree(inputs, circ, 0, adj_matrices.size());
            // NOT(AND(NOT x))
            std::vector<common::utils::wire_t> as_vec;
            as_vec.push_back(res);
            A[i].push_back(circ.addMGate(common::utils::GateType::kFlip, as_vec)[0]);
        }
    }

    std::vector<std::vector<std::vector<common::utils::wire_t>>> wrapper;
    wrapper.push_back(A);
    // 4*n^2*(D-1) bytes setup
    // 8*n^2*(D-1) bytes online
    // D - 1 rounds
    return pi_3_reference(wrapper, circ, n, weights);
}

std::vector<common::utils::wire_t> subcirc::pi_1(   std::vector<std::vector<common::utils::wire_t>> &source_bits,
                                                    std::vector<std::vector<common::utils::wire_t>> &destination_bits,
                                                    std::vector<common::utils::wire_t>              &vertex_flags,
                                                    std::vector<std::vector<common::utils::wire_t>> &payload,
                                                    common::utils::Circuit<Ring> &circ, 
                                                    size_t &next_free_shuffle_id, 
                                                    size_t n,
                                                    size_t nmbr_bits,
                                                    size_t depth) {
    // Need sorting by source bits and appended (LSB) flipped vertex flags so that a vertex comes before its outgoing edges
    std::vector<common::utils::wire_t> flipped_vertex_flags = circ.addMGate(common::utils::GateType::kFlip, vertex_flags); // local
    // genSortingPerm cannot be expanded in first iteration unfortunately, so do manually:
    // (32t*nmrb_bits + 4t) + (24t*nmbr_bits + 8t) in 4*nmbr_bits + 1 rounds
    auto rho_0_source = circ.addMGate(common::utils::GateType::kGenCompaction, flipped_vertex_flags);
    auto perm_source = rho_0_source;
    for (size_t j = 0; j < nmbr_bits; j++) {
        perm_source = optimizedSortingIteration(perm_source, source_bits[j], circ, next_free_shuffle_id);
    }

    // Need sorting by destination bits and appended (LSB) vertex flags so that a vertex comes after its incoming edges
    // genSortingPerm cannot be expanded in first iteration unfortunately, so do manually:
    // (32t*nmrb_bits + 4t) + (24t*nmbr_bits + 8t) in 4*nmbr_bits + 1 rounds, but during previous
    auto rho_0_dest = circ.addMGate(common::utils::GateType::kGenCompaction, vertex_flags);
    auto perm_dest = rho_0_dest;
    for (size_t j = 0; j < nmbr_bits; j++) {
        perm_dest = optimizedSortingIteration(perm_dest, destination_bits[j], circ, next_free_shuffle_id);
    }

    // For vertex order, go from source order that already has vertices in ascending order and then compact by flipped vertex flags
    // Works as the used radix sort/compaction is stable
    // 32t + 24t in 4 rounds
    auto perm_vertex = optimizedSortingIteration(perm_dest, flipped_vertex_flags, circ, next_free_shuffle_id);

    // Shuffle ID-s for going to the different orderings, will be reused in each iteration
    // 36t + 24t in 2 round
    auto [clear_shuffled_perm_vertex, shuffle_vertex, clear_shuffled_perm_source, shuffle_source, clear_shuffled_perm_dest, shuffle_dest]
            = prepare_message_passing_shufflings(perm_vertex, perm_source, perm_dest, circ, next_free_shuffle_id);

    // Up to here:
    // 64t*nmbr_bits + 76t setup
    // 48t*nmbr_bits + 64t online
    // 4*nmbr_bits + 7 rounds

    // Block below is:
    // 24n^2 + 32tnD + 8tn + 24t setup
    // 48n^2 + 16tnD + 4tn online
    // 3D + 6 rounds

    // Now, in parallel, run n instances of message passing for all the different vectors of payloads.
    size_t dshuffle_v_to_s = next_free_shuffle_id;
    size_t dshuffle_s_to_d = next_free_shuffle_id + 1;
    size_t dshuffle_d_to_v = next_free_shuffle_id + 2;
    next_free_shuffle_id += 3;
    std::vector<common::utils::wire_t> out(n);
    for (size_t j = 0; j < n; j++) {
        // 32t*D + 32t setup (24t less if doubleshuffles were used before)
        // 16t*D + 4t online
        // 3*D rounds
        auto payload_from_n = run_message_passing_bfs(  clear_shuffled_perm_vertex, shuffle_vertex,
                                                        clear_shuffled_perm_source, shuffle_source,
                                                        clear_shuffled_perm_dest, shuffle_dest,
                                                        dshuffle_v_to_s, dshuffle_s_to_d, dshuffle_d_to_v,
                                                        payload[j],
                                                        circ,
                                                        n, depth);
        // Now, clip all values for nodes to [0;1]
        // First, set to 1 where value was 0, otherwise 0
        // 24n + 48n in 6 rounds
        for(size_t k = 0; k < n; k++) {
            auto eqz = circ.addParamGate(common::utils::GateType::kEqualsZero, payload_from_n[k], 0); // 4 + 8 in 1 round (inefficient bit packing)
            eqz = circ.addParamGate(common::utils::GateType::kEqualsZero, eqz, 1); // 4 + 8 in 1 round (inefficient bit packing)
            eqz = circ.addParamGate(common::utils::GateType::kEqualsZero, eqz, 2); // 4 + 8 in 1 round (inefficient bit packing)
            eqz = circ.addParamGate(common::utils::GateType::kEqualsZero, eqz, 3); // 4 + 8 in 1 round (inefficient bit packing)
            eqz = circ.addParamGate(common::utils::GateType::kEqualsZero, eqz, 4); // 4 + 8 in 1 round (inefficient bit packing)
            payload_from_n[k] = circ.addGate(common::utils::GateType::kConvertB2A, eqz); // 4 + 8 in 1 round
        }
        // Flip 0/1
        payload_from_n = circ.addMGate(common::utils::GateType::kFlip, payload_from_n);
        // Get number of reached nodes = sum over payload_from_n for all nodes
        auto accu = payload_from_n[0];
        for (size_t k = 1; k < n; k++) 
            accu = circ.addGate(common::utils::GateType::kAdd, accu, payload_from_n[k]);
        out[j] = accu;
    }
    return out;
}

std::vector<common::utils::wire_t> subcirc::pi_1_reference( std::vector<std::vector<std::vector<common::utils::wire_t>>> &adj_matrices,
                                                            common::utils::Circuit<Ring> &circ,
                                                            size_t n,
                                                            size_t depth) {
    assert(depth > 0);
    std::vector<std::vector<common::utils::wire_t>> B;
    std::vector<std::vector<common::utils::wire_t>> Bk;
    std::vector<std::vector<common::utils::wire_t>> Bsum;
    for (size_t i = 0; i < n; i++) {
        B.push_back(std::vector<common::utils::wire_t>());
        Bk.push_back(std::vector<common::utils::wire_t>());
        Bsum.push_back(std::vector<common::utils::wire_t>());
        for (size_t j = 0; j < n; j++) {
            auto accu = adj_matrices[0][i][j];
            for (size_t l = 1; l < adj_matrices.size(); l++) {
                accu = circ.addGate(common::utils::GateType::kAdd, accu, adj_matrices[l][i][j]);
            }
            B[i].push_back(accu);
            Bk[i].push_back(accu);
            Bsum[i].push_back(accu);
        }
    }

    // 4n^3(D-1) + 8n^3(D-1) in (D-1) rounds
    for (size_t k = 1; k < depth; k++) {
        std::vector<std::vector<common::utils::wire_t>> new_Bk(n);
        // n^3 multiplications that are 4 + 8 bytes each, single round total
        for (size_t u = 0; u < n; u++) {
            for (size_t v = 0; v < n; v++) {
                common::utils::wire_t accu;
                for (size_t w = 0; w < n; w++) {
                    auto prod = circ.addGate(common::utils::GateType::kMul, Bk[u][w], B[w][v]);
                    if (w == 0)
                        accu = prod;
                    else
                        accu = circ.addGate(common::utils::GateType::kAdd, accu, prod);
                }
                new_Bk[u].push_back(accu);
                Bsum[u][v] = circ.addGate(common::utils::GateType::kAdd, Bsum[u][v], accu);
            }
        }
        Bk = new_Bk;
    }

    // 24n^2 + 48n^2 in 6 rounds
    for (size_t u = 0; u < n; u++) {
        for (size_t v = 0; v < n; v++) {
            // 24 + 48 bytes in 6 rounds
            auto eqz = circ.addParamGate(common::utils::GateType::kEqualsZero, Bsum[u][v], 0);
            eqz = circ.addParamGate(common::utils::GateType::kEqualsZero, eqz, 1);
            eqz = circ.addParamGate(common::utils::GateType::kEqualsZero, eqz, 2);
            eqz = circ.addParamGate(common::utils::GateType::kEqualsZero, eqz, 3);
            eqz = circ.addParamGate(common::utils::GateType::kEqualsZero, eqz, 4);
            Bsum[u][v] = circ.addGate(common::utils::GateType::kConvertB2A, eqz);
        }
    }
    for (size_t u = 0; u < n; u++) {
        Bsum[u] = circ.addMGate(common::utils::GateType::kFlip, Bsum[u]);
    }

    std::vector<common::utils::wire_t> res(n);
    for (size_t u = 0; u < n; u++) {
        auto accu = Bsum[u][0];
        for (size_t v = 1; v < n; v++) {
            accu = circ.addGate(common::utils::GateType::kAdd, accu, Bsum[u][v]);
        }
        res[u] = accu;
    }

    return res;
}
