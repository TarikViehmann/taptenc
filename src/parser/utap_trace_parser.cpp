/** \file
 * Parser for uppaal symbolic traces (.trace files).
 *
 * \author (2019) Tarik Viehmann
 */
#include "utap_trace_parser.h"
#include "../constants.h"
#include "../constraints/constraints.h"
#include "../encoder/filter.h"
#include "../timed-automata/timed_automata.h"
#include "../utils.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <unordered_map>

#include <boost/graph/directed_graph.hpp>
#include <boost/graph/exterior_property.hpp>
#include <boost/graph/floyd_warshall_shortest.hpp>

using std::cout;
using std::endl;
using std::iostream;
using std::pair;
using std::string;
using std::unordered_map;
namespace taptenc {
// type for weight/distance on each edge

SpecialClocksInfo
UTAPTraceParser::determineSpecialClockBounds(dbm_t differences) {
  typedef timepoint t_weight;

  // define the graph type
  typedef boost::property<boost::edge_weight_t, t_weight> EdgeWeightProperty;
  typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,
                                boost::no_property, EdgeWeightProperty>
      Graph;

  typedef boost::property_map<Graph, boost::edge_weight_t>::type WeightMap;

  // Declare a matrix type and its corresponding property map that
  // will contain the distances between each pair of vertices.
  typedef boost::exterior_vertex_property<Graph, t_weight> DistanceProperty;
  typedef DistanceProperty::matrix_type DistanceMatrix;
  typedef DistanceProperty::matrix_map_type DistanceMatrixMap;
  Graph g;
  unordered_map<string, timepoint> ids;
  SpecialClocksInfo res;
  size_t x = 0;
  size_t t0 = 0;
  size_t glob = 0;
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
    boost::add_edge(ins_source.first->second, ins_dest.first->second,
                    edge.second, g);
  }
  WeightMap weight_pmap = boost::get(boost::edge_weight, g);

  // set the distance matrix to receive the floyd warshall output
  DistanceMatrix distances(num_vertices(g));
  DistanceMatrixMap dm(distances, g);

  // find all pairs shortest paths
  bool valid = floyd_warshall_all_pairs_shortest_paths(
      g, dm, boost::weight_map(weight_pmap));

  // check if there no negative cycles
  if (!valid) {
    std::cerr << "Error - Negative cycle in matrix" << std::endl;
    return res;
  }
  timepoint max_delay = std::numeric_limits<timepoint>::max();
  for (size_t i = 0; i < distances[t0].size(); i++) {
    if (t0 != i) {
      timepoint curr_max_delay = distances[i][t0] + distances[t0][i];
      if (curr_max_delay < max_delay) {
        max_delay = curr_max_delay;
      }
    }
  }
  res.global_clock =
      ::std::make_pair(-distances[t0][glob], distances[glob][t0]);
  res.max_delay = max_delay;
  return res;
}

::std::string UTAPTraceParser::addStateToTraceTA(::std::string state_id) {
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
    std::cout << "UTAPTraceParser parseState: Error, source state not found: "
              << state_id << std::endl;
  }
  std::cout << "CREATED: " << ta_state_id << std::endl;
  return ta_state_id;
}

int UTAPTraceParser::parseState(std::string &currentReadLine) {
  dbm_t closed_dbm;
  size_t eow = currentReadLine.find_first_of(" \t");
  // skip "State: "
  currentReadLine = currentReadLine.substr(eow + 1);
  // skip state name
  eow = currentReadLine.find_first_of(" ");
  currentReadLine = currentReadLine.substr(eow + 1);
  while (currentReadLine != "") {
    eow = currentReadLine.find_first_of("-");
    string t_source = currentReadLine.substr(0, eow);
    // skip source state and "-"
    currentReadLine = currentReadLine.substr(eow + 1);
    eow = currentReadLine.find_first_of("<");
    string t_dest = currentReadLine.substr(0, eow);
    // skip dest state and "<"
    currentReadLine = currentReadLine.substr(eow + 1);
    if (currentReadLine.front() == '=') {
      // skip "="
      // TODO: respect bounds when calculating time window
      currentReadLine.erase(0, 1);
    }
    eow = currentReadLine.find_first_of(" \t");
    int t_weight = stoi(currentReadLine.substr(0, eow));
    currentReadLine = currentReadLine.substr(eow + 1);
    auto ins = closed_dbm.insert(
        ::std::make_pair(::std::make_pair(t_source, t_dest), t_weight));
    if (!ins.second) {
      cout << "ERROR: duplicate dbm entry" << endl;
    }
  }
  ta_to_symbolic_state.insert(
      std::make_pair("trace" + std::to_string(source_states.size() - 1)),
      closed_dbm);
  specialClocksInfo gci = determineSpecialClockBounds(closed_dbm);
  return gci.global_clock.first;
}

::std::vector<::std::string>
UTAPTraceParser::parseTransition(std::string &currentReadLine,
                                 const Automaton &base_ta,
                                 const Automaton &plan_ta) {
  std::vector<std::string> res;
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
  std::string trace_ta_source_id;
  if (trace_to_ta_ids.size() == 0) {
    // this is the first transition, so also create the source state
    trace_ta_source_id = addStateToTraceTA(source_id);
  } else {
    trace_ta_source_id = "trace" + std::to_string((trace_to_ta_ids.size() - 1));
  }
  std::string trace_ta_dest_id = addStateToTraceTA(dest_id);
  trace_ta.transitions.push_back(Transition(
      trace_ta_source_id, trace_ta_dest_id, "", UnparsedCC(guard_str),
      Transition::updateFromString(update_str, trace_ta.clocks), sync_str));
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

// vector<pair<Transition,size_t>>
::std::vector<::std::pair<timepoint, ::std::vector<::std::string>>>
UTAPTraceParser::parseTraceInfo(const std::string &file,
                                const Automaton &base_ta,
                                const Automaton &plan_ta) {

  // file
  std::fstream fileStream;
  std::string currentReadLine;
  fileStream.open(file, std::fstream::in); // open the file in Input mode
  cout << "----------------------------------" << endl;
  cout << "---------Final Plan---------------" << endl;
  cout << "----------------------------------" << endl;
  std::vector<std::pair<timepoint, std::vector<std::string>>> info;
  if (!getline(fileStream, currentReadLine)) {
    std::cout << "UTAPTraceParser parseTraceInfo: trace not valid" << std::endl;
  }
  while (getline(fileStream, currentReadLine)) {
    if (currentReadLine.size() > 10 &&
        currentReadLine.substr(0, 10) == "Transition") {
      std::vector<std::string> trans_vec =
          parseTransition(currentReadLine, base_ta, plan_ta);
      getline(fileStream, currentReadLine);
      if (currentReadLine != "") {
        std::cout << "UTAPTraceParser parseTraceInfo: expected empty line "
                     "after transition, but read: "
                  << currentReadLine << std::endl;
      }
      if (getline(fileStream, currentReadLine)) {
        if (currentReadLine.size() > 5) {
          if (currentReadLine.substr(0, 5) == "State") {
            info.push_back(make_pair(parseState(currentReadLine), trans_vec));
          }
        } else {
          std::cout << "UTAPTraceParser parseTraceInfo: expected state after "
                       "transition, but read: "
                    << currentReadLine << std::endl;
          break;
        }
      }
    } else {
      continue;
    }
  }
  fileStream.close();
  for (const auto &entry : info) {
    if (entry.second.size() > 0) {
      std::cout << entry.first << ": ";
      for (const auto &t : entry.second) {
        std::cout << "\t" << t << std::endl;
      }
    }
  }
  return info;
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
    curr_clock_values.insert(cl, 0);
    std::cout << cl.get()->id << std::endl;
  }
}
} // end namespace taptenc
