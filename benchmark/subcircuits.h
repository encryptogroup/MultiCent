#pragma once

namespace subcirc {
    /**
     * Applies the given permutation to the given data where both are secret shared.
     * 
     * Communication:
     * 20n bytes setup
     * 12n bytes online
     * 2 rounds
     */
    std::vector<common::utils::wire_t> applyPerm(std::vector<common::utils::wire_t> &perm, std::vector<common::utils::wire_t> &data,
                common::utils::Circuit<Ring> &circ, size_t &next_free_shuffle_id);

    /**
     * Composes the given permutations by computing outer * inner, i.e., result(i) = outer(inner(i)).
     * 
     * Communication:
     * 20n bytes setup
     * 12n bytes online
     * 3 rounds
     */
    std::vector<common::utils::wire_t> compose(std::vector<common::utils::wire_t> &outer, std::vector<common::utils::wire_t> &inner,
                common::utils::Circuit<Ring> &circ, size_t &next_free_shuffle_id);

    /**
     * Generates the permutation required to stable sort the given keys using radix sort.
     * keys is structured as follows:
     * the first inner vector contains the LSBs of all keys, the next one contains the next bit of all, ...
     * and the last vector contains the MSBs of all keys.
     * 
     * Communication:
     * 32n*nmrb_bits - 28n bytes setup
     * 24n*nmbr_bits - 16n bytes online
     * 4*nmbr_bits - 3 rounds
     */
    std::vector<common::utils::wire_t> genSortingPerm(std::vector<std::vector<common::utils::wire_t>> &keys,
                                            common::utils::Circuit<Ring> &circ, size_t &next_free_shuffle_id, size_t nmbr_bits);

    /**
     * Runs the entire graph analysis for pi_3 and returns wires for the values per sorted node.
     * 
     * Note that all values are arithmetically shared, also single bits.
     * 
     * @param source_bits LSBs of all entries in the source list, then next bits of all entries, ..., up to MSBs
     * @param destination_bits LSBs of all entries in the destination list, then next bits of all entries, ..., up to MSBs
     * @param vertex_flags 1 for each entry corresponding to a node (i.e., source=destination), 0 otherwise
     * @param payload The associated payload of nodes/edges, should be initialized to 0
     * @param circ The circuit to emplace the pi_3 circuit in
     * @param next_free_shuffle_id Next free id for a unique shuffle, call-by-reference, increments if ids are used
     * @param n Number of nodes, hence n <= length of entire list that also contains edges
     * @param nmbr_bits Number of bits per source/destination entry, max 32 but set to lower if applicable to improve performance
     *                      in radix_sort. 2^nmbr_bits > n + 1 required.
     * @param weights Weights for paths of length i+1, weights.size() also determines the search depth D
     * 
     * Communication:
     * 64t*nmbr_bits + 108t + 32*t*D bytes setup
     * 48t*nmbr_bits + 68t + 16*t*D bytes online
     * 4*nmbr_bits + 7 + 3 * D rounds
     */
    std::vector<common::utils::wire_t> pi_3(std::vector<std::vector<common::utils::wire_t>> &source_bits,
                                            std::vector<std::vector<common::utils::wire_t>> &destination_bits,
                                            std::vector<common::utils::wire_t>              &vertex_flags,
                                            std::vector<common::utils::wire_t>              &payload,
                                            common::utils::Circuit<Ring> &circ, 
                                            size_t &next_free_shuffle_id, 
                                            size_t n,
                                            size_t nmbr_bits,
                                            std::vector<Ring> &weights);

    /**
     * Runs the entire reference (based on matrix multiplication) graph analysis for pi_3 and returns wires for the values per node.
     * 
     * Note that all values are arithmetically shared, also single bits.
     * 
     * @param adj_matrices List of all input adjacency matrices, expected to all be nxn
     * @param circ The circuit to emplace the pi_3 circuit in
     * @param n Number of nodes, has to match the dimension of the adj_matrices
     * @param weights Weights for paths of length i+1, weights.size() also determines the search depth D
     * 
     * Communication:
     * 4*n^2*(D-1) bytes setup
     * 8*n^2*(D-1) bytes online
     * D - 1 rounds
     */
    std::vector<common::utils::wire_t> pi_3_reference(  std::vector<std::vector<std::vector<common::utils::wire_t>>> &adj_matrices,
                                                        common::utils::Circuit<Ring> &circ,
                                                        size_t n,
                                                        std::vector<Ring> &weights);

    /**
     * Runs the entire graph analysis for pi_2 and returns wires for the values per sorted node.
     * 
     * Note that all values are arithmetically shared, also single bits.
     * 
     * @param source_bits LSBs of all entries in the source list, then next bits of all entries, ..., up to MSBs
     * @param destination_bits LSBs of all entries in the destination list, then next bits of all entries, ..., up to MSBs
     * @param vertex_flags 1 for each entry corresponding to a node (i.e., source=destination), 0 otherwise
     * @param payload The associated payload of nodes/edges, should be initialized to 0
     * @param circ The circuit to emplace the pi_3 circuit in
     * @param next_free_shuffle_id Next free id for a unique shuffle, call-by-reference, increments if ids are used
     * @param n Number of nodes, hence n <= length of entire list that also contains edges
     * @param nmbr_bits Number of bits per source/destination entry, max 32 but set to lower if applicable to improve performance
     *                      in radix_sort. 2^nmbr_bits > n + 1 required.
     * @param weights Weights for paths of length i+1, weights.size() also determines the search depth D
     * 
     * Communication:
     * 96t*nmbr_bits + 256t - 48 + 32*t*D bytes setup
     * 72t*nmbr_bits + 232t - 96 + 16*t*D bytes online 
     * 8*nmbr_bits + 20 + 3 * D rounds
     */
    std::vector<common::utils::wire_t> pi_2(std::vector<std::vector<common::utils::wire_t>> &source_bits,
                                            std::vector<std::vector<common::utils::wire_t>> &destination_bits,
                                            std::vector<common::utils::wire_t>              &vertex_flags,
                                            std::vector<common::utils::wire_t>              &payload,
                                            common::utils::Circuit<Ring> &circ, 
                                            size_t &next_free_shuffle_id, 
                                            size_t n,
                                            size_t nmbr_bits,
                                            std::vector<Ring> &weights);

    /**
     * Runs the entire reference (based on matrix multiplication) graph analysis for pi_2 and returns wires for the values per node.
     * 
     * Note that all values are arithmetically shared, also single bits.
     * 
     * @param adj_matrices List of all input adjacency matrices, expected to all be nxn
     * @param circ The circuit to emplace the pi_3 circuit in
     * @param n Number of nodes, has to match the dimension of the adj_matrices
     * @param weights Weights for paths of length i+1, weights.size() also determines the search depth D
     * 
     * Communication:
     * 4*n^2*(D+l-2) bytes setup
     * 8*n^2*(D+l-2) bytes online
     * D + ceil(log2(l)) - 1 rounds
     */
    std::vector<common::utils::wire_t> pi_2_reference(  std::vector<std::vector<std::vector<common::utils::wire_t>>> &adj_matrices,
                                                        common::utils::Circuit<Ring> &circ,
                                                        size_t n,
                                                        std::vector<Ring> &weights);

    /**
     * Runs the entire graph analysis for pi_1 and returns wires for the values per sorted node.
     * 
     * Note that all values are arithmetically shared, also single bits.
     * 
     * @param source_bits LSBs of all entries in the source list, then next bits of all entries, ..., up to MSBs
     * @param destination_bits LSBs of all entries in the destination list, then next bits of all entries, ..., up to MSBs
     * @param vertex_flags 1 for each entry corresponding to a node (i.e., source=destination), 0 otherwise
     * @param payload The associated payload of nodes/edges, should be initialized to except for that node n should set payload n-1
     *                  to 1 for the index corresponding to itself.
     *                  This is now on a "per node basis", so this should be a vector of dimension n containing all the payloads
     * @param circ The circuit to emplace the pi_3 circuit in
     * @param next_free_shuffle_id Next free id for a unique shuffle, call-by-reference, increments if ids are used
     * @param n Number of nodes, hence n <= length of entire list that also contains edges
     * @param nmbr_bits Number of bits per source/destination entry, max 32 but set to lower if applicable to improve performance
     *                      in radix_sort. 2^nmbr_bits > n + 1 required.
     * @param depth Search depth D (note that while the original definition of pi_3 consider weights, these are useless if positive)
     * 
     * Communication:
     * 64t*nmbr_bits + 24n^2 + 32tnD + 8tn + 100t bytes setup
     * 48t*nmbr_bits + 48n^2 + 16tnD + 4tn +  64t bytes online 
     * 4*nmbr_bits + 3D + 13 rounds
     */
    std::vector<common::utils::wire_t> pi_1(std::vector<std::vector<common::utils::wire_t>> &source_bits,
                                            std::vector<std::vector<common::utils::wire_t>> &destination_bits,
                                            std::vector<common::utils::wire_t>              &vertex_flags,
                                            std::vector<std::vector<common::utils::wire_t>> &payload,
                                            common::utils::Circuit<Ring> &circ, 
                                            size_t &next_free_shuffle_id, 
                                            size_t n,
                                            size_t nmbr_bits,
                                            size_t depth);

    /**
     * Runs the entire reference (based on matrix multiplication) graph analysis for pi_1 and returns wires for the values per node.
     * 
     * Note that all values are arithmetically shared, also single bits.
     * 
     * @param adj_matrices List of all input adjacency matrices, expected to all be nxn
     * @param circ The circuit to emplace the pi_3 circuit in
     * @param n Number of nodes, has to match the dimension of the adj_matrices
     * @param depth Search depth D
     * 
     * Communication:
     * 4n^3*(D-1)+24n^2 bytes setup
     * 8n^3*(D-1)+48n^2 bytes online
     * D + 5 rounds
     */
    std::vector<common::utils::wire_t> pi_1_reference(  std::vector<std::vector<std::vector<common::utils::wire_t>>> &adj_matrices,
                                                        common::utils::Circuit<Ring> &circ,
                                                        size_t n,
                                                        size_t depth);
}
