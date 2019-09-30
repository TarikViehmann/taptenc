#pragma once
#include "enc_interconnection_info.h"
#include "encoder_utils.h"
#include "filter.h"
#include "plan_ordered_tls.h"
#include "timed_automata.h"
#include "vis_info.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace taptenc {
typedef ::std::unordered_map<::std::string, ::std::string> OrigMap;
class DirectEncoder {
private:
  PlanOrderedTLs po_tls;
  ::std::vector<PlanAction> plan;
  size_t plan_ta_index;
  size_t encode_counter = 0;
  void generateBaseTimeLine(AutomataSystem &s, const int base_pos,
                            const int plan_pos);
  ::std::pair<int, int> calculateContext(const TargetSpecs &specs,
                                         ::std::string starting_pa,
                                         ::std::string ending_pa = "",
                                         bool look_ahead = true,
                                         int lb_offset = 0, int ub_offset = 0);
  DirectEncoder(const PlanOrderedTLs &tls,
                const ::std::vector<PlanAction> &plan,
                const size_t plan_ta_index);

public:
  size_t getPlanTAIndex();
  void encodeUntilChain(AutomataSystem &s, const ChainInfo &info,
                        const ::std::string start_pa,
                        const ::std::string end_pa, const int base_pos = 0);
  void encodeInvariant(AutomataSystem &s, const ::std::vector<State> &targets,
                       const ::std::string pa);
  void encodeNoOp(AutomataSystem &s, const ::std::vector<State> &targets,
                  const ::std::string pa);
  void encodeFuture(AutomataSystem &s, const ::std::string pa,
                    const UnaryInfo &info, int base_index = 0);
  void encodeUntil(AutomataSystem &s, const ::std::string pa,
                   const BinaryInfo &info, int base_index = 0);
  void encodeSince(AutomataSystem &s, const ::std::string pa,
                   const BinaryInfo &info, int base_index = 0);
  void encodePast(AutomataSystem &s, const ::std::string pa,
                  const UnaryInfo &info, int base_index = 0);
  DirectEncoder(AutomataSystem &s, const ::std::vector<PlanAction> &plan,
                const int base_pos = 0);
  AutomataSystem createFinalSystem(const AutomataSystem &s,
                                   SystemVisInfo &s_vis);
  DirectEncoder mergeEncodings(const DirectEncoder &enc2) const;
};
} // end namespace taptenc
