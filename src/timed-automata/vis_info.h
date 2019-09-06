#pragma once

#include "timed_automata.h"
#include "utils.h"
#include <string>
#include <unordered_map>
#include <vector>

#define DUPE_EDGE_DELIMITER 30
#define SELF_LOOP_DELIMITER -30
#define ROW_DELIMITER 200
#define COL_DELIMITER 200
#define LOOP_X_SHIFT -30
#define LOOP_Y_SHIFT -30

namespace taptenc {

struct stateVisInfo {
  ::std::pair<int, int> pos;
  stateVisInfo(::std::pair<int, int> arg_pos) { pos = arg_pos; }
};
typedef struct stateVisInfo StateVisInfo;

struct transitionVisInfo {
  ::std::pair<int, int> mid_point;
  ::std::pair<float, float> unit_normal;
  ::std::vector<::std::pair<int, int>> poi;
  bool is_self_loop;
  transitionVisInfo(::std::pair<int, int> arg_source_pos,
                    ::std::pair<int, int> arg_dest_pos) {
    is_self_loop = (arg_source_pos.first == arg_dest_pos.first &&
                    arg_source_pos.second == arg_dest_pos.second)
                       ? true
                       : false;
    if (is_self_loop) {
      unit_normal = ::std::make_pair(0.f, -1.f);
    } else {
      unit_normal = iUnitNormalFromPoints(arg_source_pos, arg_dest_pos);
    }
  }
};
typedef struct transitionVisInfo TransitionVisInfo;

struct systemVisInfo {
private:
  ::std::vector<
      ::std::unordered_map<::std::pair<::std::string, ::std::string>, int>>
      transition_counters;
  ::std::vector<::std::unordered_map<::std::string, StateVisInfo>> state_info;
  ::std::vector<::std::unordered_map<::std::pair<::std::string, ::std::string>,
                                     TransitionVisInfo>>
      transition_info;
  ::std::unordered_map<::std::string, StateVisInfo>
  generateStateInfo(const ::std::vector<State> &states, int x_offset = 0,
                    int y_offset = 0);
  ::std::unordered_map<::std::pair<::std::string, ::std::string>,
                       TransitionVisInfo>
  generateTransitionInfo(
      const ::std::vector<Transition> &transitions,
      const ::std::unordered_map<::std::string, StateVisInfo> &state_info);

public:
  systemVisInfo() = default;
  systemVisInfo(AutomataSystem &s);
  systemVisInfo(
      const ::std::unordered_map<
          ::std::string,
          ::std::unordered_map<
              ::std::string, ::std::pair<Automaton, ::std::vector<Transition>>>>
          &direct_encoding,
      const ::std::vector<State> &pa_order);
  ::std::pair<int, int> getStatePos(int component_index, ::std::string id);
  ::std::vector<::std::pair<int, int>> getTransitionPos(int component_index,
                                                        ::std::string source_id,
                                                        ::std::string dest_id);
};
typedef struct systemVisInfo SystemVisInfo;

} // end namespace taptenc
