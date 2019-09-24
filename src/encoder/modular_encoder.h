#pragma once
#include "timed_automata.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace taptenc {
class ModularEncoder {

public:
  void encodeNoOp(AutomataSystem &s, ::std::vector<State> &targets,
                  const ::std::string opsync, int base_index = 0);
  void encodeFuture(AutomataSystem &s, ::std::vector<State> &targets,
                    const ::std::string opsync, const Bounds bounds,
                    int base_index = 0);
  void encodePast(AutomataSystem &s, ::std::vector<State> &targets,
                  const ::std::string opsync, const Bounds bounds,
                  int base_index = 0);
  ModularEncoder(AutomataSystem &s, const int base_pos = 0);
};
} // end namespace taptenc
