#include "encoders.h"
#include "timed_automata.h"
#include "utils.h"
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#define BASE_SYNC_CONNECTIVE "XtoX"

using namespace taptenc;
std::string ModularEncoder::getBaseId(std::string id) {
  auto search = base_ids.find(id);
  if (search != base_ids.end()) {
    if (base_ids[id] == id) {
      return id;
    } else {
      return getBaseId(base_ids[id]);
    }
  } else {
    std::cout << "ModularEncoder getBaseId: id has no base id entry: " << id
              << std::endl;
    return id;
  }
}
void ModularEncoder::addToBaseIds(std::string state_id, std::string base_id) {
  auto emplace = base_ids.emplace(state_id, base_id);
  if (emplace.second == false) {
    std::cout << "ModularEncoder addToBaseIds: entry for " << state_id
              << " already exists: " << base_ids[state_id] << std::endl;
  }
}
void ModularEncoder::setBaseIds(const Automaton &s) {
  for (const auto &state : s.states) {
    addToBaseIds(state.id, state.id);
  }
}

Automaton ModularEncoder::filterAutomaton(
    const Automaton &source, const std::vector<State> &states_filter,
    std::string filter_prefix, bool strip_constraints) {
  std::vector<State> res_states;
  std::vector<Transition> res_transitions;
  for (const auto &f_state : states_filter) {
    auto search = std::find_if(
        source.states.begin(), source.states.end(),
        [f_state](const State &s) bool { return s.id == f_state.id; });
    if (search != source.states.end()) {
      res_states.push_back(State(filter_prefix + search->name,
                                 strip_constraints ? "" : search->inv));
      addToBaseIds(res_states.back().id, search->id);
    } else {
      std::cout << "ModularEncoder filterAutomaton: filter state not found (id "
                << f_state.id << ")" << std::endl;
    }
  }
  for (const auto &trans : source.transitions) {
    auto source = std::find_if(res_states.begin(), res_states.end(),
                               [trans, this](const State &s) bool {
                                 return base_ids[s.id] == trans.source_id;
                               });
    auto dest = std::find_if(res_states.begin(), res_states.end(),
                             [trans, this](const State &s) bool {
                               return base_ids[s.id] == trans.dest_id;
                             });
    if (source != res_states.end() && dest != res_states.end()) {
      if (strip_constraints) {
        res_transitions.push_back(
            Transition(source->id, dest->id, "", "", trans.sync, true));
      } else {
        res_transitions.push_back(Transition(source->id, dest->id, trans.guard,
                                             trans.update, trans.sync, true));
      }
    }
  }
  Automaton res(res_states, res_transitions, filter_prefix, false);
  if (!strip_constraints) {
    res.clocks.insert(res.clocks.end(), source.clocks.begin(),
                      source.clocks.end());
    res.bool_vars.insert(res.bool_vars.end(), source.bool_vars.begin(),
                         source.bool_vars.end());
  }
  return res;
}

Automaton
ModularEncoder::mergeAutomata(const std::vector<Automaton> &automata,
                              std::vector<Transition> &interconnections,
                              std::string prefix) {
  std::vector<State> res_states;
  std::vector<Transition> res_transitions{interconnections};
  std::vector<std::string> res_clocks;
  std::vector<std::string> res_bool_vars;
  for (const auto &ta : automata) {
    res_states.insert(res_states.end(), ta.states.begin(), ta.states.end());
    res_transitions.insert(res_transitions.end(), ta.transitions.begin(),
                           ta.transitions.end());
    res_clocks.insert(res_clocks.end(), ta.clocks.begin(), ta.clocks.end());
    res_bool_vars.insert(res_bool_vars.end(), ta.bool_vars.begin(),
                         ta.bool_vars.end());
  }
  Automaton res(res_states, res_transitions, prefix, false);
  res.clocks = res_clocks;
  res.bool_vars = res_bool_vars;
  return res;
}

std::vector<Transition> ModularEncoder::createCopyTransitionsBetweenTAs(
    const Automaton &source, const Automaton &dest,
    const std::vector<State> &filter, std::string guard, std::string update,
    std::string sync, bool passive) {
  std::vector<Transition> res_transitions;
  for (const auto &f_state : filter) {
    auto c_source =
        std::find_if(source.states.begin(), source.states.end(),
                     [f_state, this](const State &s) bool {
                       return getBaseId(s.id) == getBaseId(f_state.id);
                     });
    auto c_dest =
        std::find_if(dest.states.begin(), dest.states.end(),
                     [f_state, this](const State &s) bool {
                       return getBaseId(s.id) == getBaseId(f_state.id);
                     });
    if (c_source != source.states.end() && c_dest != dest.states.end()) {
      res_transitions.push_back(
          Transition(c_source->id, c_dest->id, guard, update, sync, passive));
    }
  }
  return res_transitions;
}

std::vector<Transition> ModularEncoder::createSuccessorTransitionsBetweenTAs(
    const Automaton &base, const Automaton &source, const Automaton &dest,
    const std::vector<State> &filter, std::string guard, std::string update) {
  std::vector<Transition> res_transitions;
  for (const auto &trans : base.transitions) {
    auto search = std::find_if(
        filter.begin(), filter.end(), [trans, this](const State &s) bool {
          return getBaseId(s.id) == getBaseId(trans.source_id);
        });
    if (search != filter.end()) {
      auto source_state = std::find_if(
          source.states.begin(), source.states.end(),
          [search](const State &s) bool { return s.id == search->id; });
      auto dest_state =
          std::find_if(dest.states.begin(), dest.states.end(),
                       [trans, this](const State &s) bool {
                         return getBaseId(s.id) == getBaseId(trans.dest_id);
                       });
      if (source_state != source.states.end() &&
          dest_state != dest.states.end()) {
        res_transitions.push_back(Transition(source_state->id, dest_state->id,
                                             guard, update, trans.sync, true));
      }
    }
  }
  return res_transitions;
}
std::vector<State>
ModularEncoder::updateFilter(const Automaton &ta,
                             const std::vector<State> &filter) {
  std::vector<State> update_filter;
  for (const auto &f_state : filter) {
    auto search =
        std::find_if(ta.states.begin(), ta.states.end(),
                     [f_state, this](const State &s) bool {
                       return getBaseId(f_state.id) == getBaseId(s.id);
                     });
    if (search != ta.states.end()) {
      update_filter.push_back(*search);
    } else {
      std::cout
          << "ModularEncoder updateFilter: base id of filter not found (id "
          << f_state.id << ")" << std::endl;
    }
  }
  return update_filter;
}

std::vector<State>
ModularEncoder::reverseFilter(const Automaton &ta,
                              const std::vector<State> &filter) {
  std::vector<State> reverse_filter;
  for (const auto &ta_state : ta.states) {
    auto search = std::find_if(
        filter.begin(), filter.end(),
        [ta_state](const State &s) bool { return ta_state.id == s.id; });
    if (search == ta.states.end()) {
      reverse_filter.push_back(ta_state);
    }
  }
  return reverse_filter;
}

void ModularEncoder::addTrapTransitions(Automaton &ta,
                                        std::vector<State> &sources,
                                        std::string guard, std::string update,
                                        std::string sync, bool passive) {
  auto trap =
      std::find_if(ta.states.begin(), ta.states.end(),
                   [](const State &s) bool { return s.name == "trap"; });
  if (trap == ta.states.end()) {
    std::cout << "ModularEncoder addTrapTransitions: trap not found. Abort."
              << std::endl;
    return;
  }
  for (const auto &source : sources) {
    auto search = std::find_if(
        ta.states.begin(), ta.states.end(),
        [source](const State &s) bool { return source.id == s.id; });
    if (search != ta.states.end()) {
      ta.transitions.push_back(
          Transition(search->id, trap->id, guard, update, sync, passive));
    } else {
      std::cout << "ModularEncoder addTrapTransitions: id of source not "
                   "found in TA (id "
                << source.id << ")" << std::endl;
    }
  }
}
void ModularEncoder::addBaseSyncs(AutomataSystem &s, const int base_pos) {
  for (auto &trans : s.instances[base_pos].first.transitions) {
    if (trans.sync == "") {
      trans.sync = trans.source_id + BASE_SYNC_CONNECTIVE + trans.dest_id;
      trans.passive = false;
      s.globals.channels.push_back(Channel(ChanType::Broadcast, trans.sync));
    } else {
      std::cout << "ModularEncoder addBaseSyncs: transition " << trans.source_id
                << " -> " << trans.dest_id << " already has sync " << trans.sync
                << std::endl;
    }
  }
}

void ModularEncoder::addInvariants(Automaton &ta,
                                   const std::vector<State> filter,
                                   std::string inv) {
  for (const auto &f_state : filter) {
    auto target = std::find_if(
        ta.states.begin(), ta.states.end(),
        [f_state](const State &s) bool { return f_state.id == s.id; });
    if (target != ta.states.end()) {
      target->inv = addConstraint(target->inv, inv);
    } else {
      std::cout << "ModularEncoder addInvariants: filter state not found (id "
                << f_state.id << ")" << std::endl;
    }
  }
}

void ModularEncoder::encodeNoOp(AutomataSystem &s, std::vector<State> &targets,
                                const std::string opsync, int base_index) {
  Automaton base_copy = filterAutomaton(s.instances[base_index].first,
                                        s.instances[base_index].first.states,
                                        "baseX" + opsync + "X");
  base_copy.states.push_back(State("trap", ""));
  std::vector<State> bc_filter = updateFilter(base_copy, targets);
  std::vector<State> rev_bc_filter = updateFilter(
      base_copy, reverseFilter(s.instances[base_index].first, targets));
  addTrapTransitions(base_copy, rev_bc_filter, "", "", opsync);
  std::vector<Transition> self_loops = createCopyTransitionsBetweenTAs(
      base_copy, base_copy, bc_filter, "", "", opsync);
  base_copy.transitions.insert(base_copy.transitions.end(), self_loops.begin(),
                               self_loops.end());
  s.instances.push_back(std::make_pair(base_copy, ""));
}
void ModularEncoder::encodeFuture(AutomataSystem &s,
                                  std::vector<State> &targets,
                                  const std::string opsync, const Bounds bounds,
                                  int base_index) {
  Automaton base_copy = filterAutomaton(s.instances[base_index].first,
                                        s.instances[base_index].first.states,
                                        "baseX" + opsync + "X");
  Automaton sat = filterAutomaton(s.instances[base_index].first,
                                  s.instances[base_index].first.states,
                                  "satX" + opsync + "X");
  std::vector<State> sat_filter = updateFilter(sat, targets);
  std::vector<State> rev_bc_filter =
      reverseFilter(base_copy, updateFilter(base_copy, targets));
  std::string clock = "clX" + opsync;
  bool upper_bounded = (bounds.y != std::numeric_limits<int>::max());
  bool lower_bounded = (bounds.x != 0 || bounds.l_op != "&lt;=");
  std::string guard_upper_bound_crossed =
      clock + inverse_op(bounds.r_op) + std::to_string(bounds.y);
  std::string guard_constraint_sat =
      (lower_bounded
           ? clock + reverse_op(bounds.l_op) + std::to_string(bounds.x)
           : "") +
      ((lower_bounded && upper_bounded) ? "&amp;&amp;" : "") +
      (upper_bounded ? clock + bounds.r_op + std::to_string(bounds.y) : "");
  if (upper_bounded) {
    addInvariants(sat, sat.states,
                  clock + bounds.r_op + std::to_string(bounds.y));
  }
  std::vector<Transition> base_to_sat = createCopyTransitionsBetweenTAs(
      base_copy, sat, base_copy.states, "", clock + " = 0", opsync);
  std::vector<Transition> sat_to_base = createCopyTransitionsBetweenTAs(
      sat, base_copy, sat_filter, guard_constraint_sat, "", "");
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
  Automaton base_copy = filterAutomaton(s.instances[base_index].first,
                                        s.instances[base_index].first.states,
                                        "baseX" + opsync + "X");
  Automaton sat = filterAutomaton(s.instances[base_index].first, targets,
                                  "satX" + opsync + "X");
  Automaton remainder = filterAutomaton(s.instances[base_index].first,
                                        s.instances[base_index].first.states,
                                        "remX" + opsync + "X");
  std::vector<State> bc_filter = updateFilter(base_copy, targets);
  std::string clock = "clX" + opsync;
  bool upper_bounded = (bounds.y != std::numeric_limits<int>::max());
  bool lower_bounded = (bounds.x != 0 || bounds.l_op != "&lt;=");
  std::string guard_lower_bound_not_met =
      clock + inverse_op(reverse_op(bounds.l_op)) + std::to_string(bounds.x);
  std::string guard_upper_bound_crossed =
      clock + inverse_op(bounds.r_op) + std::to_string(bounds.y);
  std::string guard_constraint_sat =
      (lower_bounded
           ? clock + reverse_op(bounds.l_op) + std::to_string(bounds.x)
           : "") +
      ((lower_bounded && upper_bounded) ? "&amp;&amp;" : "") +
      (upper_bounded ? clock + bounds.r_op + std::to_string(bounds.y) : "");

  if (upper_bounded) {
    addInvariants(sat, sat.states, clock + " &lt;= 0");
    addInvariants(remainder, remainder.states,
                  clock + bounds.r_op + std::to_string(bounds.y));
  }
  std::vector<Transition> base_to_sat = createCopyTransitionsBetweenTAs(
      base_copy, sat, bc_filter, "", clock + " = 0", "");
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
  setBaseIds(s.instances[base_pos].first);
  addBaseSyncs(s, base_pos);
}
