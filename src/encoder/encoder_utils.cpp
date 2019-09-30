#include "encoder_utils.h"
#include "constants.h"
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
Automaton
encoderutils::generatePlanAutomaton(const std::vector<PlanAction> &plan,
                                    std::string name) {
  std::vector<PlanAction> full_plan = plan;
  full_plan.push_back(PlanAction(constants::END_PA,
                                 Bounds(0, std::numeric_limits<int>::max())));
  full_plan.insert(full_plan.begin(),
                   PlanAction(constants::START_PA,
                              Bounds(0, std::numeric_limits<int>::max())));
  std::vector<State> plan_states;
  for (auto it = full_plan.begin(); it != full_plan.end(); ++it) {
    plan_states.push_back(
        State((it->name == constants::END_PA || it->name == constants::START_PA)
                  ? it->name
                  : it->name + constants::PA_SEP +
                        std::to_string(it - full_plan.begin()),
              (it->name == constants::END_PA || it->name == constants::START_PA)
                  ? ""
                  : ("cpa " + it->duration.r_op + " " +
                     std::to_string(it->duration.upper_bound)),
              false, (it->name == constants::START_PA) ? true : false));
  }
  auto find_initial =
      std::find_if(plan_states.begin(), plan_states.end(),
                   [](const State &s) bool { return s.initial; });
  if (find_initial == plan_states.end()) {
    std::cout << "generatePlanAutomaton: no initial state found" << std::endl;
  } else {
    std::cout << "generatePlanAutomaton: initial state: " << find_initial->id
              << std::endl;
  }
  std::vector<Transition> plan_transitions;
  int i = 0;
  for (auto it = plan_states.begin() + 1; it < plan_states.end(); ++it) {
    std::string sync_op = "";
    std::string guard = "";
    std::string update = "";
    auto prev_state = (it - 1);
    int pa_index = prev_state - plan_states.begin();
    guard = "cpa " + reverse_op(full_plan[pa_index].duration.l_op) + " " +
            std::to_string(full_plan[pa_index].duration.lower_bound);
    update = "cpa = 0";
    plan_transitions.push_back(Transition(prev_state->id, it->id, it->id, guard,
                                          update, sync_op, false));
    i++;
  }
  Automaton res = Automaton(plan_states, plan_transitions, name, false);
  res.clocks.push_back("cpa");
  return res;
}

Automaton encoderutils::mergeAutomata(const std::vector<Automaton> &automata,
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

std::vector<Transition> encoderutils::createCopyTransitionsBetweenTAs(
    const Automaton &source, const Automaton &dest,
    const std::vector<State> &filter, std::string guard, std::string update,
    std::string sync, bool passive) {
  std::vector<Transition> res_transitions;
  for (const auto &f_state : filter) {
    auto c_source = std::find_if(
        source.states.begin(), source.states.end(),
        [f_state](const State &s) bool {
          return Filter::getSuffix(s.id, constants::BASE_SEP) ==
                 Filter::getSuffix(f_state.id, constants::BASE_SEP);
        });
    auto c_dest = std::find_if(
        dest.states.begin(), dest.states.end(), [f_state](const State &s) bool {
          return Filter::getSuffix(s.id, constants::BASE_SEP) ==
                 Filter::getSuffix(f_state.id, constants::BASE_SEP);
        });
    if (c_source != source.states.end() && c_dest != dest.states.end()) {
      res_transitions.push_back(Transition(c_source->id, c_dest->id, "", guard,
                                           update, sync, passive));
    }
  }
  return res_transitions;
}

std::vector<Transition> encoderutils::createSuccessorTransitionsBetweenTAs(
    const Automaton &base, const Automaton &source, const Automaton &dest,
    const std::vector<State> &filter, std::string guard, std::string update) {
  std::vector<Transition> res_transitions;
  for (const auto &trans : base.transitions) {
    auto search = std::find_if(
        filter.begin(), filter.end(), [trans](const State &s) bool {
          return Filter::getSuffix(s.id, constants::BASE_SEP) ==
                 Filter::getSuffix(trans.source_id, constants::BASE_SEP);
        });
    if (search != filter.end()) {
      auto source_state = std::find_if(
          source.states.begin(), source.states.end(),
          [search](const State &s) bool { return s.id == search->id; });
      auto dest_state = std::find_if(
          dest.states.begin(), dest.states.end(), [trans](const State &s) bool {
            return Filter::getSuffix(s.id, constants::BASE_SEP) ==
                   Filter::getSuffix(trans.dest_id, constants::BASE_SEP);
          });
      if (source_state != source.states.end() &&
          dest_state != dest.states.end()) {
        res_transitions.push_back(
            Transition(source_state->id, dest_state->id, trans.action,
                       addConstraint(trans.guard, guard),
                       addUpdate(trans.update, update), trans.sync, true));
      }
    }
  }
  return res_transitions;
}

void encoderutils::addTrapTransitions(Automaton &ta,
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
          Transition(search->id, trap->id, "", guard, update, sync, passive));
    } else {
      std::cout << "Encoder addTrapTransitions: id of source not "
                   "found in TA (id "
                << source.id << ")" << std::endl;
    }
  }
}
void encoderutils::addBaseSyncs(AutomataSystem &s, const int base_pos) {
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

void encoderutils::addInvariants(Automaton &ta, const std::vector<State> filter,
                                 std::string inv) {
  for (const auto &f_state : filter) {
    auto target = std::find_if(
        ta.states.begin(), ta.states.end(), [f_state](const State &s) bool {
          return Filter::matchesFilter(s.id, "", f_state.id);
        });
    if (target != ta.states.end()) {
      target->inv = addConstraint(target->inv, inv);
    }
  }
}

std::string encoderutils::toPrefix(std::string op, std::string sub,
                                   std::string pa) {
  std::string res;
  res += pa;
  res.push_back(constants::TL_SEP);
  res += op;
  res.push_back(constants::CONSTRAINT_SEP);
  res += sub;
  res.push_back(constants::BASE_SEP);
  return res;
}

std::string encoderutils::addToPrefix(std::string prefix, std::string op,
                                      std::string sub) {
  std::string res;
  std::string pa_prefix = Filter::getPrefix(prefix, constants::TL_SEP);
  pa_prefix.push_back(constants::TL_SEP);
  res += Filter::stripPrefix(prefix, pa_prefix);
  std::string prefix_to_add;
  prefix_to_add += op;
  prefix_to_add.push_back(constants::CONSTRAINT_SEP);
  prefix_to_add += sub;
  prefix_to_add.push_back(constants::CONSTRAINT_SEP);
  res = pa_prefix + prefix_to_add + res;
  return res;
}
