#pragma once

#include "timed_automata.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace taptenc {
class Encoder {
protected:
  // store the original id of a copied state
  ::std::unordered_map<::std::string, ::std::string> base_ids;

  ::std::string getBaseId(::std::string id);
  void addToBaseIds(::std::string state_id, ::std::string base_id);
  void setBaseIds(const Automaton &s);
  void filterTransitionsInPlace(::std::vector<Transition> &trans,
                                const ::std::vector<State> &states_filter,
                                bool filter_source);
  void filterAutomatonInPlace(Automaton &source,
                              const ::std::vector<State> &states_filter);
  Automaton filterAutomaton(const Automaton &source,
                            const ::std::vector<State> &states_filter,
                            ::std::string filter_prefix,
                            bool strip_constraints = true);
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
  ::std::vector<State> updateFilter(const Automaton &ta,
                                    const ::std::vector<State> &filter);
  ::std::vector<State> reverseFilter(const Automaton &ta,
                                     const ::std::vector<State> &filter);
  void addTrapTransitions(Automaton &ta, ::std::vector<State> &sources,
                          ::std::string guard, ::std::string update,
                          ::std::string sync, bool passive = true);
  void addBaseSyncs(AutomataSystem &s, const int base_pos);
  void addInvariants(Automaton &ta, const ::std::vector<State> filter,
                     ::std::string inv);

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
