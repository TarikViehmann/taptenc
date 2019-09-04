#include "encoders.h"
#include "filter.h"
#include "printer.h"
#include "timed_automata.h"
#include "vis_info.h"
#include <algorithm>
#include <assert.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <unordered_map>
#include <vector>

using namespace taptenc;
using namespace std;

void append_prefix_to_states(vector<State> &arg_states, string prefix) {
  for (auto it = arg_states.begin(); it != arg_states.end(); ++it) {
    it->name += prefix;
  }
}
Automaton generatePlanAutomaton(const vector<int> &arg_plan, string arg_name) {
  vector<string> full_plan;
  transform(arg_plan.begin(), arg_plan.end(), back_inserter(full_plan),
            [](const int pa) -> string { return "alpha" + to_string(pa); });
  full_plan.push_back(constants::END_PA);
  full_plan.insert(full_plan.begin(), constants::START_PA);
  vector<State> plan_states;
  for (auto it = full_plan.begin(); it != full_plan.end(); ++it) {
    plan_states.push_back(
        State(*it + constants::PA_SEP + to_string(it - full_plan.begin()),
              (*it == "fin" || *it == "start") ? "" : "cpa &lt; 60"));
  }
  vector<Transition> plan_transitions;
  int i = 0;
  for (auto it = plan_states.begin() + 1; it < plan_states.end(); ++it) {
    string sync_op = "";
    string guard = "";
    string update = "";
    auto prev_state = (it - 1);
    if (prev_state->id != constants::END_PA &&
        prev_state->id != constants::START_PA) {
      guard = "cpa &gt; 30";
      update = "cpa = 0";
    }
    plan_transitions.push_back(
        Transition(prev_state->id, it->id, guard, update, sync_op, false));
    i++;
  }
  Automaton res = Automaton(plan_states, plan_transitions, arg_name, false);
  res.clocks.push_back("cpa");
  return res;
}

Automaton generateSyncPlanAutomaton(
    const vector<int> &arg_plan,
    const unordered_map<int, vector<string>> &arg_constraint_activations,
    string arg_name) {
  vector<pair<int, vector<string>>> syncs_to_add;
  vector<string> full_plan;
  transform(arg_plan.begin(), arg_plan.end(), back_inserter(full_plan),
            [](const int pa) -> string { return "alpha" + to_string(pa); });
  for (auto it = arg_plan.begin(); it != arg_plan.end(); ++it) {
    auto activate = arg_constraint_activations.find(*it);
    if (activate != arg_constraint_activations.end()) {
      syncs_to_add.push_back(
          make_pair(it - arg_plan.begin(), activate->second));
    }
  }
  int offset = 0;
  for (auto it = syncs_to_add.begin(); it != syncs_to_add.end(); ++it) {
    full_plan.insert(full_plan.begin() + offset + it->first, it->second.begin(),
                     it->second.end());
    offset += it->second.size();
  }
  full_plan.push_back(constants::END_PA);
  full_plan.insert(full_plan.begin(), constants::START_PA);
  vector<State> plan_states;
  for (auto it = full_plan.begin(); it != full_plan.end(); ++it) {
    bool urgent = false;
    if (it->substr(0, 5) != "alpha") {
      urgent = true;
    }
    plan_states.push_back(
        State(*it + constants::PA_SEP + to_string(it - full_plan.begin()),
              ((*it == constants::END_PA || *it == constants::START_PA)
                   ? ""
                   : "cpa &lt; 60"),
              urgent));
  }
  vector<Transition> plan_transitions;
  int i = 0;
  for (auto it = plan_states.begin() + 1; it < plan_states.end(); ++it) {
    string sync_op = "";
    string guard = "";
    string update = "";
    auto prev_state = (it - 1);
    if (prev_state->name.substr(0, 5) != "alpha") {
      sync_op = Filter::getPrefix(prev_state->name, constants::PA_SEP);
    } else {
      guard = "cpa &gt; 30";
      update = "cpa = 0";
    }
    plan_transitions.push_back(
        Transition(prev_state->id, it->id, guard, update, sync_op, false));
    i++;
  }
  Automaton res = Automaton(plan_states, plan_transitions, arg_name, false);
  res.clocks.push_back("cpa");
  return res;
}

DirectEncoder
createDirectEncoding(AutomataSystem &direct_system,
                     const unordered_map<int, vector<string>> &activations,
                     unordered_map<string, vector<State>> &targets, Bounds b,
                     int plan_index = 1) {
  DirectEncoder enc(direct_system);
  for (const auto &pa : direct_system.instances[plan_index].first.states) {
    string pa_op = Filter::getPrefix(pa.id, constants::PA_SEP);
    if (pa_op.substr(5) == "start" || pa_op.substr(5) == "fin") {
      continue;
    }
    int pa_id = stoi(pa_op.substr(5));
    auto search = activations.find(pa_id);
    if (search != activations.end()) {
      for (const auto &ac : search->second) {
        auto search = targets.find(ac);
        if (search != targets.end()) {
          if (ac.substr(0, 5) == "snoop") {
            enc.encodeNoOp(direct_system, search->second, pa.id);
          }
          if (ac.substr(0, 5) == "spast") {
            enc.encodePast(direct_system, search->second, pa.id, b);
          }
          if (ac.substr(0, 8) == "sfinally") {
            enc.encodeFuture(direct_system, search->second, pa.id, b);
          }
        }
      }
    }
  }
  return enc;
}

ModularEncoder
createModularEncoding(AutomataSystem &system, const AutomataGlobals g,
                      unordered_map<string, vector<State>> &targets, Bounds b) {
  ModularEncoder enc(system);
  for (const auto chan : g.channels) {
    auto search = targets.find(chan.name);
    if (search != targets.end()) {
      if (chan.name.substr(0, 5) == "snoop") {
        enc.encodeNoOp(system, search->second, chan.name);
      }
      if (chan.name.substr(0, 5) == "spast") {
        enc.encodePast(system, search->second, chan.name, b);
      }
      if (chan.name.substr(0, 8) == "sfinally") {
        enc.encodeFuture(system, search->second, chan.name, b);
      }
    }
  }
  return enc;
}

CompactEncoder
createCompactEncoding(AutomataSystem &system, const AutomataGlobals g,
                      unordered_map<string, vector<State>> &targets, Bounds b) {
  CompactEncoder enc;
  for (const auto chan : g.channels) {
    auto search = targets.find(chan.name);
    if (search != targets.end()) {
      if (chan.name.substr(0, 5) == "snoop") {
        enc.encodeNoOp(system, search->second, chan.name);
      }
      if (chan.name.substr(0, 5) == "spast") {
        enc.encodePast(system, search->second, chan.name, b);
      }
      if (chan.name.substr(0, 8) == "sfinally") {
        enc.encodeFuture(system, search->second, chan.name, b);
      }
    }
  }
  return enc;
}

int main() {
  vector<State> states;
  vector<Transition> transitions;
  vector<string> whatever;
  states.push_back(State("idlel", ""));
  states.push_back(State("prepl", "pleft &lt; 10"));
  states.push_back(State("busyl", "bleft&lt;= 5"));
  states.push_back(State("idler", ""));
  states.push_back(State("prepr", "pright &lt; 10"));
  states.push_back(State("busyr", "bright &lt;= 5"));
  transitions.push_back(Transition(states[0].id, states[1].id, "ileft &gt; 5",
                                   "pleft=0", "", true));
  transitions.push_back(Transition(states[1].id, states[2].id, "pleft &gt; 5",
                                   "bleft=0", "", true));
  transitions.push_back(Transition(states[2].id, states[0].id, "bleft == 5",
                                   "ileft=0", "", true));
  transitions.push_back(Transition(states[3].id, states[4].id, "iright &gt; 5",
                                   "pright=0", "", true));
  transitions.push_back(Transition(states[4].id, states[5].id, "pright &gt; 5",
                                   "bright=0", "", true));
  transitions.push_back(Transition(states[5].id, states[3].id, "bright == 5",
                                   "iright=0", "", true));
  transitions.push_back(Transition(states[2].id, states[4].id, "bleft &lt; 5",
                                   "pright=0", "", true));
  transitions.push_back(Transition(states[5].id, states[1].id, "bright &lt; 5",
                                   "pleft=0", "", true));
  Automaton test(states, transitions, "main", false);
  test.clocks.insert(test.clocks.end(),
                     {"ileft", "pleft", "bleft", "iright", "pright", "bright"});
  vector<State> right;
  vector<State> bot;
  vector<State> top;
  vector<State> left;
  vector<State> idles;
  right.insert(right.end(), {test.states[3], test.states[4], test.states[5]});
  bot.insert(bot.end(), {test.states[0], test.states[2], test.states[4]});
  left.insert(left.end(), {test.states[0], test.states[1], test.states[2]});
  top.insert(top.end(), {test.states[1], test.states[3], test.states[4]});
  idles.insert(idles.end(), {test.states[0], test.states[3]});
  Bounds b;
  b.l_op = "&lt;=";
  b.r_op = "&lt;=";
  b.x = 5;
  b.y = 10;
  AutomataGlobals glob;
  glob.channels.push_back(Channel(ChanType::Binary, "sfinally1"));
  glob.channels.push_back(Channel(ChanType::Binary, "snoop1"));
  glob.channels.push_back(Channel(ChanType::Binary, "spast1"));
  glob.channels.push_back(Channel(ChanType::Binary, "spast2"));
  glob.channels.push_back(Channel(ChanType::Binary, "spast3"));
  glob.channels.push_back(Channel(ChanType::Binary, "spast4"));
  glob.channels.push_back(Channel(ChanType::Binary, "spast5"));
  glob.channels.push_back(Channel(ChanType::Binary, "spast6"));
  glob.channels.push_back(Channel(ChanType::Binary, "spast7"));
  glob.channels.push_back(Channel(ChanType::Binary, "spast8"));
  glob.channels.push_back(Channel(ChanType::Binary, "spast9"));
  glob.channels.push_back(Channel(ChanType::Binary, "spast10"));
  XMLPrinter printer;
  XTAPrinter xta_printer;
  vector<pair<Automaton, string>> automata_direct;
  vector<pair<Automaton, string>> automata_sync;
  automata_direct.push_back(make_pair(test, ""));
  automata_sync.push_back(make_pair(test, ""));
  automata_sync[0].first.states.push_back(State("trap", ""));
  vector<int> plan{3, 1, 2, 3, 4, 3, 2, 5, 8, 4, 6};
  unordered_map<int, vector<string>> activations;
  activations[1] = vector<string>{"snoop1"};
  activations[4] = vector<string>{"snoop1", "sfinally1"};
  activations[2] = vector<string>{"spast1", "spast7"};
  activations[5] = vector<string>{"spast2", "spast8"};
  activations[6] = vector<string>{"spast3", "spast9"};
  activations[7] = vector<string>{"spast4", "spast10"};
  activations[8] = vector<string>{"spast5", "spast6"};
  unordered_map<string, vector<State>> targets;
  targets["spast1"] = bot;
  targets["spast2"] = top;
  targets["spast3"] = idles;
  targets["spast4"] = left;
  targets["spast5"] = right;
  targets["spast6"] = bot;
  targets["spast7"] = top;
  targets["spast8"] = idles;
  targets["spast9"] = left;
  targets["spast10"] = right;
  targets["snoop1"] = right;
  targets["sfinally1"] = left;
  Automaton plan_ta = generatePlanAutomaton(plan, "plan");
  Automaton plan_sync_ta = generateSyncPlanAutomaton(plan, activations, "plan");
  automata_direct.push_back(make_pair(plan_ta, ""));
  automata_sync.push_back(make_pair(plan_sync_ta, ""));
  AutomataSystem direct_system;
  AutomataSystem sync_system;
  direct_system.instances = automata_direct;
  sync_system.instances = automata_sync;
  sync_system.globals = glob;
  DirectEncoder enc1 =
      createDirectEncoding(direct_system, activations, targets, b);
  enc1.createFinalSystem(direct_system);
  auto internal = enc1.getInternalTAs();
  SystemVisInfo direct_system_vis_info(internal, plan_ta.states);
  printer.print(direct_system, direct_system_vis_info, "direct.xml");
  xta_printer.print(direct_system, direct_system_vis_info, "direct.xta");
  AutomataSystem modular_system = sync_system;
  createModularEncoding(modular_system, glob, targets, b);
  SystemVisInfo modular_system_vis_info(modular_system);
  printer.print(modular_system, modular_system_vis_info, "modular.xml");
  xta_printer.print(modular_system, modular_system_vis_info, "modular.xta");
  AutomataSystem compact_system = sync_system;
  createCompactEncoding(compact_system, glob, targets, b);
  SystemVisInfo compact_system_vis_info(compact_system);
  printer.print(compact_system, compact_system_vis_info, "compact.xml");
  xta_printer.print(compact_system, compact_system_vis_info, "compact.xta");
}
