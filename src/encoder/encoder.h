#pragma once

#include "timed_automata.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace taptenc {
enum ICType { Future, NoOp, Past, Invariant };
struct encICInfo {
  ::std::vector<State> targets;
  ::std::string name;
  Bounds bounds;
  ICType type;
  encICInfo(::std::vector<State> arg_targets, ::std::string arg_name,
            const Bounds &arg_bounds, ICType arg_type)
      : targets(arg_targets), name(arg_name), bounds(arg_bounds),
        type(arg_type) {}
};
typedef struct encICInfo EncICInfo;

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
  Automaton generatePlanAutomaton(const ::std::vector<PlanAction> &plan,
                                  ::std::string name);
};
} // end namespace taptenc
