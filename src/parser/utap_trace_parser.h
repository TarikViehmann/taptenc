#include "timed_automata.h"
#include <string>
#include <unordered_map>

namespace taptenc {
namespace UTAPTraceParser {
struct specialClocksInfo {
  ::std::pair<int, int> global_clock;
  ::std::pair<int, int> state_clock;
};
typedef specialClocksInfo SpecialClocksInfo;
// type for weight/distance on each edge
SpecialClocksInfo determineSpecialClockBounds(
    ::std::unordered_map<::std::pair<::std::string, ::std::string>, int>
        differences);

void replaceStringInPlace(::std::string &subject, const ::std::string &search,
                          const ::std::string &replace);

::std::string convertCharsToHTML(::std::string str);
int parseState(::std::string &currentReadLine);

::std::vector<::std::string> parseTransition(::std::string &currentReadLine,
                                             const Automaton &base_ta,
                                             const Automaton &plan_ta);
void parseTraceInfo(const ::std::string &file, const Automaton &base_ta,
                    const Automaton &plan_ta);
} // end namespace UTAPTraceParser
} // end namespace taptenc
