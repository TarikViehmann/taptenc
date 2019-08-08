#pragma once

#include "timed_automata.h"
#include <string>
#include <vector>

namespace taptenc {
class Encoder {
public:
  virtual void encodeNoOp(AutomataSystem &s, ::std::vector<State> &targets,
                          const ::std::string opsync, int base_index = 0) = 0;
  virtual void encodeFuture(AutomataSystem &s, ::std::vector<State> &targets,
                            const ::std::string opsync, const Bounds bounds,
                            int base_index = 0) = 0;
  virtual void encodePast(AutomataSystem &s, ::std::vector<State> &targets,
                          const ::std::string opsync, const Bounds bounds,
                          int base_index = 0) = 0;
  // virtual void encodeUntil(AutomataSystem &s, ::std::vector<State> targets,
  // int base_index = 0) = 0; virtual void encodeSince(AutomataSystem &s,
  // ::std::vector<State> targets, int base_index = 0) = 0;
};
} // end namespace taptenc
#include "compact_encoder.h"
