#pragma once
#include "encoder_utils.h"
#include "timed_automata.h"
#include <string>
#include <vector>

namespace taptenc {
class CompactEncoder {
public:
  void encodeNoOp(AutomataSystem &s, ::std::vector<State> &targets,
                  const ::std::string opsync, int base_index = 0);
  void encodeFuture(AutomataSystem &s, ::std::vector<State> &targets,
                    const ::std::string opsync, const Bounds bounds,
                    int base_index = 0);
  void encodePast(AutomataSystem &s, ::std::vector<State> &targets,
                  const ::std::string opsync, const Bounds bounds,
                  int base_index = 0);
};
} // end namespace taptenc
