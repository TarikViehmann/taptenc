#include "constants.h"
#include "encoders.h"
#include "filter.h"
#include "plan_ordered_tls.h"
#include "platform_model_generator.h"
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
  for (auto pa = direct_system.instances[plan_index].first.states.begin();
       pa != direct_system.instances[plan_index].first.states.end(); ++pa) {
    string pa_op = pa->id;
    if (pa->id != constants::START_PA && pa->id != constants::END_PA) {
      pa_op = Filter::getPrefix(pa->id, constants::PA_SEP);
    }
    auto search = activations.find(pa_op);
    if (search != activations.end()) {
      for (const auto &ac : search->second) {
        switch (ac->type) {
        case ICType::Future: {
          std::cout << "start Future " << pa->id << std::endl;
          UnaryInfo *info = dynamic_cast<UnaryInfo *>(ac.get());
          enc.encodeFuture(direct_system, pa->id, *info);
        } break;
        case ICType::Until: {
          std::cout << "start Until " << pa->id << std::endl;
          BinaryInfo *info = dynamic_cast<BinaryInfo *>(ac.get());
          enc.encodeUntil(direct_system, pa->id, *info);
        } break;
        case ICType::Since: {
          std::cout << "start Since " << pa->id << std::endl;
          BinaryInfo *info = dynamic_cast<BinaryInfo *>(ac.get());
          enc.encodeSince(direct_system, pa->id, *info);
        } break;
        case ICType::Past: {
          std::cout << "start Past " << pa->id << std::endl;
          UnaryInfo *info = dynamic_cast<UnaryInfo *>(ac.get());
          enc.encodePast(direct_system, pa->id, *info);
        } break;
        case ICType::NoOp: {
          std::cout << "start NoOp " << pa->id << std::endl;
          UnaryInfo *info = dynamic_cast<UnaryInfo *>(ac.get());
          enc.encodeNoOp(direct_system, info->specs.targets, pa->id);
        } break;
        case ICType::Invariant: {
          std::cout << "start Invariant " << pa->id << std::endl;
          UnaryInfo *info = dynamic_cast<UnaryInfo *>(ac.get());
          enc.encodeInvariant(direct_system, info->specs.targets, pa->id);
        } break;
        case ICType::UntilChain: {
          ChainInfo *info = dynamic_cast<ChainInfo *>(ac.get());
          for (auto epa = pa;
               epa != direct_system.instances[plan_index].first.states.end();
               ++epa) {
            string epa_op = epa->id;
            if (epa->id != constants::START_PA &&
                epa->id != constants::END_PA) {
              epa_op = Filter::getPrefix(epa->id, constants::PA_SEP);
            }
            if (info->end_pa == epa_op) {
              std::cout << "start until chain " << pa->id << " " << epa->id
                        << std::endl;
              enc.encodeUntilChain(direct_system, *info, pa->id, epa->id);
              break;
            }
          }
        } break;
        default:
          cout << "error: no support yet for type " << ac->type << endl;
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

std::string getEnvVar(std::string const &key) {
  char *val = std::getenv(key.c_str());
  if (val == NULL) {
    cout << "Environment Variable " << key << " not set!" << endl;
    return "";
  }
  return std::string(val);
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
  Bounds full_bounds(0, numeric_limits<int>::max());
  Bounds no_bounds(0, numeric_limits<int>::max());
  Bounds goto_bounds(15, 45);
  Bounds pick_bounds(13, 18);
  Bounds discard_bounds(3, 6);
  Bounds end_bounds(0, 5);
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
  AutomataGlobals glob;
  XMLPrinter printer;
  // --------------- Perception ------------------------------------
  Automaton perception_ta = benchmarkgenerator::generatePerceptionTA();
  AutomataSystem base_system;
  base_system.instances.push_back(make_pair(perception_ta, ""));
  DirectEncoder enc1 =
      createDirectEncoding(base_system, plan,
                           benchmarkgenerator::generatePerceptionConstraints(
                               perception_ta, pa_names));
  // --------------- Communication ---------------------------------
  Automaton comm_ta = benchmarkgenerator::generateCommTA();
  AutomataSystem comm_system;
  comm_system.instances.push_back(make_pair(comm_ta, ""));
  DirectEncoder enc2 = createDirectEncoding(
      comm_system, plan, benchmarkgenerator::generateCommConstraints(comm_ta));
  // --------------- Calibration ----------------------------------
  Automaton calib_ta = benchmarkgenerator::generateCalibrationTA();
  for (const auto &s : calib_ta.states) {
    std::cout << s.id << std::endl;
  }
  AutomataSystem calib_system;
  calib_system.instances.push_back(make_pair(calib_ta, ""));
  DirectEncoder enc3 = createDirectEncoding(
      calib_system, plan,
      benchmarkgenerator::generateCalibrationConstraints(calib_ta));
  // --------------- Merge ----------------------------------------
  AutomataSystem merged_system;
  merged_system.globals.clocks.insert(merged_system.globals.clocks.end(),
                                      comm_system.globals.clocks.begin(),
                                      comm_system.globals.clocks.end());
  merged_system.globals.clocks.insert(merged_system.globals.clocks.end(),
                                      base_system.globals.clocks.begin(),
                                      base_system.globals.clocks.end());
  merged_system.globals.clocks.insert(merged_system.globals.clocks.end(),
                                      calib_system.globals.clocks.begin(),
                                      calib_system.globals.clocks.end());
  merged_system.instances = base_system.instances;
  DirectEncoder enc4 = enc1.mergeEncodings(enc2);
  enc4 = enc4.mergeEncodings(enc3);
  // --------------- Print XMLs -----------------------------------
  SystemVisInfo direct_system_vis_info;
  AutomataSystem direct_system =
      enc1.createFinalSystem(base_system, direct_system_vis_info);
  printer.print(direct_system, direct_system_vis_info, "perception_direct.xml");
  SystemVisInfo comm_system_vis_info;
  AutomataSystem direct_system2 =
      enc2.createFinalSystem(comm_system, comm_system_vis_info);
  printer.print(direct_system2, comm_system_vis_info, "comm_direct.xml");
  SystemVisInfo calib_system_vis_info;
  AutomataSystem direct_system3 =
      enc3.createFinalSystem(calib_system, calib_system_vis_info);
  printer.print(direct_system3, calib_system_vis_info, "calib_direct.xml");
  SystemVisInfo merged_system_vis_info;
  AutomataSystem direct_system4 =
      enc4.createFinalSystem(merged_system, merged_system_vis_info);
  printer.print(direct_system4, merged_system_vis_info, "merged_direct.xml");
  Automaton product_ta =
      PlanOrderedTLs::productTA(perception_ta, comm_ta, "product");
  product_ta = PlanOrderedTLs::productTA(product_ta, calib_ta, "product");
  solve("merged_direct", product_ta,
        base_system.instances[enc3.getPlanTAIndex()].first);

  return 0;
}
