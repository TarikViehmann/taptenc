#pragma once
#include "../utils.h"
#include "constraints.h"
#include <algorithm>
#include <string>
#include <vector>

namespace taptenc {

struct state {
  static int obj_count;
  ::std::string id;
  ::std::string name;
  ::std::string inv;
  bool urgent;
  bool initial;
  state(::std::string arg_name, ::std::string arg_inv, bool arg_urgent = false,
        bool arg_initial = false) {
    name = arg_name;
    // name = "id" + ::std::to_string(obj_count);
    obj_count++;
    id = arg_name;
    inv = arg_inv;
    urgent = arg_urgent;
    initial = arg_initial;
  }
};
typedef struct state State;

struct transition {
  ::std::string source_id;
  ::std::string dest_id;
  ::std::string guard;
  ::std::string update;
  ::std::string sync;
  bool passive;
  transition(::std::string arg_source_id, ::std::string arg_dest_id,
             ::std::string arg_guard, ::std::string arg_update,
             ::std::string arg_sync, bool arg_passive = false) {
    source_id = arg_source_id;
    dest_id = arg_dest_id;
    guard = arg_guard;
    update = arg_update;
    sync = arg_sync;
    passive = arg_passive;
  }
};
typedef struct transition Transition;

enum ChanType { Binary = 1, Broadcast = 0 };
struct channel {
  ChanType type;
  ::std::string name;
  channel(ChanType arg_type, ::std::string arg_name) {
    type = arg_type;
    name = arg_name;
  }
};
typedef struct channel Channel;

struct automataGlobals {
  ::std::vector<::std::string> clocks;
  ::std::vector<::std::string> bool_vars;
  ::std::vector<Channel> channels;
};
typedef struct automataGlobals AutomataGlobals;

struct automaton {
  ::std::vector<State> states;
  ::std::vector<Transition> transitions;
  ::std::vector<::std::string> clocks;
  ::std::vector<::std::string> bool_vars;
  ::std::string prefix;

  automaton(::std::vector<State> arg_states,
            ::std::vector<Transition> arg_transitions, ::std::string arg_prefix,
            bool setTrap = true) {
    states = arg_states;
    if (setTrap == true && states.end() == find_if(states.begin(), states.end(),
                                                   [](const State &s) -> bool {
                                                     return s.name == "trap";
                                                   })) {
      states.push_back(State("trap", ""));
    }
    transitions = arg_transitions;
    prefix = arg_prefix;
  }
};
typedef struct automaton Automaton;

struct automataSystem {
  ::std::vector<::std::pair<Automaton, ::std::string>> instances;
  AutomataGlobals globals;
};
typedef struct automataSystem AutomataSystem;

} // end namespace taptenc
