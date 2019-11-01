/** \file
 * Parser for uppaal symbolic traces (.trace files).
 *
 * \author (2019) Tarik Viehmann
 */
#include "../constraints/constraints.h"
#include "../timed-automata/timed_automata.h"
#include <string>
#include <unordered_map>

/** Type to specify action times. */
using timepoint = int;
namespace taptenc {
namespace UTAPTraceParser {
/**
 * Wrapper for bounds on global time and state time clocks.
 */
struct specialClocksInfo {
  ::std::pair<timepoint, timepoint> global_clock;
  ::std::pair<timepoint, timepoint> state_clock;
};
typedef specialClocksInfo SpecialClocksInfo;

/**
 * Calculate bounds of the special clocks (for global time and state time) from
 * a DBM.
 *
 * TODO: also consider strict bounds
 *
 * @param differences DBM, where [clock1,clock2] -> i menas clock1 -clock2 <= i
 * @return SpecialClockInfo holding bounds on global time and state time
 */
SpecialClocksInfo determineSpecialClockBounds(
    ::std::unordered_map<::std::pair<::std::string, ::std::string>, int>
        differences);

/**
 * Parse a .trace file (output of uppaal) and print the fastest resulting trace.
 *
 * @param file name of the file containing the trace
 * @param base_ta Platform model TA
 * @param plan_ta plan TA
 */
::std::vector<::std::pair<timepoint, ::std::vector<::std::string>>>
parseTraceInfo(const ::std::string &file, const Automaton &base_ta,
               const Automaton &plan_ta);
} // end namespace UTAPTraceParser
} // end namespace taptenc
