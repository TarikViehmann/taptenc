#include "encoders.h"
#include "filter.h"
#include "timed_automata.h"
#include "utils.h"
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

using namespace taptenc;
using namespace encoderutils;
void ModularEncoder::encodeNoOp(AutomataSystem &s, std::vector<State> &targets,
                                const std::string opsync, int base_index) {
  Filter target_filter = Filter(targets);
  Filter base_filter = Filter(s.instances[base_index].first.states);
  Automaton base_copy = base_filter.filterAutomaton(
      s.instances[base_index].first, toPrefix(opsync, "base"));
  base_copy.states.push_back(State("trap", ""));
  Filter bc_filter = target_filter.updateFilter(base_copy);
  Filter rev_bc_filter =
      target_filter.reverseFilter(s.instances[base_index].first)
          .updateFilter(base_copy);
  addTrapTransitions(base_copy, rev_bc_filter.getFilter(), "", "", opsync);
  std::vector<Transition> self_loops = createCopyTransitionsBetweenTAs(
      base_copy, base_copy, bc_filter.getFilter(), "", "", opsync);
  base_copy.transitions.insert(base_copy.transitions.end(), self_loops.begin(),
                               self_loops.end());
  s.instances.push_back(std::make_pair(base_copy, ""));
}
void ModularEncoder::encodeFuture(AutomataSystem &s,
                                  std::vector<State> &targets,
                                  const std::string opsync, const Bounds bounds,
                                  int base_index) {
  Filter target_filter = Filter(targets);
  Filter base_filter = Filter(s.instances[base_index].first.states);
  Automaton base_copy = base_filter.filterAutomaton(
      s.instances[base_index].first, toPrefix(opsync, "base"));
  Automaton sat = base_filter.filterAutomaton(s.instances[base_index].first,
                                              toPrefix(opsync, "sat"));
  Filter sat_filter = target_filter.updateFilter(sat);
  Filter rev_bc_filter =
      target_filter.updateFilter(base_copy).reverseFilter(base_copy);
  std::string clock = "clX" + opsync;
  bool upper_bounded = (bounds.upper_bound != std::numeric_limits<int>::max());
  bool lower_bounded = (bounds.lower_bound != 0 || bounds.l_op != "&lt;=");
  std::string guard_upper_bound_crossed =
      clock + inverse_op(bounds.r_op) + std::to_string(bounds.upper_bound);
  std::string guard_constraint_sat =
      (lower_bounded ? clock + reverse_op(bounds.l_op) +
                           std::to_string(bounds.lower_bound)
                     : "") +
      ((lower_bounded && upper_bounded) ? "&amp;&amp;" : "") +
      (upper_bounded ? clock + bounds.r_op + std::to_string(bounds.upper_bound)
                     : "");
  if (upper_bounded) {
    addInvariants(sat, sat.states,
                  clock + bounds.r_op + std::to_string(bounds.upper_bound));
  }
  std::vector<Transition> base_to_sat = createCopyTransitionsBetweenTAs(
      base_copy, sat, base_copy.states, "", clock + " = 0", opsync);
  std::vector<Transition> sat_to_base = createCopyTransitionsBetweenTAs(
      sat, base_copy, sat_filter.getFilter(), guard_constraint_sat, "", "");
  std::vector<Transition> interconnections;
  interconnections.insert(interconnections.end(), base_to_sat.begin(),
                          base_to_sat.end());
  interconnections.insert(interconnections.end(), sat_to_base.begin(),
                          sat_to_base.end());

  std::vector<Automaton> parts{base_copy, sat};
  Automaton res = mergeAutomata(parts, interconnections, "compX" + opsync);
  res.states.push_back(State("trap", ""));
  res.clocks.push_back(clock);
  addTrapTransitions(res, sat.states, "", "", opsync);
  addTrapTransitions(res, sat.states, guard_upper_bound_crossed, "", "");
  s.instances.push_back(std::make_pair(res, ""));
}

void ModularEncoder::encodePast(AutomataSystem &s, std::vector<State> &targets,
                                const std::string opsync, const Bounds bounds,
                                int base_index) {
  Filter target_filter = Filter(targets);
  Filter base_filter = Filter(s.instances[base_index].first.states);
  Automaton base_copy = base_filter.filterAutomaton(
      s.instances[base_index].first, toPrefix(opsync, "base"));
  Automaton sat = target_filter.filterAutomaton(s.instances[base_index].first,
                                                toPrefix(opsync, "sat"));
  Automaton remainder = base_filter.filterAutomaton(
      s.instances[base_index].first, toPrefix(opsync, "rem"));
  Filter bc_filter = target_filter.updateFilter(base_copy);
  std::string clock = "clX" + opsync;
  bool upper_bounded = (bounds.upper_bound != std::numeric_limits<int>::max());
  bool lower_bounded = (bounds.lower_bound != 0 || bounds.l_op != "&lt;=");
  std::string guard_lower_bound_not_met = clock +
                                          inverse_op(reverse_op(bounds.l_op)) +
                                          std::to_string(bounds.lower_bound);
  std::string guard_upper_bound_crossed =
      clock + inverse_op(bounds.r_op) + std::to_string(bounds.upper_bound);
  std::string guard_constraint_sat =
      (lower_bounded ? clock + reverse_op(bounds.l_op) +
                           std::to_string(bounds.lower_bound)
                     : "") +
      ((lower_bounded && upper_bounded) ? "&amp;&amp;" : "") +
      (upper_bounded ? clock + bounds.r_op + std::to_string(bounds.upper_bound)
                     : "");

  if (upper_bounded) {
    addInvariants(sat, sat.states, clock + " &lt;= 0");
    addInvariants(remainder, remainder.states,
                  clock + bounds.r_op + std::to_string(bounds.upper_bound));
  }
  std::vector<Transition> base_to_sat = createCopyTransitionsBetweenTAs(
      base_copy, sat, bc_filter.getFilter(), "", clock + " = 0", "");
  std::vector<Transition> sat_to_rem = createSuccessorTransitionsBetweenTAs(
      s.instances[base_index].first, sat, remainder, sat.states, "", "");
  std::vector<Transition> rem_to_base = createCopyTransitionsBetweenTAs(
      remainder, base_copy, remainder.states, guard_constraint_sat, "", opsync);

  std::vector<Transition> interconnections;
  interconnections.insert(interconnections.end(), base_to_sat.begin(),
                          base_to_sat.end());
  interconnections.insert(interconnections.end(), sat_to_rem.begin(),
                          sat_to_rem.end());
  interconnections.insert(interconnections.end(), rem_to_base.begin(),
                          rem_to_base.end());

  std::vector<Automaton> parts{base_copy, sat, remainder};
  Automaton res = mergeAutomata(parts, interconnections, "compX" + opsync);
  res.states.push_back(State("trap", ""));
  res.clocks.push_back(clock);
  addTrapTransitions(res, sat.states, "", "", opsync);
  addTrapTransitions(res, remainder.states, guard_upper_bound_crossed, "", "");
  if (lower_bounded) {
    addTrapTransitions(res, remainder.states, guard_lower_bound_not_met, "",
                       opsync);
  }
  s.instances.push_back(std::make_pair(res, ""));
}

ModularEncoder::ModularEncoder(AutomataSystem &s, const int base_pos) {
  addBaseSyncs(s, base_pos);
}
