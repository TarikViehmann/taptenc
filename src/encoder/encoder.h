#pragma once

#include "timed_automata.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace taptenc {
class Encoder {
protected:
  Automaton mergeAutomata(const ::std::vector<Automaton> &automata,
                          ::std::vector<Transition> &interconnections,
                          ::std::string prefix);
  ::std::vector<Transition> createCopyTransitionsBetweenTAs(
      const Automaton &source, const Automaton &dest,
      const ::std::vector<State> &filter, ::std::string guard,
      ::std::string update, ::std::string sync, bool passive = true);
  ::std::vector<Transition> createSuccessorTransitionsBetweenTAs(
      const Automaton &base, const Automaton &source, const Automaton &dest,
      const ::std::vector<State> &filter, ::std::string guard,
      ::std::string update);
  void addTrapTransitions(Automaton &ta, const ::std::vector<State> &sources,
                          ::std::string guard, ::std::string update,
                          ::std::string sync, bool passive = true);
  void addBaseSyncs(AutomataSystem &s, const int base_pos);
  void addInvariants(Automaton &ta, const ::std::vector<State> filter,
                     ::std::string inv);
  ::std::string toPrefix(::std::string op, ::std::string sub = "",
                         ::std::string pa = "");
  ::std::string addToPrefix(::std::string prefix, ::std::string op,
                            ::std::string sub = "");

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
