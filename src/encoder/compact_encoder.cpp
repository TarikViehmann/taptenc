#include "encoders.h"
#include "timed_automata.h"
#include "utils.h"
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

using namespace taptenc;

int id_suffix = 0;
int fresh_suffix() {
  id_suffix++;
  return id_suffix;
}

void CompactEncoder::encodeNoOp(AutomataSystem &s, std::vector<State> &target,
                                const std::string opsync, const int base_pos) {
  auto trap =
      std::find_if(s.instances[base_pos].first.states.begin(),
                   s.instances[base_pos].first.states.end(),
                   [](const State &s) -> bool { return s.name == "trap"; });
  if (trap == s.instances[base_pos].first.states.end()) {
    std::cout << "CompactEncoder encodeNoOp: trap not found" << std::endl;
    return;
  }
  for (auto it = s.instances[base_pos].first.states.begin();
       it != s.instances[base_pos].first.states.end(); ++it) {
    std::string key = it->name;
    auto its =
        std::find_if(target.begin(), target.end(),
                     [key](const State &s) -> bool { return s.name == key; });
    if (its != target.end()) {
      s.instances[base_pos].first.transitions.push_back(
          Transition(it->id, it->id, "", "", opsync, true));
    } else if (it->id != trap->id) {
      s.instances[base_pos].first.transitions.push_back(
          Transition(it->id, trap->id, "", "", opsync, true));
    }
  }
}

void CompactEncoder::encodeFuture(AutomataSystem &s, std::vector<State> &target,
                                  std::string opsync, Bounds bounds,
                                  int base_pos) {
  auto trap =
      std::find_if(s.instances[base_pos].first.states.begin(),
                   s.instances[base_pos].first.states.end(),
                   [](const State &s) -> bool { return s.name == "trap"; });
  int suffix = fresh_suffix();
  std::string boolvar = "bfinally" + std::to_string(suffix);
  std::string clock = "cfinally" + std::to_string(suffix);
  s.instances[base_pos].first.clocks.push_back(clock);
  s.instances[base_pos].first.bool_vars.push_back(boolvar);
  if (trap == s.instances[base_pos].first.states.end()) {
    std::cout << "Compact Encoder encodeFuture: trap not found" << std::endl;
    return;
  }
  std::string invariant = "";
  if (bounds.y != std::numeric_limits<int>::max()) {
    invariant += "(" + boolvar + " == false || " + clock + bounds.r_op +
                 std::to_string(bounds.y) + ")";
  }
  bool upper_bounded = (bounds.y != std::numeric_limits<int>::max());
  bool lower_bounded = (bounds.x != 0 || bounds.l_op != "&lt;=");
  std::string guard_upper_bound_crossed = clock + inverse_op(bounds.r_op) +
                                          std::to_string(bounds.y) +
                                          " &amp;&amp; " + boolvar + "==true";
  std::string guard_constraint_sat =
      (lower_bounded
           ? clock + reverse_op(bounds.l_op) + std::to_string(bounds.x)
           : "") +
      ((lower_bounded && upper_bounded) ? "&amp;&amp;" : "") +
      (upper_bounded ? clock + bounds.r_op + std::to_string(bounds.y) : "");
  for (auto it = s.instances[base_pos].first.states.begin();
       it != s.instances[base_pos].first.states.end(); ++it) {
    if (it->name == "trap") {
      continue;
    }
    std::string key = it->name;
    auto its = find_if(target.begin(), target.end(),
                       [key](const State &s) -> bool { return s.name == key; });
    std::string update_on_activation = "";
    if (its == target.end() || lower_bounded) {
      update_on_activation = boolvar + " = true, " + clock + " = 0";
    }
    // self loop that triggers upon constraint activation
    s.instances[base_pos].first.transitions.push_back(
        Transition(it->id, it->id, "", update_on_activation, opsync, true));
    // transitions to trap once above y
    if (upper_bounded) {
      s.instances[base_pos].first.transitions.push_back(Transition(
          it->id, trap->id, guard_upper_bound_crossed, "", "", true));
    }
    it->inv = addConstraint(it->inv, invariant);
    if (its != target.end()) {
      // transitions when constraint is satisfied
      s.instances[base_pos].first.transitions.push_back(
          Transition(it->id, it->id, guard_constraint_sat, boolvar + " = false",
                     "", true));
    }
  }
}

void CompactEncoder::encodePast(AutomataSystem &s, std::vector<State> &target,
                                std::string opsync, Bounds bounds,
                                int base_pos) {
  auto trap =
      std::find_if(s.instances[base_pos].first.states.begin(),
                   s.instances[base_pos].first.states.end(),
                   [](const State &s) -> bool { return s.name == "trap"; });
  int suffix = fresh_suffix();
  std::string boolvar = "bpast" + std::to_string(suffix);
  std::string clock = "cpast" + std::to_string(suffix);
  s.instances[base_pos].first.clocks.push_back(clock);
  s.instances[base_pos].first.bool_vars.push_back(boolvar);
  if (trap == s.instances[base_pos].first.states.end()) {
    std::cout << "Compact Encoder encodePast: trap not found" << std::endl;
    return;
  }
  std::string invariant = "";
  if (bounds.y != std::numeric_limits<int>::max()) {
    invariant += "(" + boolvar + " == false || " + clock + bounds.r_op +
                 std::to_string(bounds.y) + ")";
  }
  bool upper_bounded = (bounds.y != std::numeric_limits<int>::max());
  bool lower_bounded = (bounds.x != 0 || bounds.l_op != "&lt;=");
  std::string guard_upper_bound_crossed = clock + inverse_op(bounds.r_op) +
                                          std::to_string(bounds.y) +
                                          " &amp;&amp; " + boolvar + "==true";
  std::string guard_lower_bound_not_reached =
      clock + inverse_op(reverse_op(bounds.l_op)) + std::to_string(bounds.x);
  std::string guard_constraint_sat =
      (lower_bounded
           ? clock + reverse_op(bounds.l_op) + std::to_string(bounds.x)
           : "") +
      ((lower_bounded && upper_bounded) ? "&amp;&amp;" : "") +
      (upper_bounded ? clock + bounds.r_op + std::to_string(bounds.y) : "");
  for (auto it = s.instances[base_pos].first.states.begin();
       it != s.instances[base_pos].first.states.end(); ++it) {
    if (it->name == "trap") {
      continue;
    }
    std::string key = it->name;
    auto its = find_if(target.begin(), target.end(),
                       [key](const State &s) -> bool { return s.name == key; });
    std::string update_on_activation = "";
    if (its == target.end() || lower_bounded) {
      update_on_activation = boolvar + " = true," + clock + " = 0";
    }
    // transitions to trap once above y
    // or below x
    if (upper_bounded) {
      s.instances[base_pos].first.transitions.push_back(Transition(
          it->id, trap->id, guard_upper_bound_crossed, "", "", true));
      s.instances[base_pos].first.transitions.push_back(Transition(
          it->id, trap->id, guard_lower_bound_not_reached, "", opsync, true));
    }
    it->inv = addConstraint(it->inv, invariant);
    if (its != target.end()) {
      // transitions when constraint is satisfied
      s.instances[base_pos].first.transitions.push_back(
          Transition(it->id, it->id, guard_constraint_sat, boolvar + " = false",
                     opsync, true));
    }
    // self loop that triggers upon constraint activation
    s.instances[base_pos].first.transitions.push_back(
        Transition(it->id, it->id, "", update_on_activation, "", true));
  }
}
