#pragma once
#include "encoder.h"
#include "timed_automata.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace taptenc {
class DirectEncoder : Encoder {
private:
  size_t encode_counter = 0;
  ::std::unordered_map<
      ::std::string,
      ::std::unordered_map<::std::string,
                           ::std::pair<Automaton, ::std::vector<Transition>>>>
      pa_tls;
  ::std::vector<::std::string> pa_order;
  void generateBaseTimeLine(AutomataSystem &s, const int base_pos,
                            const int plan_pos);
  ::std::vector<Transition>
  createTransitionsBackToOrigTL(::std::vector<Transition> &trans,
                                ::std::string prefix, ::std::string pa);
  ::std::vector<Transition>
  addToPrefixOnTransitions(const ::std::vector<Transition> &trans,
                           ::std::string prefix, bool only_inner_trans = false);
  void removeTransitionsToNextTl(::std::vector<Transition> &trans,
                                 ::std::string curr_pa);

public:
  ::std::unordered_map<
      ::std::string,
      ::std::unordered_map<::std::string,
                           ::std::pair<Automaton, ::std::vector<Transition>>>>
  getInternalTAs();
  void encodeNoOp(AutomataSystem &s, ::std::vector<State> &targets,
                  const ::std::string pa, int base_index = 0);
  void encodeFuture(AutomataSystem &s, ::std::vector<State> &targets,
                    const ::std::string pa, const ::std::string constraint_name,
                    const Bounds bounds, int context, int base_index = 0);
  void encodePast(AutomataSystem &s, ::std::vector<State> &targets,
                  const ::std::string pa, const Bounds bounds,
                  int base_index = 0);
  DirectEncoder(AutomataSystem &s, const int base_pos = 0,
                const int plan_pos = 1);
  void createFinalSystem(AutomataSystem &s);
};
} // end namespace taptenc
