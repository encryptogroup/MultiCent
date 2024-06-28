#include "sharing.h"

namespace graphsc {
//check the correctness of the following functions: 
template <>
void AddShare<BoolRing>::randomize(emp::PRG& prg) {
 bool data[3];
 prg.random_bool(static_cast<bool*>(data), 1);
 value_ = data[1];
}

};  // namespace grpahsc
