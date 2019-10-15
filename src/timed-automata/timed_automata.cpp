/****************************************************************************
 * timed_automata.cpp - provides datastructures to represent timed automata *
 *                    systems and encodings                                 *
 *                                                                          *
 * Author(s): (2019) Tarik Viehmann                                         *
 ****************************************************************************/

#include "timed_automata.h"
#include "../utils.h"
#include <algorithm>

using namespace taptenc;

state::state(::std::string arg_id, ::std::string arg_inv, bool arg_urgent,
             bool arg_initial)
    : id(arg_id), inv(arg_inv), urgent(arg_urgent), initial(arg_initial) {}

bool state::operator<(const state &r) const { return id + inv < r.id + r.inv; }

transition::transition(::std::string arg_source_id, ::std::string arg_dest_id,
                       std::string arg_action, ::std::string arg_guard,
                       ::std::string arg_update, ::std::string arg_sync,
                       bool arg_passive)
    : source_id(arg_source_id), dest_id(arg_dest_id), action(arg_action),
      guard(arg_guard), update(arg_update), sync(arg_sync),
      passive(arg_passive) {}

bool transition::operator<(const transition &r) const {
  return source_id + dest_id + guard + update + sync <
         r.source_id + r.dest_id + r.guard + r.update + r.sync;
}

automaton::automaton(::std::vector<State> arg_states,
                     ::std::vector<Transition> arg_transitions,
                     ::std::string arg_prefix, bool setTrap) {
  states = arg_states;
  if (setTrap == true && states.end() == find_if(states.begin(), states.end(),
                                                 [](const State &s) -> bool {
                                                   return s.id == "trap";
                                                 })) {
    states.push_back(State("trap", ""));
  }
  transitions = arg_transitions;
  prefix = arg_prefix;
}

tlEntry::tlEntry(Automaton &arg_ta, ::std::vector<Transition> arg_trans_out)
    : ta(arg_ta), trans_out(arg_trans_out) {}
