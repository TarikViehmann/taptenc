/** \file
 * Parser for uppaal symbolic traces (.trace files).
 *
 * \author (2019) Tarik Viehmann
 */
#pragma once

#include "../constraints/constraints.h"
#include "../timed-automata/timed_automata.h"
#include "../utils.h"
#include <string>
#include <unordered_map>

namespace taptenc {
/**
 * Constraint, where the first entry encodes the bound, the second entry
 * encodes the strictness (true = strict)
 */
typedef ::std::pair<taptenc::timepoint, bool> dbm_entry_t;

/**
 * Wrapper for bounds on global time clock.
 */
struct specialClocksInfo {
  ::std::pair<dbm_entry_t, dbm_entry_t> global_clock;
  dbm_entry_t max_delay;
};
typedef specialClocksInfo SpecialClocksInfo;

/**
 * Different bounds representation, where [clock1,clock2] -> (i,true) means
 * clock1 - clock2 <= i.
 */
typedef ::std::unordered_map<::std::pair<::std::string, ::std::string>,
                             dbm_entry_t>
    dbm_t;

/**
 * Stores a timed trace by holding time constraints and the actions that are
 * started after the time constraints are met.
 */
typedef ::std::vector<
    ::std::pair<SpecialClocksInfo, ::std::vector<::std::string>>>
    timed_trace_t;

class UTAPTraceParser {

public:
  /**
   * Creates a trace parser from an automata system.
   */
  UTAPTraceParser(const AutomataSystem &s);
  /**
   * Parses a .trace file (output of uppaal).
   *
   * @param file name of the file containing the trace
   * @return true iff parsing was successful
   */
  bool parseTraceInfo(const ::std::string &file);

  /**
   * Applies a delay to the concrete trace and calculates a new temporal trace
   * from it.
   *
   * @param delay_pos index of concrete state where the delay occured
   * @param delay delay duration (full duration of the visit in the state at
   *        position \a delay_pos)
   */
  timed_trace_t applyDelay(size_t delay_pos, timepoint delay);

  /**
   * Extracts the timed trace after a trace has been parsed.
   *
   * Needs to be called after parseTraceInfo().
   *
   * @return timed trace
   */
  timed_trace_t getTimedTrace(const Automaton &base_ta,
                              const Automaton &plan_ta);

private:
  bool parsed = false;
  Automaton trace_ta;
  std::unordered_map<std::string, std::string> trace_to_ta_ids;
  std::vector<State> source_states;
  std::unordered_map<::std::shared_ptr<Clock>, dbm_entry_t> curr_clock_values;
  std::unordered_map<std::string, dbm_t> ta_to_symbolic_state;
  timed_trace_t parsed_trace;

  /**
   * Calculates the fastest concrete trace timings from a parsed symbolic trace.
   *
   * @return Timings of the concrete trace ordered according to the symbolic
   *         state orderings.
   */
  ::std::vector<SpecialClocksInfo> getTraceTimings();
  /**
   * Retrieves all actions that are associated to a given trace transition.
   *
   * @param trans trace transition from trace_ta
   * @param base_ta platform TA that was used in the encoding
   * @param plan_ta plan automaton used in the encoding
   * @return all actions that are attached to the platform and plan TA
   *         transitions causing the trace transition
   */
  ::std::vector<::std::string>
  getActionsFromTraceTrans(const Transition &trans, const Automaton &base_ta,
                           const Automaton &plan_ta);
  /**
   * Adds a fresh state to the trace TA.
   *
   * @param state_id state id from the encoding automaton to add
   * @return state id of the added state
   */
  ::std::string addStateToTraceTA(::std::string state_id);

  /**
   * Calculates bounds of the special clock counting global time from a DBM.
   *
   * @param differences different bound matrix of a symbolic state
   * @return SpecialClockInfo holding bounds on global time and maximal delay
   *          possible in the symbolic state described by \a differences
   */
  SpecialClocksInfo determineSpecialClockBounds(dbm_t differences);

  /**
   * Parses a line from a .trace file (output of uppaal) containing a
   * transition.
   *
   * @param currentReadLine line from a .trace file containing transition info
   */
  void parseTransition(std::string &currentReadLine);
  /**
   * Parses a line from a .trace file (output of uppaal) containing a state.
   *
   * @param currentReadLine line from a .trace file containing state info
   */
  void parseState(std::string &currentReadLine);
};
} // end namespace taptenc
