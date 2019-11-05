/** \file
 * Provides datastructures to represent timed automata systems and encodings.
 *
 *
 * \author: (2019) Tarik Viehmann
 */

#include "timed_automata.h"
#include "../constraints/constraints.h"
#include "../utils.h"

#include <algorithm>

using namespace taptenc;

/**
 * Helpers to help implementing move constructors.
 */
namespace moveutils {
/**
 * Swaps the members of two states using std::swap().
 *
 * @param first first state participating in the swap
 * @param second second state participating in the swap
 */
void swap(state &first, state &second) {
  std::swap(first.id, second.id);
  std::swap(first.inv, second.inv);
  std::swap(first.urgent, second.urgent);
  std::swap(first.initial, second.initial);
}

/**
 * Swaps the members of two transitions using std::swap().
 *
 * @param first first transition participating in the swap
 * @param second second transition participating in the swap
 */
void swap(transition &first, transition &second) {
  std::swap(first.source_id, second.source_id);
  std::swap(first.dest_id, second.dest_id);
  std::swap(first.action, second.action);
  std::swap(first.guard, second.guard);
  std::swap(first.update, second.update);
  std::swap(first.sync, second.sync);
  std::swap(first.passive, second.passive);
}
} // end namespace moveutils

using namespace moveutils;

state::state(::std::string arg_id, const ClockConstraint &arg_inv,
             bool arg_urgent, bool arg_initial)
    : id(arg_id), urgent(arg_urgent), initial(arg_initial) {
  inv = arg_inv.createCopy();
}

state::state(const state &other) {
  id = other.id;
  inv = other.inv->createCopy();
  urgent = other.urgent;
  initial = other.initial;
}
state::state(state &&other) noexcept {
  id = std::move(other.id);
  inv = std::move(other.inv->createCopy());
  urgent = std::move(other.urgent);
  initial = std::move(other.initial);
}

state &state::operator=(const state &other) {
  *this = state(other);
  return *this;
}

state &state::operator=(state &&other) noexcept {
  swap(*this, other);
  return *this;
}

transition::transition(const transition &other) {
  source_id = other.source_id;
  dest_id = other.dest_id;
  action = other.action;
  guard = other.guard->createCopy();
  update = other.update;
  sync = other.sync;
  passive = other.passive;
}

transition::transition(transition &&other) noexcept {
  source_id = std::move(other.source_id);
  dest_id = std::move(other.dest_id);
  action = std::move(other.action);
  guard = std::move(other.guard);
  update = std::move(other.update);
  sync = std::move(other.sync);
  passive = std::move(other.passive);
}

transition &transition::operator=(const transition &other) {
  *this = transition(other);
  return *this;
}

transition &transition::operator=(transition &&other) noexcept {
  swap(*this, other);
  return *this;
}

bool state::operator<(const state &r) const {
  return id + inv->toString() < r.id + r.inv->toString();
}

transition::transition(::std::string arg_source_id, ::std::string arg_dest_id,
                       std::string arg_action, const ClockConstraint &arg_guard,
                       const update_t &arg_update, ::std::string arg_sync,
                       bool arg_passive)
    : source_id(arg_source_id), dest_id(arg_dest_id), action(arg_action),
      update(arg_update), sync(arg_sync), passive(arg_passive) {
  guard = arg_guard.createCopy();
}

::std::string transition::updateToString() const {
  std::string res;
  bool is_first_iteration = true;
  for (const auto &clock_ptr : update) {
    if (!is_first_iteration) {
      res += ", ";
    } else {
      is_first_iteration = false;
    }
    res += clock_ptr.get()->id + " = 0";
  }
  return res;
}

bool transition::operator<(const transition &r) const {
  return source_id + dest_id + guard->toString() + updateToString() + sync <
         r.source_id + r.dest_id + r.guard->toString() + r.updateToString() +
             r.sync;
}

automaton::automaton(::std::vector<State> arg_states,
                     ::std::vector<Transition> arg_transitions,
                     ::std::string arg_prefix, bool setTrap) {
  states = arg_states;
  if (setTrap == true && states.end() == find_if(states.begin(), states.end(),
                                                 [](const State &s) -> bool {
                                                   return s.id == "trap";
                                                 })) {
    TrueCC true_cc = TrueCC();
    states.push_back(State("trap", true_cc));
  }
  transitions = arg_transitions;
  prefix = arg_prefix;
}

tlEntry::tlEntry(Automaton &arg_ta, ::std::vector<Transition> arg_trans_out)
    : ta(arg_ta), trans_out(arg_trans_out) {}
