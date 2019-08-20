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

Automaton generatePlanAutomaton(
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
  full_plan.push_back("alphafin");
  vector<State> plan_states;
  for (auto it = full_plan.begin(); it != full_plan.end(); ++it) {
    bool urgent = false;
    if (it->substr(0, 5) != "alpha") {
      urgent = true;
    }
    plan_states.push_back(
        State(*it + "X" + to_string(it - full_plan.begin()), "", urgent));
  }
  vector<Transition> plan_transitions;
  int i = 0;
  for (auto it = plan_states.begin() + 1; it < plan_states.end(); ++it) {
    string sync_op = "";
    string guard = "";
    string update = "";
    auto prev_state = (it - 1);
    if (prev_state->name.substr(0, 5) != "alpha") {
      sync_op = prev_state->name;
      size_t x_pos = sync_op.find_first_of("X");
      sync_op = sync_op.substr(0, x_pos);
    } else {
      guard = "cpa &gt; 30";
      update = "cpa = 0";
    }
    plan_transitions.push_back(
        Transition(prev_state->id, it->id, guard, update, sync_op, false));

    prev_state->name += "pa" + to_string(i);
    i++;
  }
  Automaton res = Automaton(plan_states, plan_transitions, arg_name);
  res.clocks.push_back("cpa");
  return res;
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
  vector<pair<Automaton, string>> automata;
  automata.push_back(make_pair(test, ""));
  vector<int> plan{3, 1, 2, 3, 4, 3, 2, 5, 8, 7, 2, 4, 6};
  unordered_map<int, vector<string>> activations;
  activations[1] = vector<string>{"snoop1"};
  activations[4] = vector<string>{"snoop1", "sfinally1"};
  activations[2] = vector<string>{"spast1", "spast7"};
  activations[5] = vector<string>{"spast2", "spast8"};
  activations[6] = vector<string>{"spast3", "spast9"};
  activations[7] = vector<string>{"spast4", "spast10"};
  activations[8] = vector<string>{"spast5", "spast6"};
  automata.push_back(
      make_pair(generatePlanAutomaton(plan, activations, "plan"), ""));
  AutomataSystem my_system;
  my_system.instances = automata;
  my_system.globals = glob;
  ModularEncoder enc(my_system);
  enc.encodePast(my_system, bot, "spast1", b);
  enc.encodePast(my_system, top, "spast2", b);
  enc.encodePast(my_system, idles, "spast3", b);
  enc.encodePast(my_system, left, "spast4", b);
  enc.encodePast(my_system, right, "spast5", b);
  enc.encodePast(my_system, bot, "spast6", b);
  enc.encodePast(my_system, top, "spast7", b);
  enc.encodePast(my_system, idles, "spast8", b);
  enc.encodePast(my_system, left, "spast9", b);
  enc.encodePast(my_system, right, "spast10", b);
  enc.encodeNoOp(my_system, right, "snoop1");
  enc.encodeFuture(my_system, left, "sfinally1", b);
  SystemVisInfo my_system_vis_info(my_system);
  printer.print(my_system, my_system_vis_info, "test.xml");
  xta_printer.print(my_system, my_system_vis_info, "test.xta");
}
