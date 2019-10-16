/****************************************************************************
 * vis_info.h - provides datastructures to store visual information on      *
 *              timed automata systems, such as state and transition        *
 *              coordinates.                                                *
 *                                                                          *
 * Author(s): (2019) Tarik Viehmann                                         *
 ****************************************************************************/

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
  ::std::vector<::std::pair<int, int>> poi; // nails to shape the edge
  bool is_self_loop;
  /*
   * creates visual information given endpoint coordinates of a transition
   * In particular, determines if the transition is a self loop and calculates
   * the unit normal
   *
   * @param arg_source_pos source state position
   * @param arg_dest_pos dest state position
   */
  transitionVisInfo(::std::pair<int, int> arg_source_pos,
                    ::std::pair<int, int> arg_dest_pos);
};
typedef struct transitionVisInfo TransitionVisInfo;

struct systemVisInfo {
private:
  // track the number of transitions that have the same source and dest
  ::std::vector<
      ::std::unordered_map<::std::pair<::std::string, ::std::string>, int>>
      m_transition_counters;

  // store the generated state and transition info for each TA of a system
  // the vector index corresponds to the systems TA index
  ::std::vector<::std::unordered_map<::std::string, StateVisInfo>> m_state_info;
  ::std::vector<::std::unordered_map<::std::pair<::std::string, ::std::string>,
                                     TransitionVisInfo>>
      m_transition_info;

  /*
   * generate coordinates for the given states
   *
   * @param states states to generate generate coordinates for
   * @return state id -> StateVisInfo mapping
   */
  ::std::unordered_map<::std::string, StateVisInfo>
  generateStateInfo(const ::std::vector<State> &states) const;

  /*
   * generate coordinates for the given states with an offset
   *
   * @param states states to generate generate coordinates for
   * @param x_offset x offset to add, stores the x coordinate of the rightmost
   *                 placed state afterwards
   * @param y_offset y offset to add, stores the y coordinate of the bottommost
   *                 placed state afterwards
   * @return state id -> StateVisInfo mapping
   */
  ::std::unordered_map<::std::string, StateVisInfo>
  generateStateInfo(const ::std::vector<State> &states, int &x_offset,
                    int &y_offset) const;

  /*
   * generate coordinates for the given transitions.
   * This includes a mid point and in case of a self loop also two pois (nails)
   *
   * @param transitions transitions to generate generate coordinates for
   * @param state_info mapping to retrieve associated state positions from
   * @return (source id, dest_id) -> TransitionVisInfo mapping
   */
  ::std::unordered_map<::std::pair<::std::string, ::std::string>,
                       TransitionVisInfo>
  generateTransitionInfo(const ::std::vector<Transition> &transitions,
                         const ::std::unordered_map<::std::string, StateVisInfo>
                             &state_info) const;

public:
  systemVisInfo() = default;

  /*
   * generate all necessary visual information from a given AutomataSystem
   *
   * @param s AutomataSystem to generate visual information for
   */
  systemVisInfo(AutomataSystem &s);

  /*
   * generate all necessary visual information for TimeLines representation
   * of a TA system.
   * Yields a clean visualization where the automata copies are grouped
   * according to the TimeLines Encoding and horizontally ordered
   * according to the plan ordering
   *
   * @param direct_encoding timelines to visualize
   * @param pa_order ordering of the plan actions to follow when generating
   *                 the visual information
   */
  systemVisInfo(const TimeLines &direct_encoding,
                const ::std::vector<::std::string> &pa_order);

  /*
   * get the position of a state
   *
   * @param component_index TA index corresponding to the TA index in the
   *                        associated AutomataSystem
   * @param id state id
   *
   * @return <x,y> position of the state (or <0,0> if the state is not known)
   *
   */
  ::std::pair<int, int> getStatePos(int component_index,
                                    ::std::string id) const;

  /*
   * get the endpoint positions of a transition
   *
   * @param component_index TA index corresponding to the TA index in the
   *                        associated AutomataSystem
   * @param source_id source id of the transition
   * @param dest_id dest id of the transition
   *
   * @return [<mid_point>, <poi_1>, <poi_n>] position of the nails (or [<0,0>]
   *         if the associated states cannot be found
   *
   */
  ::std::vector<::std::pair<int, int>> getTransitionPos(int component_index,
                                                        ::std::string source_id,
                                                        ::std::string dest_id);
};
typedef struct systemVisInfo SystemVisInfo;

} // end namespace taptenc
