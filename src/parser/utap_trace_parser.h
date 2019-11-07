/** \file
 * Parser for uppaal symbolic traces (.trace files).
 *
 * \author (2019) Tarik Viehmann
 */
#include "../constraints/constraints.h"
#include "../timed-automata/timed_automata.h"
#include <string>
#include <unordered_map>

namespace taptenc {
/**
 * Wrapper for bounds on global time clock.
 */
struct specialClocksInfo {
  ::std::pair<timepoint, timepoint> global_clock;
};
typedef specialClocksInfo SpecialClocksInfo;
class UTAPTraceParser {

public:
  UTAPTraceParser(const AutomataSystem &s);
  /**
   * Parses a .trace file (output of uppaal) and print the fastest resulting
   * trace.
   *
   * @param file name of the file containing the trace
   * @param base_ta Platform model TA
   * @param plan_ta plan TA
   */
  ::std::vector<::std::pair<timepoint, ::std::vector<::std::string>>>
  parseTraceInfo(const ::std::string &file, const Automaton &base_ta,
                 const Automaton &plan_ta);

private:
  bool parsed = false;
  Automaton trace_ta;
  std::unordered_map<std::string, std::string> trace_to_ta_ids;
  std::vector<State> source_states;

  /**
   * Adds a fresh state to the trace TA.
   *
   * @param state_id state id from the encoding automaton to add
   * @return state id of the added state
   */
  ::std::string addStateToTraceTA(::std::string state_id);

  /**
   * Calculates bounds of the special clocks (for global time and state time)
   * from a DBM.
   *
   * TODO: also consider strict bounds
   *
   * @param differences DBM, where [clock1,clock2] -> i menas
   *                    clock1 -clock2 <= i
   * @return SpecialClockInfo holding bounds on global time and state time
   */
  SpecialClocksInfo determineSpecialClockBounds(
      ::std::unordered_map<::std::pair<::std::string, ::std::string>, int>
          differences);

  /**
   * Parses a line from a .trace file (output of uppaal) containing a
   * transition.
   *
   * @param currentReadLine line from a .trace file containing transition info
   * @param base_ta Platform model TA
   * @param plan_ta plan TA
   *
   * @return the action names associated with the parsed transition
   */
  ::std::vector<::std::string> parseTransition(std::string &currentReadLine,
                                               const Automaton &base_ta,
                                               const Automaton &plan_ta);
  /**
   * Parses a line from a .trace file (output of uppaal) containing a state.
   *
   * @param currentReadLine line from a .trace file containing state info
   * @return global time lower bound of the parsed state
   */
  int parseState(std::string &currentReadLine);
};
} // end namespace taptenc
