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
    AutomataSystem &direct_system, const vector<PlanAction> plan,
    const unordered_map<string, vector<EncICInfo>> &activations,
    int plan_index = 1) {
  DirectEncoder enc(direct_system, plan);
  for (const auto &pa : direct_system.instances[plan_index].first.states) {
    string pa_op = Filter::getPrefix(pa.id, constants::PA_SEP);
    if (pa_op == constants::START_PA || pa_op == constants::END_PA) {
      continue;
    }
    auto search = activations.find(pa_op);
    if (search != activations.end()) {
      for (const auto &ac : search->second) {
        switch (ac.type) {
        case ICType::Future:
          enc.encodeFuture(direct_system, pa.id, ac);
          break;
        case ICType::Until:
          enc.encodeUntil(direct_system, pa.id, ac);
          break;
        case ICType::Past:
          enc.encodePast(direct_system, pa.id, ac);
          break;
        case ICType::NoOp:
          enc.encodeNoOp(direct_system, ac.targets, pa.id);
          break;
        case ICType::Invariant:
          enc.encodeInvariant(direct_system, ac.targets, pa.id);
          break;
        default:
          cout << "error: no support yet for type " << ac.type << endl;
        }
      }
    }
  }
  return enc;
}

DirectEncoder testUntilChain(AutomataSystem &direct_system,
                             const vector<PlanAction> plan,
                             vector<EncICInfo> &infos, string start_pa,
                             string end_pa) {
  DirectEncoder enc(direct_system, plan);
  for (auto pa = direct_system.instances[1].first.states.begin();
       pa != direct_system.instances[1].first.states.end(); ++pa) {
    string pa_op = Filter::getPrefix(pa->id, constants::PA_SEP);
    if (pa_op == constants::START_PA || pa_op == constants::END_PA) {
      continue;
    }
    if (start_pa == pa_op) {
      for (auto epa = pa; epa != direct_system.instances[1].first.states.end();
           ++epa) {
        string epa_op = Filter::getPrefix(epa->id, constants::PA_SEP);
        if (end_pa == epa_op) {
          enc.encodeUntilChain(direct_system, infos, pa->id, epa->id);
          break;
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
void addStateClock(vector<Transition> &trans) {
  for (auto &t : trans) {
    t.update = addUpdate(t.update, string(constants::STATE_CLOCK) + " = 0");
  }
}

int main() {
  if (getEnvVar("VERIFYTA_DIR") == "") {
    cout << "ERROR: VERIFYTA_DIR not set!" << endl;
    return -1;
  }
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
  addStateClock(transitions);
  Automaton test(states, transitions, "main", false);

  test.clocks.insert(test.clocks.end(),
                     {"icp", "cam", "pic", "sense", "globtime", "statetime",
                      constants::STATE_CLOCK});
  vector<State> vision_filter;
  vector<State> cam_off_filter;
  vector<State> puck_sense_filter;
  vision_filter.insert(vision_filter.end(), {test.states[5]});
  cam_off_filter.insert(cam_off_filter.end(), {test.states[0], test.states[6]});
  puck_sense_filter.insert(puck_sense_filter.end(), {test.states[6]});
  Bounds full_bounds(0, 10); // numeric_limits<int>::max());
  Bounds vision_bounds(20, numeric_limits<int>::max());
  Bounds cam_off_bounds(2, 5);
  Bounds puck_sense_bounds(0, 15, "&lt;=", "&lt;");
  Bounds goto_bounds(15, 45);
  Bounds pick_bounds(13, 18);
  Bounds end_bounds(0, 0);
  AutomataGlobals glob;
  XMLPrinter printer;
  vector<pair<Automaton, string>> automata_direct;
  automata_direct.push_back(make_pair(test, ""));
  vector<PlanAction> plan{
      PlanAction("goto", goto_bounds), PlanAction("endgoto", end_bounds),
      PlanAction("pick", pick_bounds), PlanAction("endpick", end_bounds),
      PlanAction("goto", goto_bounds), PlanAction("endgoto", end_bounds)};
  //, "put", "goto", "pick", "discard", "goto",
  //"pick", "goto", "put"};
  // unordered_map<string, vector<EncICInfo>> activations;
  // activations.insert(make_pair(
  //     "goto", vector<EncICInfo>{EncICInfo(cam_off_filter, "cam_off",
  //                                         cam_off_bounds,
  //                                         ICType::Invariant)}));
  // activations.insert(make_pair(
  //     "pick", vector<EncICInfo>{EncICInfo(vision_filter, "vision",
  //                                         vision_bounds, ICType::Future),
  //                               EncICInfo(puck_sense_filter, "puck_sense",
  //                                         puck_sense_bounds,
  //                                         ICType::Future)}));
  // activations.insert(make_pair(
  //     "put", vector<EncICInfo>{EncICInfo(vision_filter, "vision",
  //     vision_bounds,
  //                                        ICType::Future)}));
  // activations.insert(make_pair(
  //     "discard",
  //     vector<EncICInfo>{
  //         EncICInfo(cam_off_filter, "cam_off", cam_off_bounds, ICType::NoOp),
  //         EncICInfo(puck_sense_filter, "puck_sense", puck_sense_bounds,
  //                   ICType::Future)}));
  vector<EncICInfo> uc{
      EncICInfo(states, "full1", full_bounds, ICType::Future),
      EncICInfo(vision_filter, "vision", vision_bounds, ICType::Future),
      EncICInfo(states, "full2", cam_off_bounds, ICType::Future),
      EncICInfo(cam_off_filter, "cam_off", puck_sense_bounds, ICType::Future),
      EncICInfo(puck_sense_filter, "puck_sense", cam_off_bounds,
                ICType::Future)};
  AutomataSystem base_system;
  base_system.instances = automata_direct;
  // DirectEncoder enc1 = createDirectEncoding(base_system, plan, activations);
  DirectEncoder enc1 = testUntilChain(base_system, plan, uc, "pick", "endgoto");
  SystemVisInfo direct_system_vis_info;
  AutomataSystem direct_system =
      enc1.createFinalSystem(base_system, direct_system_vis_info);
  printer.print(direct_system, direct_system_vis_info, "perception_direct.xml");
  // std::ofstream myfile;
  // myfile.open("perception_direct.q", std::ios_base::trunc);
  // myfile << "E<> sys_direct.query";
  // myfile.close();
  // string call_get_if =
  //     "UPPAAL_COMPILE_ONLY=1 " + getEnvVar("VERIFYTA_DIR") +
  //     "/verifyta perception_direct.xml - > perception_direct.if";
  // std::system(call_get_if.c_str());
  // string call_get_trace = getEnvVar("VERIFYTA_DIR") +
  //                         "/verifyta -t 1 -f perception_direct "
  //                         "perception_direct.xml perception_direct.q";
  // std::system(call_get_trace.c_str());
  // deleteEmptyLines("perception_direct-1.xtr");
  // string call_make_trace_readable =
  //     "tracer perception_direct.if perception_direct-1.xtr > "
  //     "perception_direct.trace";
  // std::system(call_make_trace_readable.c_str());
  // UTAPTraceParser::parseTraceInfo(
  //     "perception_direct.trace", test,
  //     base_system.instances[enc1.getPlanTAIndex()].first);
  return 0;
}
