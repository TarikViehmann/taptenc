/** \file
 * Provides datastructures to represent timed automata systems and encodings.
 *
 *
 * \author: (2019) Tarik Viehmann
 */

#pragma once

#include "../constraints/constraints.h"
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace taptenc {
/**
 * @name Timed automata related declarations
 *  - State
 *  - Transition
 *  - Automaton
 */
///@{
struct state {
  ::std::string id;
  ::std::unique_ptr<ClockConstraint> inv;
  bool urgent;
  bool initial;
  state(::std::string arg_id, const ClockConstraint &inv,
        bool arg_urgent = false, bool arg_initial = false);
  ~state() = default;
  /** Copy constructor that clones the uniquely managed resources. */
  state(const state &other);
  /** Move constructor that steals the uniquely managed resources. */
  state(state &&other) noexcept;
  /** Copy assignment using the copy constructor. */
  state &operator=(const state &other);
  /** Move assignment swapping all member contents. */
  state &operator=(state &&other) noexcept;
  /** Ordering operator determined by comparing string representations. */
  bool operator<(const state &r) const;
};
typedef struct state State;

typedef ::std::set<::std::shared_ptr<Clock>> update_t;

struct transition {
  ::std::string source_id;
  ::std::string dest_id;
  ::std::string action;
  ::std::unique_ptr<ClockConstraint> guard;
  update_t update;
  ::std::string sync;
  bool passive; // true: receiver of sync (?), false: emmitter of sync (!)
  transition(::std::string arg_source_id, ::std::string arg_dest_id,
             std::string arg_action, const ClockConstraint &guard,
             const update_t &arg_update, ::std::string arg_sync,
             bool arg_passive = false);
  ~transition() = default;
  /** Copy constructor that clones the uniquely managed resources. */
  transition(const transition &other);
  /** Move constructor that steals the uniquely managed resources. */
  transition(transition &&other) noexcept;
  /** Copy assignment using the copy constructor. */
  transition &operator=(const transition &other);
  /** Move assignment swapping all member contents. */
  transition &operator=(transition &&other) noexcept;
  /** Composes a string containing all clock updates. */
  ::std::string updateToString() const;
  /**
   * Composes a clock set from an update string.
   *
   * @param update update string to parse
   * @param clocks all clocks that may be updated
   * @return update set with clocks from \a clocks that are contained in
   *          \a update
   */
  static update_t updateFromString(const ::std::string &update,
                                   const update_t &clocks);
  /** Ordering operator determined by comparing string representations. */
  bool operator<(const transition &r) const;
};
typedef struct transition Transition;

struct automaton {
  ::std::vector<State> states;
  ::std::vector<Transition> transitions;
  ::std::set<::std::shared_ptr<Clock>> clocks;
  ::std::vector<::std::string> bool_vars;
  ::std::string prefix;

  automaton(::std::vector<State> arg_states,
            ::std::vector<Transition> arg_transitions, ::std::string arg_prefix,
            bool setTrap = true);
};
typedef struct automaton Automaton;
///@}

enum ChanType { Binary = 1, Broadcast = 0 };

/**
 * @name TA system related declarations (structure based of uppaal systems)
 *  - ChanType
 *  - Channel
 *  - AutomataGlobals
 *  - AutomataSystem
 */
///@{
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
  ::std::set<::std::shared_ptr<Clock>> clocks;
  ::std::vector<::std::string> bool_vars;
  ::std::vector<Channel> channels;
};
typedef struct automataGlobals AutomataGlobals;
struct automataSystem {
  ::std::vector<::std::pair<Automaton, ::std::string>> instances;
  AutomataGlobals globals;
};
typedef struct automataSystem AutomataSystem;
///@}

/**
 * @name Encoding related declarations to store automata copies
 *  - TlEntry (a TA copy together with outgoing transitions)
 *  - TimeLine (a set of TlEntries that can be retrieved via keys)
 *  - TimeLines (a set with TimeLine elements)
 */
///@{
struct tlEntry {
  Automaton ta;
  ::std::vector<Transition> trans_out;
  tlEntry(Automaton &arg_ta, ::std::vector<Transition> arg_trans_out);
};
typedef struct tlEntry TlEntry;
typedef ::std::unordered_map<::std::string, TlEntry> TimeLine;
typedef ::std::unordered_map<::std::string, TimeLine> TimeLines;
///@}
} // end namespace taptenc
