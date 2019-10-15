#pragma once

#include "timed_automata.h"
#include <string>
#include <unordered_map>
#include <vector>


namespace taptenc {

struct stateVisInfo {
  ::std::pair<int, int> pos;
  stateVisInfo(::std::pair<int, int> arg_pos);
};
typedef struct stateVisInfo StateVisInfo;

struct transitionVisInfo {
  ::std::pair<int, int> mid_point;
  ::std::pair<float, float> unit_normal;
  ::std::vector<::std::pair<int, int>> poi;
  bool is_self_loop;
  transitionVisInfo(::std::pair<int, int> arg_source_pos,
                    ::std::pair<int, int> arg_dest_pos);
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
  generateStateInfo(const ::std::vector<State> &states);
  ::std::unordered_map<::std::string, StateVisInfo>
  generateStateInfo(const ::std::vector<State> &states, int &x_offset,
                    int &y_offset);
  ::std::unordered_map<::std::pair<::std::string, ::std::string>,
                       TransitionVisInfo>
  generateTransitionInfo(
      const ::std::vector<Transition> &transitions,
      const ::std::unordered_map<::std::string, StateVisInfo> &state_info);

public:
  systemVisInfo() = default;
  systemVisInfo(AutomataSystem &s);
  systemVisInfo(const TimeLines &direct_encoding,
                const ::std::vector<::std::string> &pa_order);
  ::std::pair<int, int> getStatePos(int component_index, ::std::string id);
  ::std::vector<::std::pair<int, int>> getTransitionPos(int component_index,
                                                        ::std::string source_id,
                                                        ::std::string dest_id);
};
typedef struct systemVisInfo SystemVisInfo;

} // end namespace taptenc
