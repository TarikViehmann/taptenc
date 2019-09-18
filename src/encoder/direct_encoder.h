#pragma once
#include "encoder.h"
#include "filter.h"
#include "timed_automata.h"
#include "vis_info.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace taptenc {
typedef ::std::unordered_map<::std::string, ::std::string> OrigMap;
class DirectEncoder : Encoder {
private:
  size_t encode_counter = 0;
  size_t plan_ta_index;
  ::std::vector<PlanAction> plan;
  TimeLines pa_tls;
  ::std::vector<::std::string> pa_order;
  void generateBaseTimeLine(AutomataSystem &s, const int base_pos,
                            const int plan_pos);
  void addOutgoingTransOfOrigTL(const TimeLine &orig_tl, TimeLine &new_tl,
                                const OrigMap &to_orig,
                                ::std::string guard = "");
  ::std::vector<Transition>
  createTransitionsBackToOrigTL(const ::std::vector<Transition> &trans,
                                ::std::string prefix, ::std::string pa,
                                ::std::string guard = "");
  ::std::vector<Transition>
  addToPrefixOnTransitions(const ::std::vector<Transition> &trans,
                           ::std::string prefix, bool on_inner_trans = true,
                           bool on_outgoing_trans = true);
  void modifyTransitionsToNextTl(::std::vector<Transition> &trans,
                                 ::std::string curr_pa, ::std::string guard,
                                 ::std::string update, ::std::string sync,
                                 ::std::string op_name = "");
  void removeTransitionsToNextTl(::std::vector<Transition> &trans,
                                 ::std::string curr_pa);
  ::std::pair<int, int> calculateContext(const EncICInfo &info,
                                         ::std::string starting_pa,
                                         ::std::string ending_pa = "",
                                         int lb_offset = 0, int ub_offset = 0);
  OrigMap createOrigMapping(const TimeLines &orig_tls, ::std::string prefix);
  TimeLines createWindow(const TimeLines &origin, ::std::string start_pa,
                         ::std::string end_pa, const Filter &base_filter,
                         const Filter &target_filter, ::std::string prefix,
                         ::std::string clock, const EncICInfo &info);
  void createTransitionsBetweenWindows(
      const Automaton &base_ta, TimeLines &source_tls, TimeLines &dest_tls,
      const ::std::unordered_map<std::string, ::std::string> &map_to_orig,
      ::std::string start_pa, ::std::string end_pa, const Filter &target_filter,
      ::std::string guard, ::std::string update);
  void mergeWindows(TimeLines &dest, const TimeLines &to_add,
                    bool overwrite = false);

public:
  static void printTLs(const TimeLines &to_print);
  size_t getPlanTAIndex();
  void encodeUntilChain(AutomataSystem &s, const ::std::vector<EncICInfo> &info,
                        const ::std::string start_pa,
                        const ::std::string end_pa, const int base_pos = 0);
  void encodeInvariant(AutomataSystem &s, const ::std::vector<State> &targets,
                       const ::std::string pa);
  void encodeNoOp(AutomataSystem &s, const ::std::vector<State> &targets,
                  const ::std::string pa);
  void encodeFuture(AutomataSystem &s, const ::std::string pa,
                    const EncICInfo &info, int base_index = 0);
  void encodePast(AutomataSystem &s, ::std::vector<State> &targets,
                  const ::std::string pa, const Bounds bounds,
                  int base_index = 0);
  DirectEncoder(AutomataSystem &s, const ::std::vector<PlanAction> &plan,
                const int base_pos = 0);
  AutomataSystem createFinalSystem(const AutomataSystem &s,
                                   SystemVisInfo &s_vis);
};
} // end namespace taptenc
