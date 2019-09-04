#include "timed_automata.h"
#include <string>
#include <unordered_map>

namespace taptenc {
namespace UTAPTraceParser {
// type for weight/distance on each edge
::std::pair<int, int> determineGlobTime(
    ::std::unordered_map<::std::pair<::std::string, ::std::string>, int>
        differences);

void replaceStringInPlace(::std::string &subject, const ::std::string &search,
                          const ::std::string &replace);

::std::string convertCharsToHTML(::std::string str);
void parseState(::std::string &currentReadLine);

void parseTransition(::std::string &currentReadLine, const Automaton &base_ta,
                     const Automaton &plan_ta);
void parseTraceInfo(const ::std::string &file, const Automaton &base_ta,
                    const Automaton &plan_ta);
} // end namespace UTAPTraceParser
} // end namespace taptenc
