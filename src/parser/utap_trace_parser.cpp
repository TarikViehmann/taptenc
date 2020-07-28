/** \file
 * Parser for uppaal symbolic traces (.trace files).
 *
 * \author (2019) Tarik Viehmann
 */
#include "utap_trace_parser.h"
#include "../constants.h"
#include "../constraints/constraints.h"
#include "../encoder/filter.h"
#include "../printer/printer.h"
#include "../timed-automata/timed_automata.h"
#include "../uppaal_calls.h"
#include "../utils.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <unordered_map>

using std::cout;
using std::endl;
using std::iostream;
using std::pair;
using std::string;
using std::unordered_map;
namespace taptenc {

std::ostream& operator<<(std::ostream& os, const SpecialClocksInfo &g) {
  os << (g.global_clock.first.second ? "(" : "[") << g.global_clock.first.first
	   << ", "
     << g.global_clock.second.first << (g.global_clock.second.second ? ")" : "]")
		 << " + [0, "
     << g.max_delay.first << (g.max_delay.second ? ")" : "]");
	return os;
}

SpecialClocksInfo
UTAPTraceParser::determineSpecialClockBounds(dbm_t differences) {
  unordered_map<string, timepoint> ids;
  SpecialClocksInfo res;
  size_t x = 0;
  size_t t0 = 0;
  size_t glob = 0;
  size_t num_clocks = trace_ta.clocks.size() + 1;
  // Floyd-Warshall algorithm is applied here:
  // 1. all entries are set to the maximum value
  std::vector<std::vector<dbm_entry_t>> distances(
      num_clocks,
      std::vector<dbm_entry_t>(
          num_clocks,
          std::make_pair(std::numeric_limits<timepoint>::max(), true)));
  // 2. all weights are added
  for (const auto &edge : differences) {
    auto ins_source = ids.insert(::std::make_pair(edge.first.first, x));
    if (ins_source.second) {
      if (edge.first.first.find(constants::GLOBAL_CLOCK) != string::npos) {
        glob = x;
      }
      if (edge.first.first.find("t(0)") != string::npos) {
        t0 = x;
      }
      x++;
    }
    auto ins_dest = ids.insert(::std::make_pair(edge.first.second, x));
    if (ins_dest.second) {
      if (edge.first.second.find(constants::GLOBAL_CLOCK) != string::npos) {
        glob = x;
      }
      if (edge.first.second.find("t(0)") != string::npos) {
        t0 = x;
      }
      x++;
    }
    distances[ins_source.first->second][ins_dest.first->second] = edge.second;
  }
  // 3. self loops get null costs
  dbm_entry_t null_constraint = std::make_pair(0, false);
  for (size_t i = 0; i < num_clocks; i++) {
    distances[i][i] = null_constraint;
  }
  // 4. all possible shorter indirect routes are considered
  for (size_t k = 0; k < num_clocks; k++) {
    for (size_t i = 0; i < num_clocks; i++) {
      for (size_t j = 0; j < num_clocks; j++) {
        dbm_entry_t curr_indirect_path = distances[i][k] + distances[k][j];
        if (distances[i][j] > curr_indirect_path) {
          distances[i][j] = curr_indirect_path;
        }
      }
    }
  }
  // 5. check for negative cycle (= self loop with negative costs)
  bool valid = true;
  for (size_t i = 0; i < num_clocks; i++) {
    if (distances[i][i].first != 0) {
      valid = false;
    }
  }
  // Floyd-Warshall end

  // check if there no negative cycles
  if (!valid) {
    std::cerr << "Error - Negative cycle in matrix" << std::endl;
    return res;
  }
  // determine maximum delay in the symbolic state

  // Note that distances[t0][cl] holds the negated lower bound constraint on
  // clock cl.
  dbm_entry_t max_delay =
      std::make_pair(std::numeric_limits<timepoint>::max(), false);
  for (size_t i = 0; i < distances[t0].size(); i++) {
    if (t0 != i) {
      // the the lower bound is negated, so flip everything
      // e.g. if the entries are representing (i-t0 < y) and (t0-i <= -x)
      // then the entries are (<,y) and (<=, -x). To apply constraint
      // arithmetric we have remodel the lb constraint:
      // t0-i <= -x   <=>   i-t0 >= x
      // The corresponding upper bound constraint describing the excluded set
      // is i -t0 < x, or as entry: (<,x)
      dbm_entry_t curr_lb = distances[t0][i];
      // curr_lb.first *=-1;
      // curr_lb.second = !curr_lb.second;
      dbm_entry_t curr_max_delay = distances[i][t0] + curr_lb;
      if (curr_max_delay < max_delay) {
        max_delay = curr_max_delay;
      }
    }
  }
  // to obtain the lower bound we have to negate the value, but not the
  // strictness property
  dbm_entry_t glob_lb = distances[t0][glob];
  glob_lb.first *= -1;
  res.global_clock = ::std::make_pair(glob_lb, distances[glob][t0]);
  res.max_delay = max_delay;
  return res;
}

::std::string UTAPTraceParser::addStateToTraceTA(::std::string state_id) {
  if (parsed) {
    return state_id;
  }
  auto source_state_it =
      std::find_if(source_states.begin(), source_states.end(),
                   [state_id](const State &s) { return s.id == state_id; });
  std::string ta_state_id = "";
  if (source_state_it != source_states.end()) {
    State ta_state(*source_state_it);
    ta_state_id = "trace" + std::to_string(trace_to_ta_ids.size());
    ta_state.id = ta_state_id;
    trace_to_ta_ids.insert(std::make_pair(ta_state_id, source_state_it->id));
    trace_ta.states.push_back(ta_state);
  } else {
    std::cout
        << "UTAPTraceParser addStateToTraceTA: Error, source state not found: "
        << state_id << std::endl;
  }
  return ta_state_id;
}

void UTAPTraceParser::parseState(std::string &currentReadLine) {
  dbm_t closed_dbm;
  size_t eow = currentReadLine.find_first_of(" \t");
  // skip "State: "
  currentReadLine = currentReadLine.substr(eow + 1);
  // skip component name
  eow = currentReadLine.find_first_of(".");
  currentReadLine = currentReadLine.substr(eow + 1);
  eow = currentReadLine.find_first_of(" ");
  std::string parsed_state_name = currentReadLine.substr(0, eow);
  // skip state name
  currentReadLine = currentReadLine.substr(eow + 1);
  while (currentReadLine != "") {
    eow = currentReadLine.find_first_of("-");
    string t_source = currentReadLine.substr(0, eow);
    // skip source state and "-"
    currentReadLine = currentReadLine.substr(eow + 1);
    eow = currentReadLine.find_first_of("<");
    string t_dest = currentReadLine.substr(0, eow);
    t_source = Filter::getSuffix(t_source, '.');
    t_dest = Filter::getSuffix(t_dest, '.');
    bool strict = true;
    // skip dest state and "<"
    currentReadLine = currentReadLine.substr(eow + 1);
    if (currentReadLine.front() == '=') {
      // skip "="
      // TODO: respect bounds when calculating time window
      currentReadLine.erase(0, 1);
      strict = false;
    }
    eow = currentReadLine.find_first_of(" \t");
    int t_weight = stoi(currentReadLine.substr(0, eow));
    currentReadLine = currentReadLine.substr(eow + 1);
    auto ins = closed_dbm.insert(::std::make_pair(
        ::std::make_pair(t_source, t_dest), std::make_pair(t_weight, strict)));
    if (!ins.second) {
      cout << "UTAPTraceParser parseState: ERROR duplicate dbm entry" << endl;
    }
  }
  if (parsed) {
    // we currently parse a trace from the trace TA, therefore the name is
    // already correct.
    ta_to_symbolic_state.insert(std::make_pair(parsed_state_name, closed_dbm));
  } else {
    size_t state_suffix = trace_ta.states.size();
    if (trace_ta.states.size() != 0) {
      state_suffix -= 1;
    }
    ta_to_symbolic_state.insert(
        std::make_pair("trace" + std::to_string(state_suffix), closed_dbm));
  }
}

void UTAPTraceParser::parseTransition(std::string &currentReadLine) {
  size_t eow = currentReadLine.find_first_of(" \t");
  currentReadLine = currentReadLine.substr(eow + 1);
  eow = currentReadLine.find_first_of(".");
  string component = currentReadLine.substr(0, eow);
  // skip component of source id
  currentReadLine = currentReadLine.substr(eow + 1);
  eow = currentReadLine.find_first_of(" \t");
  string source_id = currentReadLine.substr(0, eow);
  // skip source id
  currentReadLine = currentReadLine.substr(eow + 1);
  eow = currentReadLine.find_first_of(".");
  // skip delimiter -> and component of dest id
  currentReadLine = currentReadLine.substr(eow + 1);
  eow = currentReadLine.find_first_of(" \t");
  string dest_id = currentReadLine.substr(0, eow);
  eow = currentReadLine.find_first_of("{");
  // skip dest id (only labels remaining)
  currentReadLine = currentReadLine.substr(eow + 1);
  eow = currentReadLine.find_first_of(";");
  string guard_str = currentReadLine.substr(0, eow);
  // skip guard and "; "
  currentReadLine = currentReadLine.substr(eow + 2);
  eow = currentReadLine.find_first_of(";");
  string sync_str = currentReadLine.substr(0, eow);
  // skip sync and "; "
  currentReadLine = currentReadLine.substr(eow + 2);
  eow = currentReadLine.find_first_of(";");
  string update_str = currentReadLine.substr(0, eow);
  // remove empty guards
  guard_str = (guard_str == "1") ? "" : convertCharsToHTML(guard_str);
  sync_str = (sync_str == "0") ? "" : convertCharsToHTML(sync_str);
  update_str = (update_str == "1") ? "" : convertCharsToHTML(update_str);
  // add parsed transition to trace ta
  std::string trace_ta_source_id = source_id;
  std::string trace_ta_dest_id = dest_id;
  // if the parser already parsed a trace this means that the now parsed state
  // ids correspond to the states of trace_ta, therefore only update the already
  // parsed guard.
  if (parsed) {
    auto trans_entry = std::find_if(
        trace_ta.transitions.begin(), trace_ta.transitions.end(),
        [trace_ta_source_id, trace_ta_dest_id](const Transition &t) {
          return t.source_id == trace_ta_source_id &&
                 t.dest_id == trace_ta_dest_id;
        });
    if (trans_entry != trace_ta.transitions.end()) {
      trans_entry->guard = std::make_unique<UnparsedCC>(UnparsedCC(guard_str));
    } else {
      std::cout << "UTAPTraceParser parseTransition: cannot find original "
                   "transition while parsing trace from trace TA: "
                << trace_ta_source_id << " -> " << trace_ta_dest_id
                << std::endl;
    }
    // else add a fresh state to trace_ta.
  } else {
    if (trace_to_ta_ids.size() == 0) {
      // this is the first transition, so also create the source state
      trace_ta_source_id = addStateToTraceTA(source_id);
    } else {
      trace_ta_source_id =
          "trace" + std::to_string((trace_to_ta_ids.size() - 1));
    }
    trace_ta_dest_id = addStateToTraceTA(dest_id);
    trace_ta.transitions.push_back(Transition(
        trace_ta_source_id, trace_ta_dest_id, "", UnparsedCC(guard_str),
        Transition::updateFromString(update_str, trace_ta.clocks), sync_str));
  }
}

::std::vector<::std::string>
UTAPTraceParser::getActionsFromTraceTrans(const Transition &trans,
                                          const Automaton &base_ta,
                                          const Automaton &plan_ta) {
  std::vector<std::string> res;
  std::string source_id = trace_to_ta_ids[trans.source_id];
  std::string dest_id = trace_to_ta_ids[trans.dest_id];
  std::string guard_str = trans.guard.get()->toString();
  std::string update_str = trans.updateToString();
  std::string sync_str = trans.sync;
  // obtain action name
  string pa_source_id = Filter::getPrefix(source_id, constants::TL_SEP);
  string pa_dest_id = Filter::getPrefix(dest_id, constants::TL_SEP);
  if (pa_dest_id == constants::QUERY) {
    return res;
  }
  if (pa_source_id != pa_dest_id) {
    auto pa_trans = ::std::find_if(
        plan_ta.transitions.begin(), plan_ta.transitions.end(),
        [pa_source_id, pa_dest_id, guard_str, sync_str,
         update_str](const Transition &t) {
          return t.source_id == pa_source_id && t.dest_id == pa_dest_id &&
                 guard_str.find(t.guard.get()->toString()) != string::npos &&
                 sync_str.find(t.sync) != string::npos &&
                 update_str.find(t.updateToString()) != string::npos;
        });
    if (pa_trans == plan_ta.transitions.end()) {
      cout << "ERROR:  cannot find plan ta transition: " << pa_source_id
           << " -> " << pa_dest_id << " {" << guard_str << "; " << sync_str
           << "; " << update_str << "}" << endl;
    } else if (pa_trans->action != "") {
      res.push_back(pa_trans->action);
    } else {
      res.push_back("(" + pa_trans->source_id + " -> " + pa_trans->dest_id +
                    ")");
    }
  }
  string base_source_id = Filter::getSuffix(source_id, constants::BASE_SEP);
  string base_dest_id = Filter::getSuffix(dest_id, constants::BASE_SEP);
  auto base_trans = ::std::find_if(
      base_ta.transitions.begin(), base_ta.transitions.end(),
      [base_source_id, base_dest_id, guard_str, sync_str,
       update_str](const Transition &t) {
        return t.source_id == base_source_id &&
               t.dest_id.find(base_dest_id) != string::npos &&
               isPiecewiseContained(t.guard.get()->toString(), guard_str,
                                    constants::CC_CONJUNCTION) &&
               sync_str.find(t.sync) != string::npos &&
               isPiecewiseContained(t.updateToString(), update_str,
                                    constants::UPDATE_CONJUNCTION);
      });
  if (base_trans == base_ta.transitions.end()) {
    if (base_source_id != base_dest_id) {
      cout << "ERROR:  cannot find base ta transition: " << base_source_id
           << " -> " << base_dest_id << " {" << guard_str << "; " << sync_str
           << "; " << update_str << "}" << endl;
    }
  } else {
    std::vector<std::string> action_vec =
        splitBySep(base_trans->action, constants::ACTION_SEP);
    std::vector<std::string> source_vec =
        splitBySep(base_trans->source_id, constants::COMPONENT_SEP);
    std::vector<std::string> dest_vec =
        splitBySep(base_trans->dest_id, constants::COMPONENT_SEP);
    if (action_vec.size() == source_vec.size() &&
        source_vec.size() == dest_vec.size()) {
      for (long unsigned int i = 0; i < action_vec.size(); i++) {
        if (source_vec[i] != dest_vec[i] || action_vec[i] != "") {
          res.push_back(source_vec[i] + " -" + action_vec[i] + "-> " +
                        dest_vec[i]);
        }
      }
    }
  }
  return res;
}

timed_trace_t UTAPTraceParser::applyDelay(size_t delay_pos, timepoint delay) {
  if (delay_pos >= trace_ta.transitions.size()) {
    std::cout
        << "UTAPTraceParser applyDelay: Error, delay pos not valid. Abort."
        << std::endl;
    return timed_trace_t();
  }
  auto global_clock_it = std::find_if(
      trace_ta.clocks.begin(), trace_ta.clocks.end(),
      [](const auto &cl) { return cl.get()->id == constants::GLOBAL_CLOCK; });
  if (global_clock_it != trace_ta.clocks.end()) {
    for (size_t trans_offset = 0; trans_offset <= delay_pos; trans_offset++) {
      auto ta_trans_it = trace_ta.transitions.begin() + trans_offset;
      timepoint execute_at = (parsed_trace.begin() + trans_offset)
                                 ->first.global_clock.second.first;
      if (trans_offset == delay_pos) {
        execute_at = (parsed_trace.begin() + trans_offset)
                         ->first.global_clock.first.first +
                     delay;
      }
      ta_trans_it->guard = std::make_unique<ConjunctionCC>(
          *ta_trans_it->guard.get(),
          ComparisonCC(*global_clock_it, ComparisonOp::GTE, execute_at));
    }
    AutomataSystem trace_system;
    trace_system.instances.push_back(std::make_pair(trace_ta, ""));
    std::string query_str =
        "E<> sys_" + trace_ta.prefix + "." + trace_ta.states.back().id;
    uppaalcalls::solve(trace_system, "trace_ta", query_str);
    ta_to_symbolic_state.clear();
    parseTraceInfo("trace_ta.trace");

    for (auto &cl_val : curr_clock_values) {
      cl_val.second = std::make_pair(0, false);
    }
    std::vector<SpecialClocksInfo> timings = getTraceTimings();
    assert(timings.size() == parsed_trace.size() + 1);
    for (size_t i = delay_pos; i < parsed_trace.size(); i++) {
      (parsed_trace.begin() + i)->first = *(timings.begin() + i);
    }
    return parsed_trace;
  } else {
    std::cout
        << "UTAPTraceParser applyDelay: Error, global clock not found. Abort."
        << std::endl;
    return parsed_trace;
  }
}

::std::vector<SpecialClocksInfo> UTAPTraceParser::getTraceTimings() {
  std::vector<SpecialClocksInfo> res;
  if (parsed == true) {
    for (auto ta_trans = trace_ta.transitions.begin();
         ta_trans != trace_ta.transitions.end(); ++ta_trans) {
      if (ta_trans == trace_ta.transitions.begin()) {
        auto src_dbm_it = ta_to_symbolic_state.find(ta_trans->source_id);
        if (src_dbm_it != ta_to_symbolic_state.end()) {
          res.push_back(determineSpecialClockBounds(src_dbm_it->second));
        } else {
          std::cout << "ERROR, dbm not found: " << ta_trans->source_id
                    << std::endl;
        }
      }
      auto dst_dbm_it = ta_to_symbolic_state.find(ta_trans->dest_id);
      if (dst_dbm_it != ta_to_symbolic_state.end()) {
        SpecialClocksInfo src_duration =
            determineSpecialClockBounds(dst_dbm_it->second);
        // add upper bound to previous (=source) state
        res.back().global_clock.second = src_duration.global_clock.first;
        // determine the updated upper bound of t(0) - global_clock by
        // adding the negated previous bound together with the current one.
        // Note that the bound is stored by negated by default, therefore
        // the current lb is actually negated.
        dbm_entry_t last_lb = res.back().global_clock.first;
        dbm_entry_t progress = src_duration.global_clock.first;
        // last_lb.first *= -1;
        progress = progress - last_lb;
        // progress all clocks
        for (const auto &cl : trace_ta.clocks) {
          curr_clock_values[cl] = curr_clock_values[cl] + progress;
        }
        // reset clocks on transition
        for (const auto &cl_up : ta_trans->update) {
          curr_clock_values[cl_up] = std::make_pair(0, false);
        }
        // update the dbm
        for (const auto &cl : trace_ta.clocks) {
          auto cl_lb_entry =
              std::find_if(dst_dbm_it->second.begin(), dst_dbm_it->second.end(),
                           [cl](const auto &entry) {
                             return entry.first.first == "t(0)" &&
                                    entry.first.second == cl.get()->id;
                           });
          if (cl_lb_entry == dst_dbm_it->second.end()) {
            std::cout << "create new entry" << std::endl;
            dst_dbm_it->second[std::make_pair("t(0)", cl.get()->id)] =
                curr_clock_values[cl];
            dst_dbm_it->second[std::make_pair("t(0)", cl.get()->id)].first *=
                -1;
          } else {
            cl_lb_entry->second = curr_clock_values[cl];
            cl_lb_entry->second.first *= -1;
          }
        }
        res.push_back(determineSpecialClockBounds(dst_dbm_it->second));
      } else {
        std::cout << "ERROR, dbm not found: " << ta_trans->dest_id << std::endl;
      }
    }
    return res;
  } else {
    std::cout
        << "UTAPTraceParser getTimedTrace: Trace was not parsed yet. Abort. "
        << std::endl;
    return res;
  }
}

timed_trace_t UTAPTraceParser::getTimedTrace(const Automaton &base_ta,
                                             const Automaton &plan_ta) {
  std::vector<SpecialClocksInfo> trace_timings = getTraceTimings();
  timed_trace_t res;
  assert(trace_timings.size() == trace_ta.transitions.size() + 1);
  for (size_t i = 0; i < trace_ta.transitions.size(); i++) {
    res.push_back(std::make_pair(
        *(trace_timings.begin() + i),
        getActionsFromTraceTrans(*(trace_ta.transitions.begin() + i), base_ta,
                                 plan_ta)));
  }
  parsed_trace = res;
  return res;
}

bool UTAPTraceParser::parseTraceInfo(const std::string &file) {
  // file
  std::fstream fileStream;
  std::string currentReadLine;
  fileStream.open(file, std::fstream::in); // open the file in Input mode
  if (getline(fileStream, currentReadLine)) {
    if (currentReadLine.size() > 5) {
      if (currentReadLine.substr(0, 5) == "State") {
        parseState(currentReadLine);
      }
    } else {
      std::cout << "UTAPTraceParser parseTraceInfo: expected trace to begin "
                   "with inital state, but read: "
                << currentReadLine << std::endl;
      return false;
    }
  } else {
    std::cout << "UTAPTraceParser parseTraceInfo: trace not valid" << std::endl;
    return false;
  }
  while (getline(fileStream, currentReadLine)) {
    if (currentReadLine.size() > 10 &&
        currentReadLine.substr(0, 10) == "Transition") {
      parseTransition(currentReadLine);
      getline(fileStream, currentReadLine);
      if (currentReadLine != "") {
        std::cout << "UTAPTraceParser parseTraceInfo: expected empty line "
                     "after transition, but read: "
                  << currentReadLine << std::endl;
        return false;
      }
      if (getline(fileStream, currentReadLine)) {
        if (currentReadLine.size() > 5) {
          if (currentReadLine.substr(0, 5) == "State") {
            parseState(currentReadLine);
          }
        } else {
          std::cout << "UTAPTraceParser parseTraceInfo: expected state after "
                       "transition, but read: "
                    << currentReadLine << std::endl;
          return false;
        }
      }
    } else {
      continue;
    }
  }
  fileStream.close();
  parsed = true;
  return true;
}

UTAPTraceParser::UTAPTraceParser(const AutomataSystem &s)
    : trace_ta(Automaton({}, {}, "trace_ta", false)) {
  trace_ta.clocks.insert(s.globals.clocks.begin(), s.globals.clocks.end());
  for (const auto &ta : s.instances) {
    source_states.insert(source_states.begin(), ta.first.states.begin(),
                         ta.first.states.end());
    trace_ta.clocks.insert(ta.first.clocks.begin(), ta.first.clocks.end());
  }
  for (const auto &cl : trace_ta.clocks) {
    curr_clock_values.insert(std::make_pair(cl, std::make_pair(0, false)));
  }
}
} // end namespace taptenc
