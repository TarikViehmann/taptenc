#pragma once
#include "timed-automata/timed_automata.h"
#include <string>

namespace taptenc {
namespace uppaalcalls {
constexpr char QUERY_STR[]{"E<> sys_direct.AqueryA"};
void deleteEmptyLines(const std::string &filePath);
std::string getEnvVar(std::string const &key);
void solve(::std::string file_name = "taptenc_temp.xml",
           ::std::string query_str = QUERY_STR);
void solve(const AutomataSystem &sys,
           ::std::string file_name = "taptenc_temp.xml",
           ::std::string query_str = QUERY_STR);
} // end namespace uppaalcalls
} // end namespace taptenc
