#include "vis_info.h"
#include "constants.h"
#include "timed_automata.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace taptenc;

systemVisInfo::systemVisInfo(AutomataSystem &s) {
  for (auto it = s.instances.begin(); it != s.instances.end(); ++it) {
    state_info.push_back(this->generateStateInfo(it->first.states));
    transition_info.push_back(
        this->generateTransitionInfo(it->first.transitions, state_info.back()));
  }
  transition_counters.resize(s.instances.size());
}

systemVisInfo::systemVisInfo(const TimeLines &direct_encoding,
                             const std::vector<State> &pa_order) {
  int x_offset = 0;
  int y_offset = 0;
  int instances = 0;
  state_info.resize(1);
  transition_info.resize(1);
  for (auto tl : pa_order) {
    auto search = direct_encoding.find(tl.id);
    if (search != direct_encoding.end()) {
      for (const auto &entity : search->second) {
        instances++;
        auto si = this->generateStateInfo(entity.second.ta.states, x_offset,
                                          y_offset);
        state_info[0].insert(si.begin(), si.end());
        y_offset += 500;
        auto ti = this->generateTransitionInfo(entity.second.ta.transitions,
                                               state_info.back());
        transition_info[0].insert(ti.begin(), ti.end());
      }
      y_offset = 0;
      x_offset += 800;
    }
  }
  auto search = direct_encoding.find(constants::QUERY);
  if (search != direct_encoding.end()) {
    for (const auto &entity : search->second) {
      instances++;
      auto si =
          this->generateStateInfo(entity.second.ta.states, x_offset, y_offset);
      state_info[0].insert(si.begin(), si.end());
      auto ti = this->generateTransitionInfo(entity.second.ta.transitions,
                                             state_info.back());
      transition_info[0].insert(ti.begin(), ti.end());
    }
  }
  for (const auto &tl : direct_encoding) {
    for (const auto &entity : tl.second) {
      auto iti = this->generateTransitionInfo(entity.second.trans_out,
                                              state_info.back());
      transition_info[0].insert(iti.begin(), iti.end());
    }
  }
  transition_counters.resize(instances);
}

std::pair<int, int> systemVisInfo::getStatePos(int component_index,
                                               std::string id) {
  auto s_info = state_info[component_index].find(id);
  std::pair<int, int> res;
  if (s_info == state_info[component_index].end()) {
    std::cout << "systemVisInfo getStatePos: Could not find StateVisInfo"
              << std::endl;
    res = std::make_pair(0, 0);
    return res;
  } else {
    res = s_info->second.pos;
    return res;
  }
}

std::vector<std::pair<int, int>>
systemVisInfo::getTransitionPos(int component_index, std::string source_id,
                                std::string dest_id) {
  std::pair<std::string, std::string> spair =
      std::make_pair(source_id, dest_id);
  if (transition_counters[component_index].find(spair) ==
      transition_counters[component_index].end()) {
    transition_counters[component_index][spair] = 1;
  } else {
    transition_counters[component_index][spair]++;
  }
  auto t_info = transition_info[component_index].find(spair);
  std::vector<std::pair<int, int>> res;
  if (t_info == transition_info[component_index].end()) {
    std::cout
        << "systemVisInfo getTransitionPos: Could not find TransitionVisInfo"
        << std::endl;
    res.push_back(std::make_pair(0, 0));
    return res;
  } else {
    if (source_id != dest_id) {
      t_info->second.poi.push_back(std::make_pair(
          t_info->second.mid_point.first +
              (int)(t_info->second.unit_normal.first *
                    (float)DUPE_EDGE_DELIMITER *
                    transition_counters[component_index][spair]),
          t_info->second.mid_point.second +
              (int)(t_info->second.unit_normal.second *
                    (float)DUPE_EDGE_DELIMITER *
                    transition_counters[component_index][spair])));

    } else {
      t_info->second.poi[0].second +=
          transition_counters[component_index][spair] * SELF_LOOP_DELIMITER;
      t_info->second.poi[1].second +=
          transition_counters[component_index][spair] * SELF_LOOP_DELIMITER;
    }

    res.push_back(t_info->second.mid_point);
    res.insert(res.end(), t_info->second.poi.begin(), t_info->second.poi.end());
    return res;
  }
}

std::unordered_map<std::string, StateVisInfo>
systemVisInfo::generateStateInfo(const std::vector<State> &states, int x_offset,
                                 int y_offset) {
  int row_length = sqrt(states.size());
  std::unordered_map<std::string, StateVisInfo> res;
  int x = x_offset;
  int y = y_offset;
  for (auto it = states.begin(); it != states.end(); ++it) {
    auto emplaced = res.emplace(it->id, StateVisInfo(std::make_pair(x, y)));
    x += ROW_DELIMITER;
    if (emplaced.second == false) {
      std::cout << "systemVisInfo generateStatePositions: two states have the "
                   "same id: "
                << it->id << std::endl;
    }
    if (x > x_offset + (row_length * ROW_DELIMITER)) {
      x = x_offset;
      y += COL_DELIMITER;
    }
  }
  return res;
}

std::unordered_map<std::pair<std::string, std::string>, TransitionVisInfo>
systemVisInfo::generateTransitionInfo(
    const std::vector<Transition> &transitions,
    const std::unordered_map<std::string, StateVisInfo> &state_info) {
  std::unordered_map<std::pair<std::string, std::string>, TransitionVisInfo>
      res;
  for (auto it = transitions.begin(); it != transitions.end(); ++it) {
    if (res.find(std::make_pair(it->source_id, it->dest_id)) == res.end()) {
      auto curr_source = state_info.find(it->source_id);
      auto curr_dest = state_info.find(it->dest_id);
      if (curr_source == state_info.end() || curr_dest == state_info.end()) {
        std::cout << "generateTransitionInfo: state info not found, source "
                  << it->source_id << " dest " << it->dest_id << std::endl;
        continue;
      }
      TransitionVisInfo curr_info(curr_source->second.pos,
                                  curr_dest->second.pos);
      if (curr_info.is_self_loop) {
        curr_info.poi.push_back(curr_source->second.pos +
                                std::make_pair(-LOOP_X_SHIFT, LOOP_Y_SHIFT));
        curr_info.poi.push_back(curr_source->second.pos +
                                std::make_pair(LOOP_X_SHIFT, LOOP_Y_SHIFT));
        curr_info.mid_point =
            iMidPoint(curr_source->second.pos +
                          std::make_pair(-LOOP_X_SHIFT, LOOP_Y_SHIFT),
                      curr_source->second.pos +
                          std::make_pair(LOOP_X_SHIFT, LOOP_Y_SHIFT));
      } else {
        curr_info.mid_point =
            iMidPoint(curr_source->second.pos, curr_dest->second.pos);
      }
      res.emplace(std::make_pair(it->source_id, it->dest_id), curr_info);
    }
  }
  return res;
}
