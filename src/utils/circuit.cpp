#include "circuit.h"

#include <stdexcept>

namespace common::utils {

Gate::Gate(GateType type, wire_t out, size_t gid) : type(type), out(out), gid(gid) {}

Gate::Gate(GateType type, wire_t out, std::vector<wire_t> outs, size_t gid) : type(type), out(out) ,outs(outs), gid(gid) {}

FIn2Gate::FIn2Gate(GateType type, wire_t in1, wire_t in2, wire_t out, size_t gid)
    : Gate(type, out, gid), in1{in1}, in2{in2} {}

FIn3Gate::FIn3Gate(GateType type, wire_t in1, wire_t in2, wire_t in3, wire_t out, size_t gid)
    : Gate(type, out, gid), in1{in1}, in2{in2}, in3{in3} {}

FIn4Gate::FIn4Gate(GateType type, wire_t in1, wire_t in2, wire_t in3, wire_t in4, wire_t out, size_t gid)
    : Gate(type, out, gid), in1{in1}, in2{in2}, in3{in3}, in4{in4} {}

FIn1Gate::FIn1Gate(GateType type, wire_t in, wire_t out, size_t gid)
    : Gate(type, out, gid), in{in} {}

ParamFIn1Gate::ParamFIn1Gate(GateType type, wire_t in, wire_t out, size_t param, size_t gid)
    : Gate(type, out, gid), in{in}, param{param} {}

SIMDGate::SIMDGate(GateType type, std::vector<wire_t> in1,
                   std::vector<wire_t> in2, wire_t out, size_t gid)
    : Gate(type, out, gid), in1(std::move(in1)), in2(std::move(in2)) {}

SIMDSingleOutGate::SIMDSingleOutGate(GateType type, std::vector<wire_t> in1,
                     wire_t out, size_t gid)
    : Gate(type, out, gid), in1(std::move(in1)) {}

SIMDOGate::SIMDOGate(GateType type, std::vector<wire_t> in1,
                     std::vector<wire_t> outs, size_t gid)
    : Gate(type, outs[0], outs, gid), in1(std::move(in1)) {}

SIMDODoubleInGate::SIMDODoubleInGate(GateType type, std::vector<wire_t> in1, std::vector<wire_t> in2,
                     std::vector<wire_t> outs, size_t gid)
    : Gate(type, outs[0], outs, gid), in1(std::move(in1)), in2(std::move(in2)) {}

ParamSIMDOGate::ParamSIMDOGate(GateType type, std::vector<wire_t> in1,
                     std::vector<wire_t> outs, size_t param, size_t gid)
    : Gate(type, outs[0], outs, gid), in1(std::move(in1)), param(param) {}

TwoParamSIMDOGate::TwoParamSIMDOGate(GateType type, std::vector<wire_t> in1,
                     std::vector<wire_t> outs, size_t param1, size_t param2, size_t gid)
    : Gate(type, outs[0], outs, gid), in1(std::move(in1)), param1(param1), param2(param2) {}

ThreeParamSIMDOGate::ThreeParamSIMDOGate(GateType type, std::vector<wire_t> in1,
                     std::vector<wire_t> outs, size_t param1, size_t param2, size_t param3, size_t gid)
    : Gate(type, outs[0], outs, gid), in1(std::move(in1)), param1(param1), param2(param2), param3(param3) {}

ParamWithFlagSIMDOGate::ParamWithFlagSIMDOGate(GateType type, std::vector<wire_t> in1,
                     std::vector<wire_t> outs, size_t param, bool flag, size_t gid)
    : Gate(type, outs[0], outs, gid), in1(std::move(in1)), param(param), flag(flag) {}

std::ostream& operator<<(std::ostream& os, GateType type) {
  switch (type) {
    case kInp:
      os << "Input";
      break;

    case kBinInp:
      os << "Binary input";
      break;

    case kAdd:
      os << "Addition";
      break;

    case kCompose:
      os << "Compose (arithmetic) bits to number";
      break;

    case kMul:
      os << "Multiplication";
      break;

    case kMul3:
      os << "Multiplication with Fan-in 3";
      break;

    case kMul4:
      os << "Multiplication with Fan-in 4";
      break;

    case kSub:
      os << "Subtraction";
      break;

    case kConstAdd:
      os << "Addition with constant";
      break;

    case kConstMul:
      os << "Multiplication with constant";
      break;

    case kRelu:
      os << "ReLU";
      break;

    case kMsb:
      os << "MSB";
      break;

    case kEqz:
      os << "Equals to zero";
      break;

    case kLtz:
      os << "Less than zero";
      break;

    case kDotprod:
      os << "Dotproduct";
      break;

    case kTrdotp:
      os << "Dotproduct with truncation";
      break;

    case kShuffle:
      os << "Shuffle a vector";
      break;

    case kDoubleShuffle:
      os << "Shuffle a vector by two consecutive permutations";
      break;

    case kGenCompaction:
      os << "Compute compaction permutation of a vector containing bits";
      break;

    case kXor:
      os << "xor";
      break;

    case kAnd:
      os << "and";
      break;

    case kReveal:
      os << "Reveal";
      break;

    case kFlip:
      os << "Flip";
      break;

    case kAddConstToVec:
      os << "AddConstToVec";
      break;

    case kAddVec:
      os << "AddVec";
      break;

    case kPreparePropagate:
      os << "PreparePropagate";
      break;

    case kPrepareGather:
      os << "PrepareGather";
      break;

    case kPropagate:
      os << "Propagate";
      break;

    case kGather:
      os << "Gather";
      break;

    case kReorder:
      os << "Reorder";
      break;

    case kReorderInverse:
      os << "ReorderInverse";
      break;

    default:
      os << "Invalid";
      break;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const LevelOrderedCircuit& circ) {
  for (size_t i = 0; i < GateType::NumGates; ++i) {
    os << GateType(i) << ": " << circ.count[i] << "\n";
  }
  os << "Total: " << circ.num_gates << "\n";
  os << "Depth: " << circ.gates_by_level.size() << "\n";
  return os;
}
};  // namespace common::utils
