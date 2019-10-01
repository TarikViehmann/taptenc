#include "constants.h"
#include "encoders.h"
#include "filter.h"
#include "plan_ordered_tls.h"
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
#include <memory>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <unordered_map>
#include <unordered_set>
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
    const unordered_map<string, vector<unique_ptr<EncICInfo>>> &activations,
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
        switch (ac->type) {
        case ICType::Future: {
          UnaryInfo *info = dynamic_cast<UnaryInfo *>(ac.get());
          enc.encodeFuture(direct_system, pa.id, *info);
        } break;
        case ICType::Until: {
          BinaryInfo *info = dynamic_cast<BinaryInfo *>(ac.get());
          enc.encodeUntil(direct_system, pa.id, *info);
        } break;
        case ICType::Since: {
          BinaryInfo *info = dynamic_cast<BinaryInfo *>(ac.get());
          enc.encodeSince(direct_system, pa.id, *info);
        } break;
        case ICType::Past: {
          UnaryInfo *info = dynamic_cast<UnaryInfo *>(ac.get());
          enc.encodePast(direct_system, pa.id, *info);
        } break;
        case ICType::NoOp: {
          UnaryInfo *info = dynamic_cast<UnaryInfo *>(ac.get());
          enc.encodeNoOp(direct_system, info->specs.targets, pa.id);
        } break;
        case ICType::Invariant: {
          UnaryInfo *info = dynamic_cast<UnaryInfo *>(ac.get());
          enc.encodeInvariant(direct_system, info->specs.targets, pa.id);
        } break;
        default:
          cout << "error: no support yet for type " << ac->type << endl;
        }
      }
    }
  }
  return enc;
}

void testUntilChain(DirectEncoder &enc, AutomataSystem &direct_system,
                    ChainInfo &info, string start_pa, string end_pa) {
  for (auto pa = direct_system.instances[1].first.states.begin();
       pa != direct_system.instances[1].first.states.end(); ++pa) {
    string pa_op = pa->id;
    if (pa->id != constants::START_PA && pa->id != constants::END_PA) {
      pa_op = Filter::getPrefix(pa->id, constants::PA_SEP);
    }
    if (start_pa == pa_op) {
      for (auto epa = pa; epa != direct_system.instances[1].first.states.end();
           ++epa) {
        string epa_op = epa->id;
        if (epa->id != constants::START_PA && epa->id != constants::END_PA) {
          epa_op = Filter::getPrefix(epa->id, constants::PA_SEP);
        }
        if (end_pa == epa_op) {
          std::cout << "encode Until" << std::endl;
          enc.encodeUntilChain(direct_system, info, pa->id, epa->id);
          break;
        }
      }
    }
  }
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
void solve(std::string file_name, Automaton &base_ta, Automaton &plan_ta) {
  std::ofstream myfile;
  myfile.open(file_name + ".q", std::ios_base::trunc);
  myfile << "E<> sys_direct." << constants::QUERY;
  myfile.close();
  string call_get_if = "UPPAAL_COMPILE_ONLY=1 " + getEnvVar("VERIFYTA_DIR") +
                       "/verifyta " + file_name + ".xml - > " + file_name +
                       ".if";
  std::system(call_get_if.c_str());
  string call_get_trace = getEnvVar("VERIFYTA_DIR") + "/verifyta -t 1 -f " +
                          file_name + " " + file_name + ".xml " + file_name +
                          ".q";
  std::system(call_get_trace.c_str());
  deleteEmptyLines(file_name + "-1.xtr");
  string call_make_trace_readable = "tracer " + file_name + ".if " + file_name +
                                    "-1.xtr > " + file_name + ".trace";
  std::system(call_make_trace_readable.c_str());
  UTAPTraceParser::parseTraceInfo(file_name + ".trace", base_ta, plan_ta);
}

int main() {
  if (getEnvVar("VERIFYTA_DIR") == "") {
    cout << "ERROR: VERIFYTA_DIR not set!" << endl;
    return -1;
  }
  vector<State> states;
  vector<Transition> transitions;
  states.push_back(State("idle", "", false, true));
  states.push_back(State("cam_boot", "cam &lt; 5"));
  states.push_back(State("cam_on", ""));
  states.push_back(State("save_pic", "pic &lt;= 2"));
  states.push_back(State("icp_start", "icp &lt;= 10"));
  states.push_back(State("icp_end", "icp &lt;= 3"));
  states.push_back(State("puck_sense", "sense &lt; 5"));

  vector<State> comm_states;
  vector<Transition> comm_transitions;
  comm_states.push_back(State("idle", "", false, true));
  comm_states.push_back(State("prepare", "send &lt; 30"));
  comm_states.push_back(State("prepared", "send &lt;= 0"));
  comm_states.push_back(State("error", "send &lt;= 0"));
  comm_transitions.push_back(
      Transition("idle", "prepare", "send_prepare", "", "send = 0", "", true));
  comm_transitions.push_back(Transition("prepare", "prepared", "",
                                        "send &lt; 30 &amp;&amp; send &gt; 10",
                                        "send = 0", "", true));
  comm_transitions.push_back(
      Transition("prepare", "error", "", "send == 30", "send = 0", "", true));
  comm_transitions.push_back(
      Transition("error", "idle", "", "send == 30", "", "", true));
  comm_transitions.push_back(
      Transition("prepared", "idle", "", "", "", "", true));
  addStateClock(comm_transitions);
  Automaton comm_ta =
      Automaton(comm_states, comm_transitions, "comm_ta", false);
  comm_ta.clocks.insert(comm_ta.clocks.end(),
                        {"send", "globtime", constants::STATE_CLOCK});
  vector<State> comm_idle_filter;
  vector<State> comm_preparing_filter;
  vector<State> comm_prepared_filter;
  comm_idle_filter.push_back(comm_ta.states[0]);
  comm_prepared_filter.push_back(comm_ta.states[2]);
  comm_preparing_filter.push_back(comm_ta.states[1]);

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
                                   "icp &lt; 10 &amp;&amp; icp &gt; 5",
                                   "icp = 0", "", true));
  transitions.push_back(
      Transition("icp_end", "cam_on", "icp &gt; 1", "", "", "", true));
  transitions.push_back(
      Transition("cam_on", "idle", "power_off_cam", "", "cam = 0", "", true));
  transitions.push_back(Transition("idle", "puck_sense", "check_puck",
                                   "cam &gt; 2", "sense = 0", "", true));
  transitions.push_back(
      Transition("puck_sense", "idle", "", "sense &gt; 1", "", "", true));
  addStateClock(transitions);
  Automaton test(states, transitions, "main", false);

  test.clocks.insert(test.clocks.end(), {"icp", "cam", "pic", "sense",
                                         "globtime", constants::STATE_CLOCK});
  vector<State> pic_filter;
  vector<State> vision_filter;
  vector<State> no_vision_filter;
  vector<State> cam_off_filter;
  vector<State> cam_on_filter;
  vector<State> puck_sense_filter;
  pic_filter.insert(pic_filter.end(), {test.states[3]});
  vision_filter.insert(vision_filter.end(), {test.states[5]});
  no_vision_filter.insert(no_vision_filter.end(),
                          {test.states[0], test.states[1], test.states[2],
                           test.states[3], test.states[6]});
  cam_off_filter.insert(cam_off_filter.end(), {test.states[0], test.states[6]});
  cam_on_filter.insert(cam_on_filter.end(),
                       {test.states[1], test.states[2], test.states[3],
                        test.states[4], test.states[5]});
  puck_sense_filter.insert(puck_sense_filter.end(),
                           {test.states[6], test.states[5]});
  Bounds full_bounds(0, numeric_limits<int>::max());
  Bounds no_bounds(0, numeric_limits<int>::max());
  Bounds vision_bounds(5, 10); // numeric_limits<int>::max());
  Bounds cam_off_bounds(2, 5);
  Bounds puck_sense_bounds(2, 5, "&lt;=", "&lt;=");
  Bounds goto_bounds(15, 45);
  Bounds pick_bounds(13, 18);
  Bounds discard_bounds(3, 6);
  Bounds end_bounds(0, 5);
  AutomataGlobals glob;
  XMLPrinter printer;
  vector<pair<Automaton, string>> automata_direct;
  automata_direct.push_back(make_pair(test, ""));
  vector<PlanAction> plan{
      // goto CS-I
      PlanAction("goto", goto_bounds), PlanAction("endgoto", end_bounds),
      // pick from shelf
      PlanAction("pick", pick_bounds), PlanAction("endpick", end_bounds),
      // put on belt
      PlanAction("put", pick_bounds), PlanAction("endput", end_bounds),
      // goto CS-O
      PlanAction("goto", goto_bounds), PlanAction("endgoto", end_bounds),
      // pick waste
      PlanAction("pick", pick_bounds), PlanAction("endpick", end_bounds),
      // discard
      PlanAction("discard", discard_bounds),
      PlanAction("enddiscard", end_bounds),
      //      // goto BS
      PlanAction("goto", goto_bounds), PlanAction("endgoto", end_bounds),
      // pick base
      PlanAction("pick", pick_bounds), PlanAction("endpick", end_bounds),
      // goto RS-I
      PlanAction("goto", goto_bounds), PlanAction("endgoto", end_bounds),
      // put on belt
      PlanAction("put", pick_bounds), PlanAction("endput", end_bounds),
      // goto RS-O
      PlanAction("goto", goto_bounds), PlanAction("endgoto", end_bounds),
      // pick base + ring 1
      PlanAction("pick", pick_bounds), PlanAction("endpick", end_bounds),
      // goto RS-I
      PlanAction("goto", goto_bounds), PlanAction("endgoto", end_bounds),
      // put on belt
      PlanAction("put", pick_bounds), PlanAction("endput", end_bounds),
      // goto RS-O
      PlanAction("goto", goto_bounds), PlanAction("endgoto", end_bounds),
      // pick base + ring 1 + ring 2
      PlanAction("pick", pick_bounds), PlanAction("endpick", end_bounds),
      // goto RS-I
      PlanAction("goto", goto_bounds), PlanAction("endgoto", end_bounds),
      // put on belt
      PlanAction("put", pick_bounds), PlanAction("endput", end_bounds),
      // goto RS-O
      PlanAction("goto", goto_bounds), PlanAction("endgoto", end_bounds),
      // pick base + ring 1 + ring 2 + ring 3
      PlanAction("pick", pick_bounds), PlanAction("endpick", end_bounds),
      // goto CS-I
      PlanAction("goto", goto_bounds), PlanAction("endgoto", end_bounds),
      // put on belt
      PlanAction("put", pick_bounds), PlanAction("endput", end_bounds),
      // goto CS-O
      PlanAction("goto", goto_bounds), PlanAction("endgoto", end_bounds),
      // pick C§
      PlanAction("pick", pick_bounds), PlanAction("endpick", end_bounds),
      // goto DS
      PlanAction("goto", goto_bounds), PlanAction("endgoto", end_bounds),
      // put on belt
      PlanAction("put", pick_bounds), PlanAction("endput", end_bounds),
      PlanAction("last", end_bounds)}; //,
  unordered_set<string> pa_names;
  transform(plan.begin(), plan.end(),
            insert_iterator<unordered_set<string>>(pa_names, pa_names.begin()),
            [](const PlanAction &pa) -> string { return pa.name; });
  unordered_set<string> cam_off_execptions{"pick", "put", "end_pick",
                                           "end_put"};
  // ---------------------- Normal Constraints ------------------------------
  // cam off by default
  unordered_map<string, vector<unique_ptr<EncICInfo>>> activations;
  for (string pa : pa_names) {
    if (cam_off_execptions.find(pa) == cam_off_execptions.end()) {
      activations[pa].emplace_back(make_unique<UnaryInfo>(
          "coff", ICType::Invariant, TargetSpecs(end_bounds, cam_off_filter)));
    }
  }
  // puck sense after pick and put
  activations["endpick"].emplace_back(make_unique<UnaryInfo>(
      "sense1", ICType::Future,
      TargetSpecs(puck_sense_bounds, puck_sense_filter)));
  activations["endput"].emplace_back(make_unique<UnaryInfo>(
      "sense2", ICType::Future,
      TargetSpecs(puck_sense_bounds, puck_sense_filter)));

  // ---------------------- Until Chain -------------------------------------
  // until chain to do icp
  ChainInfo icp_chain(
      "icp_chain", ICType::UntilChain,
      // start with a booted cam
      vector<TargetSpecs>{TargetSpecs(vision_bounds, cam_on_filter),
                          // be done with icp after vision_bounds
                          TargetSpecs(full_bounds, vision_filter),
                          // do not do ICP again until pick action is done
                          TargetSpecs(full_bounds, no_vision_filter)});
  // until chain to do icp
  ChainInfo pic_chain("pic_chain", ICType::UntilChain,
                      // start however
                      vector<TargetSpecs>{TargetSpecs(full_bounds, states),
                                          // after unspecified time, save a pic
                                          TargetSpecs(full_bounds, pic_filter),
                                          // do whatever until end
                                          TargetSpecs(full_bounds, states)});
  AutomataSystem base_system;
  base_system.instances = automata_direct;
  DirectEncoder enc1 = createDirectEncoding(base_system, plan, activations);
  testUntilChain(enc1, base_system, icp_chain, "pick", "endpick");
  testUntilChain(enc1, base_system, pic_chain, "pick", "endpick");

  unordered_map<string, vector<unique_ptr<EncICInfo>>> comm_activations;
  AutomataSystem comm_system;
  comm_system.instances.push_back(make_pair(comm_ta, ""));
  DirectEncoder enc2 =
      createDirectEncoding(comm_system, plan, comm_activations);
  // until chain to prepare
  ChainInfo prepare_chain(
      "prepare_chain", ICType::UntilChain,
      vector<TargetSpecs>{TargetSpecs(no_bounds, comm_idle_filter),
                          TargetSpecs(no_bounds, comm_preparing_filter),
                          TargetSpecs(no_bounds, comm_prepared_filter),
                          TargetSpecs(no_bounds, comm_idle_filter)});
  ChainInfo idle_chain(
      "idle_chain", ICType::UntilChain,
      vector<TargetSpecs>{TargetSpecs(no_bounds, comm_idle_filter)});
  ChainInfo idle_chain2(
      "start_chain", ICType::UntilChain,
      vector<TargetSpecs>{TargetSpecs(no_bounds, comm_idle_filter)});
  ChainInfo idle_chain3(
      "end_chain", ICType::UntilChain,
      vector<TargetSpecs>{TargetSpecs(no_bounds, comm_idle_filter)});
  testUntilChain(enc2, comm_system, prepare_chain, "endput", "pick");
  testUntilChain(enc2, comm_system, idle_chain, "endpick", "put");
  testUntilChain(enc2, comm_system, idle_chain2, constants::START_PA, "pick");
  testUntilChain(enc2, comm_system, idle_chain3, "last", constants::END_PA);

  AutomataSystem merged_system;
  merged_system.globals.clocks.insert(merged_system.globals.clocks.end(),
                                      comm_system.globals.clocks.begin(),
                                      comm_system.globals.clocks.end());
  merged_system.globals.clocks.insert(merged_system.globals.clocks.end(),
                                      base_system.globals.clocks.begin(),
                                      base_system.globals.clocks.end());
  merged_system.instances = base_system.instances;
  SystemVisInfo comm_system_vis_info;
  DirectEncoder enc3 = enc1.mergeEncodings(enc2);
  AutomataSystem direct_system2 =
      enc2.createFinalSystem(comm_system, comm_system_vis_info);
  printer.print(direct_system2, comm_system_vis_info, "comm_direct.xml");
  SystemVisInfo direct_system_vis_info;
  AutomataSystem direct_system =
      enc1.createFinalSystem(base_system, direct_system_vis_info);
  printer.print(direct_system, direct_system_vis_info, "perception_direct.xml");
  SystemVisInfo merged_system_vis_info;
  AutomataSystem direct_system3 =
      enc3.createFinalSystem(merged_system, merged_system_vis_info);
  printer.print(direct_system3, merged_system_vis_info, "merged_direct.xml");
  Automaton product_ta = PlanOrderedTLs::productTA(test, comm_ta, "product");
  solve("merged_direct", product_ta,
        base_system.instances[enc3.getPlanTAIndex()].first);

  return 0;
}
