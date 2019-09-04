#include "constants.h"
#include "encoders.h"
#include "filter.h"
#include "printer.h"
#include "timed_automata.h"
#include "utap_trace_parser.h"
#include "vis_info.h"
#include <algorithm>
#include <assert.h>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <unordered_map>
#include <vector>

using namespace taptenc;
using namespace std;

void append_prefix_to_states(vector<State> &arg_states, string prefix) {
  for (auto it = arg_states.begin(); it != arg_states.end(); ++it) {
    it->name += prefix;
  }
}
Automaton generatePlanAutomaton(const vector<string> &arg_plan,
                                string arg_name) {
  vector<string> full_plan;
  transform(arg_plan.begin(), arg_plan.end(), back_inserter(full_plan),
            [](const string pa) -> string { return "alpha" + pa; });
  full_plan.push_back(constants::END_PA);
  full_plan.insert(full_plan.begin(), constants::START_PA);
  vector<State> plan_states;
  for (auto it = full_plan.begin(); it != full_plan.end(); ++it) {
    plan_states.push_back(
        State((*it == constants::END_PA || *it == constants::START_PA)
                  ? *it
                  : *it + constants::PA_SEP + to_string(it - full_plan.begin()),
              (*it == constants::END_PA || *it == constants::START_PA)
                  ? ""
                  : "cpa &lt; 60",
              false, (*it == constants::START_PA) ? true : false));
  }
  auto find_initial = find_if(plan_states.begin(), plan_states.end(),
                              [](const State &s) bool { return s.initial; });
  if (find_initial == plan_states.end()) {
    cout << "generatePlanAutomaton: no initial state found" << endl;
  } else {
    cout << "generatePlanAutomaton: initial state: " << find_initial->id
         << endl;
  }
  vector<Transition> plan_transitions;
  int i = 0;
  for (auto it = plan_states.begin() + 1; it < plan_states.end(); ++it) {
    string sync_op = "";
    string guard = "";
    string update = "";
    auto prev_state = (it - 1);
    if (prev_state->id != constants::END_PA &&
        prev_state->id != constants::START_PA) {
      guard = "cpa &gt; 30";
      update = "cpa = 0";
    }
    plan_transitions.push_back(Transition(prev_state->id, it->id, it->id, guard,
                                          update, sync_op, false));
    i++;
  }
  Automaton res = Automaton(plan_states, plan_transitions, arg_name, false);
  res.clocks.push_back("cpa");
  return res;
}

Automaton generateSyncPlanAutomaton(
    const vector<string> &arg_plan,
    const unordered_map<string, vector<string>> &arg_constraint_activations,
    string arg_name) {
  vector<pair<int, vector<string>>> syncs_to_add;
  vector<string> full_plan;
  transform(arg_plan.begin(), arg_plan.end(), back_inserter(full_plan),
            [](const string pa) -> string { return "alpha" + pa; });
  for (auto it = arg_plan.begin(); it != arg_plan.end(); ++it) {
    auto activate = arg_constraint_activations.find(*it);
    if (activate != arg_constraint_activations.end()) {
      syncs_to_add.push_back(
          make_pair(it - arg_plan.begin(), activate->second));
    }
  }
  int offset = 0;
  for (auto it = syncs_to_add.begin(); it != syncs_to_add.end(); ++it) {
    full_plan.insert(full_plan.begin() + offset + it->first, it->second.begin(),
                     it->second.end());
    offset += it->second.size();
  }
  full_plan.push_back(constants::END_PA);
  full_plan.insert(full_plan.begin(), constants::START_PA);
  vector<State> plan_states;
  for (auto it = full_plan.begin(); it != full_plan.end(); ++it) {
    bool urgent = false;
    if (it->substr(0, 5) != "alpha") {
      urgent = true;
    }
    plan_states.push_back(
        State(*it + constants::PA_SEP + to_string(it - full_plan.begin()),
              ((*it == constants::END_PA || *it == constants::START_PA)
                   ? ""
                   : "cpa &lt; 60"),
              urgent));
  }
  vector<Transition> plan_transitions;
  int i = 0;
  for (auto it = plan_states.begin() + 1; it < plan_states.end(); ++it) {
    string sync_op = "";
    string guard = "";
    string update = "";
    auto prev_state = (it - 1);
    if (prev_state->name.substr(0, 5) != "alpha") {
      sync_op = Filter::getPrefix(prev_state->name, constants::PA_SEP);
    } else {
      guard = "cpa &gt; 30";
      update = "cpa = 0";
    }
    plan_transitions.push_back(Transition(prev_state->id, it->id, it->id, guard,
                                          update, sync_op, false));
    i++;
  }
  Automaton res = Automaton(plan_states, plan_transitions, arg_name, false);
  res.clocks.push_back("cpa");
  return res;
}

DirectEncoder createDirectEncoding(
    AutomataSystem &direct_system,
    const unordered_map<string, vector<string>> &activations,
    unordered_map<string, pair<vector<State>, Bounds>> &targets,
    int plan_index = 1) {
  DirectEncoder enc(direct_system);
  for (const auto &pa : direct_system.instances[plan_index].first.states) {
    string pa_op = Filter::getPrefix(pa.id, constants::PA_SEP);
    if (pa_op.substr(5) == "start" || pa_op.substr(5) == "fin") {
      continue;
    }
    string pa_id = pa_op.substr(5);
    auto search = activations.find(pa_id);
    if (search != activations.end()) {
      for (const auto &ac : search->second) {
        auto search = targets.find(ac);
        if (search != targets.end()) {
          if (ac.substr(0, 6) == "vision") {
            enc.encodeFuture(direct_system, search->second.first, pa.id,
                             "vision", search->second.second, 0);
          }
          if (ac.substr(0, 7) == "cam_off") {
            enc.encodeNoOp(direct_system, search->second.first, pa.id);
          }
          if (ac.substr(0, 10) == "puck_sense") {
            enc.encodeFuture(direct_system, search->second.first, pa.id,
                             "puck_sense", search->second.second, 0);
          }
        }
      }
    }
  }
  return enc;
}

ModularEncoder
createModularEncoding(AutomataSystem &system, const AutomataGlobals g,
                      unordered_map<string, vector<State>> &targets, Bounds b) {
  ModularEncoder enc(system);
  for (const auto chan : g.channels) {
    auto search = targets.find(chan.name);
    if (search != targets.end()) {
      if (chan.name.substr(0, 5) == "snoop") {
        enc.encodeNoOp(system, search->second, chan.name);
      }
      if (chan.name.substr(0, 5) == "spast") {
        enc.encodePast(system, search->second, chan.name, b);
      }
      if (chan.name.substr(0, 8) == "sfinally") {
        enc.encodeFuture(system, search->second, chan.name, b);
      }
    }
  }
  return enc;
}

CompactEncoder
createCompactEncoding(AutomataSystem &system, const AutomataGlobals g,
                      unordered_map<string, vector<State>> &targets, Bounds b) {
  CompactEncoder enc;
  for (const auto chan : g.channels) {
    auto search = targets.find(chan.name);
    if (search != targets.end()) {
      if (chan.name.substr(0, 5) == "snoop") {
        enc.encodeNoOp(system, search->second, chan.name);
      }
      if (chan.name.substr(0, 5) == "spast") {
        enc.encodePast(system, search->second, chan.name, b);
      }
      if (chan.name.substr(0, 8) == "sfinally") {
        enc.encodeFuture(system, search->second, chan.name, b);
      }
    }
  }
  return enc;
}

std::string trim(const std::string &str,
                 const std::string &whitespace = " \t") {
  const auto strBegin = str.find_first_not_of(whitespace);
  if (strBegin == std::string::npos)
    return ""; // no content

  const auto strEnd = str.find_last_not_of(whitespace);
  const auto strRange = strEnd - strBegin + 1;

  return str.substr(strBegin, strRange);
}

void deleteEmptyLines(const std::string &filePath) {
  std::string bufferString = "";

  // file
  std::fstream fileStream;
  std::string currentReadLine;
  fileStream.open(filePath, std::fstream::in); // open the file in Input mode

  // Read all the lines till the end of the file
  while (getline(fileStream, currentReadLine)) {
    // Check if the line is empty
    if (!currentReadLine.empty()) {
      currentReadLine = trim(currentReadLine);
      bufferString = bufferString + currentReadLine + "\n";
    }
  }
  fileStream.close();
  fileStream.open(filePath,
                  std::ios::out |
                      std::ios::trunc); // open file in Output mode. This line
                                        // will delete all data inside the file.
  fileStream << bufferString;
  fileStream.close();
}

void replaceStringInPlace(std::string &subject, const std::string &search,
                          const std::string &replace) {
  size_t pos = 0;
  while ((pos = subject.find(search, pos)) != std::string::npos) {
    subject.replace(pos, search.length(), replace);
    pos += replace.length();
  }
}

string convertCharsToHTML(std::string str) {
  replaceStringInPlace(str, "&", "&amp;");
  replaceStringInPlace(str, "<", "&lt;");
  replaceStringInPlace(str, ">", "&gt;");
  return str;
}

std::string getEnvVar(std::string const &key) {
  char *val = std::getenv(key.c_str());
  if (val == NULL) {
    cout << "Environment Variable " << key << " not set!" << endl;
    return "";
  }
  return std::string(val);
}

int main() {
  vector<State> states;
  vector<Transition> transitions;
  vector<string> whatever;
  states.push_back(State("idle", "", false, true));
  states.push_back(State("cam_boot", "cam &lt; 5"));
  states.push_back(State("cam_on", ""));
  states.push_back(State("save_pic", "pic &lt;= 2"));
  states.push_back(State("icp_start", "icp &lt;= 10"));
  states.push_back(State("icp_end", ""));
  states.push_back(State("puck_sense", "sense &lt; 5"));

  transitions.push_back(
      Transition("idle", "cam_boot", "power_on_cam", "", "cam = 0", "", true));
  transitions.push_back(
      Transition("cam_boot", "cam_on", "", "cam &gt; 2", "cam = 0", "", true));
  transitions.push_back(
      Transition("cam_on", "save_pic", "store_pic", "", "pic = 0", "", true));
  transitions.push_back(
      Transition("save_pic", "cam_on", "", "pic &gt; 1", "", "", true));
  transitions.push_back(
      Transition("cam_on", "icp_start", "start_icp", "", "icp = 0", "", true));
  transitions.push_back(Transition("icp_start", "icp_end", "",
                                   "icp &lt; 10 &amp;&amp; icp &gt; 5", "", "",
                                   true));
  transitions.push_back(Transition("icp_end", "cam_on", "", "", "", "", true));
  transitions.push_back(
      Transition("cam_on", "idle", "power_off_cam", "", "cam = 0", "", true));
  transitions.push_back(Transition("idle", "puck_sense", "check_puck",
                                   "cam &gt; 2", "sense = 0", "", true));
  transitions.push_back(
      Transition("puck_sense", "idle", "", "sense &gt; 1", "", "", true));
  Automaton test(states, transitions, "main", false);
  test.clocks.insert(test.clocks.end(),
                     {"icp", "cam", "pic", "sense", "globtime"});
  vector<State> vision_filter;
  vector<State> cam_off_filter;
  vector<State> puck_sense_filter;
  ;
  vision_filter.insert(vision_filter.end(), {test.states[5]});
  cam_off_filter.insert(cam_off_filter.end(), {test.states[0], test.states[6]});
  puck_sense_filter.insert(puck_sense_filter.end(), {test.states[6]});
  Bounds vision_bounds(0, numeric_limits<int>::max());
  Bounds cam_off_bounds(0, numeric_limits<int>::max());
  Bounds puck_sense_bounds(0, 15, "&lt;=", "&lt;");
  AutomataGlobals glob;
  XMLPrinter printer;
  vector<pair<Automaton, string>> automata_direct;
  automata_direct.push_back(make_pair(test, ""));
  vector<string> plan{"goto", "pick",
                      "goto"}; //, "put", "goto", "pick", "discard", "goto",
                               //"pick", "goto", "put"};
  unordered_map<string, vector<string>> activations;
  activations.insert(make_pair("goto", vector<string>{"cam_off"}));
  activations.insert(make_pair("pick", vector<string>{"vision", "puck_sense"}));
  activations.insert(make_pair("put", vector<string>{"vision", ""}));
  activations.insert(
      make_pair("discard", vector<string>{"cam_off", "puck_sense"}));
  unordered_map<string, pair<vector<State>, Bounds>> targets;
  targets.insert(
      make_pair("cam_off", make_pair(cam_off_filter, cam_off_bounds)));
  targets.insert(make_pair("vision", make_pair(vision_filter, vision_bounds)));
  targets.insert(
      make_pair("puck_sense", make_pair(puck_sense_filter, puck_sense_bounds)));
  Automaton plan_ta = generatePlanAutomaton(plan, "plan");
  automata_direct.push_back(make_pair(plan_ta, ""));
  AutomataSystem direct_system;
  direct_system.instances = automata_direct;
  DirectEncoder enc1 =
      createDirectEncoding(direct_system, activations, targets);
  enc1.createFinalSystem(direct_system);
  auto internal = enc1.getInternalTAs();
  SystemVisInfo direct_system_vis_info(internal, plan_ta.states);
  printer.print(direct_system, direct_system_vis_info, "perception_direct.xml");
  std::ofstream myfile;
  myfile.open("perception_direct.q", std::ios_base::trunc);
  myfile << "E<> sys_direct.query";
  myfile.close();
  string call_get_if =
      "UPPAAL_COMPILE_ONLY=1 " + getEnvVar("VERIFYTA_DIR") +
      "/verifyta perception_direct.xml - > perception_direct.if";
  std::system(call_get_if.c_str());
  string call_get_trace = getEnvVar("VERIFYTA_DIR") +
                          "/verifyta -t 1 -f perception_direct "
                          "perception_direct.xml perception_direct.q";
  std::system(call_get_trace.c_str());
  deleteEmptyLines("perception_direct-1.xtr");
  string call_make_trace_readable =
      "tracer perception_direct.if perception_direct-1.xtr > "
      "perception_direct.trace";
  std::system(call_make_trace_readable.c_str());
  UTAPTraceParser::parseTraceInfo("perception_direct.trace", test, plan_ta);
  return 0;
}
