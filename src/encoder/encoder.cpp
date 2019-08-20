#include "encoder.h"
#include "filter.h"
#include "timed_automata.h"
#include "utils.h"
#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#define BASE_SYNC_CONNECTIVE "XtoX"

using namespace taptenc;
Automaton Encoder::mergeAutomata(const std::vector<Automaton> &automata,
                                 std::vector<Transition> &interconnections,
                                 std::string prefix) {
  std::set<State> res_states;
  std::set<Transition> res_transitions(interconnections.begin(),
                                       interconnections.end());
  std::set<std::string> res_clocks;
  std::set<std::string> res_bool_vars;
  for (const auto &ta : automata) {
    res_states.insert(ta.states.begin(), ta.states.end());
    res_transitions.insert(ta.transitions.begin(), ta.transitions.end());
    res_clocks.insert(ta.clocks.begin(), ta.clocks.end());
    res_bool_vars.insert(ta.bool_vars.begin(), ta.bool_vars.end());
  }
  Automaton res(
      std::vector<State>(res_states.begin(), res_states.end()),
      std::vector<Transition>(res_transitions.begin(), res_transitions.end()),
      prefix, false);
  res.clocks = std::vector<std::string>(res_clocks.begin(), res_clocks.end());
  res.bool_vars =
      std::vector<std::string>(res_bool_vars.begin(), res_bool_vars.end());
  return res;
}

std::vector<Transition> Encoder::createCopyTransitionsBetweenTAs(
    const Automaton &source, const Automaton &dest,
    const std::vector<State> &filter, std::string guard, std::string update,
    std::string sync, bool passive) {
  std::vector<Transition> res_transitions;
  for (const auto &f_state : filter) {
    auto c_source =
        std::find_if(source.states.begin(), source.states.end(),
                     [f_state, this](const State &s) bool {
                       return Filter::getSuffix(s.id, BASE_SEP) ==
                              Filter::getSuffix(f_state.id, BASE_SEP);
                     });
    auto c_dest = std::find_if(dest.states.begin(), dest.states.end(),
                               [f_state, this](const State &s) bool {
                                 return Filter::getSuffix(s.id, BASE_SEP) ==
                                        Filter::getSuffix(f_state.id, BASE_SEP);
                               });
    if (c_source != source.states.end() && c_dest != dest.states.end()) {
      std::cout << c_source->id << " -> " << c_dest->id << std::endl;
      res_transitions.push_back(
          Transition(c_source->id, c_dest->id, guard, update, sync, passive));
    }
  }
  return res_transitions;
}

std::vector<Transition> Encoder::createSuccessorTransitionsBetweenTAs(
    const Automaton &base, const Automaton &source, const Automaton &dest,
    const std::vector<State> &filter, std::string guard, std::string update) {
  std::vector<Transition> res_transitions;
  for (const auto &trans : base.transitions) {
    auto search = std::find_if(
        filter.begin(), filter.end(), [trans, this](const State &s) bool {
          return Filter::getSuffix(s.id, BASE_SEP) ==
                 Filter::getSuffix(trans.source_id, BASE_SEP);
        });
    if (search != filter.end()) {
      auto source_state = std::find_if(
          source.states.begin(), source.states.end(),
          [search](const State &s) bool { return s.id == search->id; });
      auto dest_state =
          std::find_if(dest.states.begin(), dest.states.end(),
                       [trans, this](const State &s) bool {
                         return Filter::getSuffix(s.id, BASE_SEP) ==
                                Filter::getSuffix(trans.dest_id, BASE_SEP);
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

void Encoder::addTrapTransitions(Automaton &ta,
                                 const std::vector<State> &sources,
                                 std::string guard, std::string update,
                                 std::string sync, bool passive) {
  auto trap =
      std::find_if(ta.states.begin(), ta.states.end(),
                   [](const State &s) bool { return s.name == "trap"; });
  if (trap == ta.states.end()) {
    std::cout << "Encoder addTrapTransitions: trap not found. Abort."
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
      std::cout << "Encoder addTrapTransitions: id of source not "
                   "found in TA (id "
                << source.id << ")" << std::endl;
    }
  }
}
void Encoder::addBaseSyncs(AutomataSystem &s, const int base_pos) {
  for (auto &trans : s.instances[base_pos].first.transitions) {
    if (trans.sync == "") {
      trans.sync = trans.source_id + BASE_SYNC_CONNECTIVE + trans.dest_id;
      trans.passive = false;
      s.globals.channels.push_back(Channel(ChanType::Broadcast, trans.sync));
    } else {
      std::cout << "Encoder addBaseSyncs: transition " << trans.source_id
                << " -> " << trans.dest_id << " already has sync " << trans.sync
                << std::endl;
    }
  }
}

void Encoder::addInvariants(Automaton &ta, const std::vector<State> filter,
                            std::string inv) {
  for (const auto &f_state : filter) {
    auto target = std::find_if(
        ta.states.begin(), ta.states.end(),
        [f_state](const State &s) bool { return f_state.id == s.id; });
    if (target != ta.states.end()) {
      target->inv = addConstraint(target->inv, inv);
    } else {
      std::cout << "Encoder addInvariants: filter state not found (id "
                << f_state.id << ")" << std::endl;
    }
  }
}

std::string Encoder::toPrefix(std::string op, std::string sub, std::string pa) {
  std::string res;
  res += pa;
  res.push_back(SUB_CONSTRAINT_SEP);
  res += op;
  res.push_back(CONSTRAINT_SEP);
  res += sub;
  res.push_back(BASE_SEP);
  return res;
}
