#include "direct_encoder.h"
#include "constants.h"
#include "enc_interconnection_info.h"
#include "encoder_utils.h"
#include "filter.h"
#include "timed_automata.h"
#include "utils.h"
#include "vis_info.h"
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#define CONTEXT 2

using namespace taptenc;
using namespace encoderutils;

void DirectEncoder::generateBaseTimeLine(AutomataSystem &s,
                                         const int base_index,
                                         const int plan_index) {
  std::cout << "DirectEncoder generateBaseTimeLine: Assuming plan automaton "
               "states are sorted by plan order!"
            << std::endl;
  Filter base_filter = Filter(s.instances[base_index].first.states);
  for (const auto &pa : s.instances[plan_index].first.states) {
    TimeLine tl;
    std::string ta_prefix = toPrefix("", "", pa.id);
    Automaton ta_copy = base_filter.filterAutomaton(
        s.instances[base_index].first, ta_prefix, "", false);
    if (pa.initial) {
      auto search = std::find_if(ta_copy.states.begin(), ta_copy.states.end(),
                                 [](const State &s) bool { return s.initial; });
      if (search != ta_copy.states.end()) {
        search->initial = true;
        std::cout << "DirectEncoder generateBaseTimeLine: Set initial state: "
                  << search->id << std::endl;
      }
    } else {
      for (auto &s : ta_copy.states) {
        s.initial = false;
      }
    }
    ta_copy.clocks.insert(ta_copy.clocks.end(),
                          s.instances[plan_index].first.clocks.begin(),
                          s.instances[plan_index].first.clocks.end());
    auto emp_ta = tl.emplace(
        std::make_pair(ta_prefix, TlEntry(ta_copy, std::vector<Transition>())));
    if (emp_ta.second == true) {
      addInvariants(emp_ta.first->second.ta, emp_ta.first->second.ta.states,
                    pa.inv);
    } else {
      std::cout << "DirectEncoder generateBaseTimeLine: plan automaton has non "
                   "unique id (id "
                << pa.id << ")" << std::endl;
    }
    po_tls.pa_order.get()->push_back(pa.id);
    auto emp_tl = po_tls.tls->emplace(std::make_pair(pa.id, tl));
    if (emp_tl.second == false) {
      std::cout << "DirectEncoder generateBaseTimeLine: plan action timeline "
                   "already present (pa "
                << pa.id << ")" << std::endl;
    }
  }
  // generate query state
  TimeLine query_tl;
  Automaton query_ta(std::vector<State>(), std::vector<Transition>(),
                     constants::QUERY, false);
  query_ta.states.push_back(State(constants::QUERY, ""));
  query_tl.emplace(std::make_pair(
      constants::QUERY, TlEntry(query_ta, std::vector<Transition>())));
  auto emp_tl = po_tls.tls->emplace(std::make_pair(constants::QUERY, query_tl));
  if (emp_tl.second == false) {
    std::cout << "DirectEncoder generateBaseTimeLine: query timeline "
                 "already present (no plan action can be named query) "
              << std::endl;
  }
  // make transitions from last plan actions to query
  for (auto &last_tl :
       po_tls.tls->find(s.instances[plan_index].first.states.back().id)
           ->second) {
    for (auto &s : last_tl.second.ta.states) {
      last_tl.second.trans_out.push_back(
          Transition(s.id, constants::QUERY, "", "", "", ""));
    }
  }
  // generate connections between TLs according to plan TA transitions
  for (const auto &pa_trans : s.instances[plan_index].first.transitions) {
    auto source_ta = po_tls.tls->find(pa_trans.source_id);
    auto dest_ta = po_tls.tls->find(pa_trans.dest_id);
    if (source_ta != po_tls.tls->end() && dest_ta != po_tls.tls->end()) {
      std::string source_ta_prefix = toPrefix("", "", pa_trans.source_id);
      std::string dest_ta_prefix = toPrefix("", "", pa_trans.dest_id);
      std::vector<Transition> succ_trans = createSuccessorTransitionsBetweenTAs(
          s.instances[base_index].first,
          source_ta->second.find(source_ta_prefix)->second.ta,
          dest_ta->second.find(dest_ta_prefix)->second.ta,
          source_ta->second.find(source_ta_prefix)->second.ta.states,
          pa_trans.guard, pa_trans.update);
      std::vector<Transition> copy_trans = createCopyTransitionsBetweenTAs(
          source_ta->second.find(source_ta_prefix)->second.ta,
          dest_ta->second.find(dest_ta_prefix)->second.ta,
          source_ta->second.find(source_ta_prefix)->second.ta.states,
          pa_trans.guard, pa_trans.update, "");
      po_tls.tls.get()
          ->find(pa_trans.source_id)
          ->second.find(source_ta_prefix)
          ->second.trans_out.insert(po_tls.tls.get()
                                        ->find(pa_trans.source_id)
                                        ->second.find(source_ta_prefix)
                                        ->second.trans_out.end(),
                                    copy_trans.begin(), copy_trans.end());
      po_tls.tls.get()
          ->find(pa_trans.source_id)
          ->second.find(source_ta_prefix)
          ->second.trans_out.insert(po_tls.tls.get()
                                        ->find(pa_trans.source_id)
                                        ->second.find(source_ta_prefix)
                                        ->second.trans_out.end(),
                                    succ_trans.begin(), succ_trans.end());
    } else {
      std::cout << "DirectEncoder generateBaseTimeLine: pa "
                << pa_trans.source_id << " or " << pa_trans.dest_id
                << " has no timeline yet" << std::endl;
    }
  }
}

std::pair<int, int>
DirectEncoder::calculateContext(const TargetSpecs &specs,
                                std::string starting_pa, std::string ending_pa,
                                bool look_ahead, int lb_offset, int ub_offset) {
  if (look_ahead) {
    // start index needs to subtract one because of start action
    // indices w.r.t. to the plan, not the plan automaton
    int start_index =
        stoi(Filter::getSuffix(starting_pa, constants::PA_SEP)) - 1;
    // if an end_index is specified this means the begin of the PA ends the
    // context, hence we have to subtract 1 for the start action and 1 to
    // exclude the ending pa itself
    int end_index =
        (ending_pa == "")
            ? plan.size()
            : stoi(Filter::getSuffix(ending_pa, constants::PA_SEP)) - 2;
    if (ending_pa == "" && lb_offset == 0) {
      lb_offset = specs.bounds.lower_bound;
    }
    int offset_index = start_index;
    if ((long unsigned int)start_index >= plan.size()) {
      std::cout << "DirectEncoder calculateContext: starting pa " << starting_pa
                << " is out of range" << std::endl;
      return std::make_pair(0, 0);
    }
    int lb_acc = 0;
    int ub_acc = 0;
    for (auto pa = plan.begin() + start_index; pa != plan.end(); ++pa) {
      lb_acc += pa->duration.lower_bound;
      if (ub_acc != std::numeric_limits<int>::max()) {
        // Increase ub_acc only if it does not overflow
        ub_acc = safeAddition(pa->duration.upper_bound, ub_acc);
      }
      if (ub_acc < lb_offset) {
        offset_index++;
      }
      if (lb_acc >= safeAddition(specs.bounds.upper_bound, ub_offset) ||
          pa - plan.begin() == end_index) {
        return std::make_pair(offset_index + 1,
                              pa - plan.begin() - offset_index);
      }
      if (pa - plan.begin() == end_index) {
        break;
      }
    }
    // res needs to add one because of fin action
    return std::make_pair(offset_index + 1, end_index - offset_index);
  } else {
    // rstart index is the offset from the end of the plan to the activated
    // action plus one (because the context starts before the activation action
    // indices w.r.t. to the plan, not the plan automaton, e.g. the position of
    // the starting pa is the suffix -1
    int rstart_index = plan.size() -
                       stoi(Filter::getSuffix(starting_pa, constants::PA_SEP)) +
                       1;
    // if an end_index is specified this means the begin of the PA ends the
    // context, hence we have to subtract 1 for the start action and 1 to
    // exclude the ending pa itself
    int rend_index =
        (ending_pa == "")
            ? plan.size()
            : plan.size() -
                  (stoi(Filter::getSuffix(ending_pa, constants::PA_SEP)) - 2);
    if (ending_pa == "" && lb_offset == 0) {
      lb_offset = specs.bounds.lower_bound;
    }
    int roffset_index = rstart_index;
    int lb_acc = 0;
    int ub_acc = 0;
    for (auto pa = plan.rbegin() + rstart_index; pa != plan.rend(); ++pa) {
      lb_acc += pa->duration.lower_bound;
      if (ub_acc != std::numeric_limits<int>::max()) {
        // Increase ub_acc only if it does not overflow
        ub_acc = safeAddition(pa->duration.upper_bound, ub_acc);
      }
      if (ub_acc < lb_offset) {
        roffset_index++;
      }
      if (lb_acc >= safeAddition(specs.bounds.upper_bound, ub_offset) ||
          pa - plan.rbegin() == rend_index) {
        return std::make_pair(plan.size() - roffset_index,
                              roffset_index - (pa - plan.rbegin()));
      }
      if (pa - plan.rbegin() == rend_index) {
        break;
      }
    }
    // res needs to add one because of fin action
    return std::make_pair(plan.size() - roffset_index,
                          roffset_index - rend_index);
  }
}

void DirectEncoder::encodeInvariant(AutomataSystem &,
                                    const std::vector<State> &targets,
                                    const std::string pa) {
  Filter target_filter = Filter(targets);
  auto search_tl = po_tls.tls->find(pa);
  if (search_tl != po_tls.tls->end()) {
    auto search_pa = std::find(po_tls.pa_order.get()->begin(),
                               po_tls.pa_order.get()->end(), pa);
    if (search_pa == po_tls.pa_order.get()->end()) {
      std::cout << "DirectEncoder encodeInvariant: could not find pa " << pa
                << std::endl;
      return;
    }
    // restrict transitions from prev tl to target states
    if (search_pa - po_tls.pa_order.get()->begin() > 0) {
      for (auto &prev_tl_entry :
           po_tls.tls.get()
               ->find(po_tls.pa_order.get()->at(
                   search_pa - po_tls.pa_order.get()->begin() - 1))
               ->second) {
        target_filter.filterTransitionsInPlace(prev_tl_entry.second.trans_out,
                                               pa, false);
      }
    }
    for (auto &tl_entry : search_tl->second) {
      target_filter.filterAutomatonInPlace(tl_entry.second.ta, "");
      target_filter.filterTransitionsInPlace(tl_entry.second.trans_out, pa,
                                             true);
    }
  } else {
    std::cout
        << " DirectEncoder encodeInvariant: could not find timeline of pa "
        << pa << std::endl;
  }
}

void DirectEncoder::encodeNoOp(AutomataSystem &,
                               const std::vector<State> &targets,
                               const std::string pa) {
  Filter target_filter = Filter(targets);
  auto search_tl = po_tls.tls->find(pa);
  if (search_tl != po_tls.tls->end()) {
    auto search_pa = std::find(po_tls.pa_order.get()->begin(),
                               po_tls.pa_order.get()->end(), pa);
    if (search_pa == po_tls.pa_order.get()->end()) {
      std::cout << "DirectEncoder encodeNoOp: could not find pa " << pa
                << std::endl;
      return;
    }
    // restrict transitions from prev tl to target states
    if (search_pa - po_tls.pa_order.get()->begin() > 0) {
      for (auto &prev_tl_entry :
           po_tls.tls.get()
               ->find(po_tls.pa_order.get()->at(
                   search_pa - po_tls.pa_order.get()->begin() - 1))
               ->second) {
        target_filter.filterTransitionsInPlace(prev_tl_entry.second.trans_out,
                                               pa, false);
      }
    }
  } else {
    std::cout << "DirectEncoder encodeNoOp: could not find timeline of pa "
              << pa << std::endl;
  }
}

void DirectEncoder::encodeUntilChain(AutomataSystem &s, const ChainInfo &info,
                                     const std::string start_pa,
                                     const std::string end_pa,
                                     const int base_index) {
  if (info.specs_list.size() == 0) {
    std::cout << "DirectEncoder enodeUntilchain: empty info, abort."
              << std::endl;
  }
  Filter base_filter = Filter(s.instances[base_index].first.states);
  std::string clock = "clX" + info.name;
  if (std::find(s.globals.clocks.begin(), s.globals.clocks.end(), clock) ==
      s.globals.clocks.end()) {
    s.globals.clocks.push_back(clock);
  }
  auto start_pa_entry = std::find(po_tls.pa_order.get()->begin(),
                                  po_tls.pa_order.get()->end(), start_pa);
  if (start_pa_entry == po_tls.pa_order.get()->end()) {
    std::cout << "DirectEncoder encodeUntilChain: could not find start pa "
              << start_pa << std::endl;
    return;
  }
  auto end_pa_entry =
      std::find(start_pa_entry, po_tls.pa_order.get()->end(), end_pa);
  if (end_pa_entry == po_tls.pa_order.get()->end()) {
    std::cout << "DirectEncoder encodeUntilChain: could not find end pa "
              << end_pa << std::endl;
    return;
  }
  int lb_acc = 0;
  int ub_acc = 0;
  // TLs before encoding the Until Chain
  PlanOrderedTLs orig_tls;
  // TLs of the current window
  PlanOrderedTLs curr_window;
  // TL of the previous window
  PlanOrderedTLs prev_window;
  for (const auto &tl : *(po_tls.tls.get())) {
    orig_tls.tls.get()->emplace(tl);
    curr_window.tls.get()->emplace(tl);
  }
  for (const auto &pa : *(po_tls.pa_order.get())) {
    orig_tls.pa_order.get()->push_back(pa);
    curr_window.pa_order.get()->push_back(pa);
    prev_window.pa_order.get()->push_back(pa);
  }

  // maps to obtain the original tl entry id given the prefix of
  // current/previous window prefix id
  OrigMap prev_to_orig;
  OrigMap curr_to_orig;
  // delete all tls form the until chain
  for (auto window_pa = start_pa_entry;
       window_pa - po_tls.pa_order.get()->begin() <
       end_pa_entry - po_tls.pa_order.get()->begin();
       ++window_pa) {
    po_tls.tls.get()->find(*window_pa)->second.clear();
  }
  if (end_pa == constants::END_PA) {
    po_tls.tls.get()->find(*end_pa_entry)->second.clear();
  }
  std::string prev_window_guard_constraint_sat = "";
  std::string guard_constraint_sat = "";
  // init curr_to_orig
  for (const auto &pa_tl : *(orig_tls.tls.get())) {
    for (const auto &pa_entry : pa_tl.second) {
      curr_to_orig[pa_entry.first] = pa_entry.first;
    }
  }
  // encode the until chain from left to right
  for (auto specs = info.specs_list.begin(); specs != info.specs_list.end();
       ++specs) {
    // init round by backing up data from the previous round
    prev_window.tls = std::move(curr_window.tls);
    prev_window_guard_constraint_sat = guard_constraint_sat;
    prev_to_orig = curr_to_orig;
    curr_to_orig.clear();
    // curr_window.tls->clear();
    // determine context (window begin and end)
    std::pair<int, int> context =
        calculateContext(*specs, start_pa, end_pa, true, lb_acc, ub_acc);
    lb_acc += specs->bounds.lower_bound;
    ub_acc = safeAddition(ub_acc, specs->bounds.upper_bound);
    std::size_t context_start = context.first;
    std::size_t context_end = context.first + context.second;
    std::string context_pa_start =
        *(po_tls.pa_order.get()->begin() + context_start);
    std::string context_pa_end =
        *(po_tls.pa_order.get()->begin() + context_end);
    // formulate constraints based on the given bounds
    bool upper_bounded =
        (specs->bounds.upper_bound != std::numeric_limits<int>::max());
    bool lower_bounded =
        (specs->bounds.lower_bound != 0 || specs->bounds.l_op != "&lt;=");
    std::string guard_upper_bound_crossed =
        clock + inverse_op(specs->bounds.r_op) +
        std::to_string(specs->bounds.upper_bound);
    guard_constraint_sat =
        (lower_bounded ? clock + reverse_op(specs->bounds.l_op) +
                             std::to_string(specs->bounds.lower_bound)
                       : "") +
        ((lower_bounded && upper_bounded) ? "&amp;&amp;" : "") +
        (upper_bounded ? clock + specs->bounds.r_op +
                             std::to_string(specs->bounds.upper_bound)
                       : "");
    std::string op_name = info.name + "F" + std::to_string(encode_counter);
    encode_counter++;
    Filter target_filter(specs->targets);
    curr_window = orig_tls.createWindow(context_pa_start, context_pa_end,
                                        target_filter, op_name);
    if (upper_bounded) {
      curr_window.addStateInvariantToWindow(
          context_pa_start, context_pa_end,
          clock + specs->bounds.r_op +
              std::to_string(specs->bounds.upper_bound));
    }
    if (specs == info.specs_list.begin()) {
      // connect TL before start pa to the first window
      if (context_start > 0) {
        std::string prev_pa =
            *(po_tls.pa_order.get()->begin() + context_start - 1);
        for (auto &prev_pa_entry : po_tls.tls.get()->find(prev_pa)->second) {
          PlanOrderedTLs::modifyTransitionsToNextTl(
              prev_pa_entry.second.trans_out, prev_pa_entry.first,
              target_filter.getFilter(), "", clock + " = 0", "", op_name);
        }
      }
    }
    OrigMap to_orig;
    curr_to_orig = orig_tls.createOrigMapping(op_name);
    to_orig.insert(prev_to_orig.begin(), prev_to_orig.end());
    to_orig.insert(curr_to_orig.begin(), curr_to_orig.end());

    prev_window.createTransitionsToWindow(
        s.instances[base_index].first, *(curr_window.tls.get()), to_orig,
        context_pa_start, context_pa_end, base_filter,
        prev_window_guard_constraint_sat, clock + " = 0");
    // add transitions back to original TLs
    if (specs + 1 == info.specs_list.end()) {
      std::string last_pa = *(po_tls.pa_order.get()->begin() + context_end);
      PlanOrderedTLs::addOutgoingTransOfOrigTL(
          orig_tls.tls.get()->find(last_pa)->second,
          curr_window.tls.get()->find(last_pa)->second, curr_to_orig,
          guard_constraint_sat);
    }
    // merge a window once it is done (this is, when outgoing transitions to the
    // next window are added)
    if (specs != info.specs_list.begin()) {
      po_tls.mergeWindow(*(prev_window.tls.get()), true);
    }
  }
  // merge the last window
  po_tls.mergeWindow(*(curr_window.tls.get()), true);
}

void DirectEncoder::encodeFuture(AutomataSystem &s, const std::string pa,
                                 const UnaryInfo &info, int base_index) {
  Filter base_filter = Filter(s.instances[base_index].first.states);
  auto start_pa_entry = std::find(po_tls.pa_order.get()->begin(),
                                  po_tls.pa_order.get()->end(), pa);
  if (start_pa_entry == po_tls.pa_order.get()->end()) {
    std::cout << "DirectEncoder encodeFuture: could not find start pa " << pa
              << std::endl;
    return;
  }
  // TLs of the current window
  PlanOrderedTLs curr_window;
  for (const auto &tl : *(po_tls.tls.get())) {
    curr_window.tls.get()->emplace(tl);
  }
  for (const auto &pa : *(po_tls.pa_order.get())) {
    curr_window.pa_order.get()->push_back(pa);
  }
  // maps to obtain the original tl entry id given the prefix of
  // new window prefix id
  std::string guard_constraint_sat = "";
  std::string clock = "clX" + info.name;
  if (std::find(s.globals.clocks.begin(), s.globals.clocks.end(), clock) ==
      s.globals.clocks.end()) {
    s.globals.clocks.push_back(clock);
  }
  // determine context (window begin and end)
  std::pair<int, int> context = calculateContext(info.specs, pa, "");
  std::size_t context_start = context.first;
  std::size_t constraint_start =
      start_pa_entry - po_tls.pa_order.get()->begin();
  std::size_t context_end = context.first + context.second;
  std::string context_pa_start =
      *(po_tls.pa_order.get()->begin() + context_start);
  std::string context_pa_end = *(po_tls.pa_order.get()->begin() + context_end);
  // formulate constraints based on the given bounds
  bool upper_bounded =
      (info.specs.bounds.upper_bound != std::numeric_limits<int>::max());
  bool lower_bounded =
      (info.specs.bounds.lower_bound != 0 || info.specs.bounds.l_op != "&lt;=");
  std::string guard_upper_bound_crossed =
      clock + inverse_op(info.specs.bounds.r_op) +
      std::to_string(info.specs.bounds.upper_bound);
  guard_constraint_sat =
      (lower_bounded ? clock + reverse_op(info.specs.bounds.l_op) +
                           std::to_string(info.specs.bounds.lower_bound)
                     : "") +
      ((lower_bounded && upper_bounded) ? "&amp;&amp;" : "") +
      (upper_bounded ? clock + info.specs.bounds.r_op +
                           std::to_string(info.specs.bounds.upper_bound)
                     : "");
  std::string op_name = info.name + "F" + std::to_string(encode_counter);
  Filter target_filter(info.specs.targets);
  curr_window = po_tls.createWindow(context_pa_start, context_pa_end,
                                    base_filter, op_name);
  // creset clock upon entering context range
  if (constraint_start > 0) {
    std::string prev_pa =
        *(po_tls.pa_order.get()->begin() + constraint_start - 1);
    for (auto &prev_pa_entry : po_tls.tls.get()->find(prev_pa)->second) {
      PlanOrderedTLs::modifyTransitionsToNextTl(
          prev_pa_entry.second.trans_out, prev_pa_entry.first,
          target_filter.getFilter(), "", clock + " = 0", "");
    }
  }
  OrigMap orig_id = po_tls.createOrigMapping("");
  OrigMap to_orig = po_tls.createOrigMapping(op_name);
  to_orig.insert(orig_id.begin(), orig_id.end());
  if (upper_bounded) {
    po_tls.addStateInvariantToWindow(
        context_pa_start, context_pa_end,
        clock + info.specs.bounds.r_op +
            std::to_string(info.specs.bounds.upper_bound));
  }
  po_tls.createTransitionsToWindow(s.instances[base_index].first,
                                   *(curr_window.tls.get()), to_orig,
                                   context_pa_start, context_pa_end,
                                   target_filter, guard_constraint_sat, "");
  std::string last_pa = *(po_tls.pa_order.get()->begin() + context_end);
  PlanOrderedTLs::addOutgoingTransOfOrigTL(
      po_tls.tls.get()->find(last_pa)->second,
      curr_window.tls.get()->find(last_pa)->second, to_orig, "");
  for (auto &tl_entry : po_tls.tls.get()->find(last_pa)->second) {
    PlanOrderedTLs::removeTransitionsToNextTl(tl_entry.second.trans_out,
                                              last_pa);
  }
  po_tls.mergeWindow(*(curr_window.tls.get()), true);
}

void DirectEncoder::encodeUntil(AutomataSystem &s, const std::string pa,
                                const BinaryInfo &info, int base_index) {
  auto start_pa_entry = std::find(po_tls.pa_order.get()->begin(),
                                  po_tls.pa_order.get()->end(), pa);
  if (start_pa_entry == po_tls.pa_order.get()->end()) {
    std::cout << "DirectEncoder encodeUntil: could not find start pa " << pa
              << std::endl;
    return;
  }
  OrigMap to_orig = po_tls.createOrigMapping("");
  Filter target_filter(info.specs.targets);
  Filter pre_target_filter(info.pre_targets);
  // determine context (window begin and end)
  std::pair<int, int> context = calculateContext(info.specs, pa, "", true);
  std::size_t context_end = context.first + context.second;
  std::size_t constraint_start =
      start_pa_entry - po_tls.pa_order.get()->begin();
  encodeFuture(s, pa, info.toUnary(), base_index);
  for (size_t i = constraint_start; i <= context_end; i++) {
    auto pa_tl = po_tls.tls->find(*(po_tls.pa_order.get()->begin() + i));
    if (pa_tl != po_tls.tls->end()) {
      for (auto &tl_entry : pa_tl->second) {
        if (to_orig[tl_entry.first] != "") {
          pre_target_filter.filterAutomatonInPlace(tl_entry.second.ta, "");
          pre_target_filter.filterTransitionsInPlace(tl_entry.second.trans_out,
                                                     "", true);
        }
      }
    }
  }
  if (constraint_start > 0) {
    std::string prev_pa =
        *(po_tls.pa_order.get()->begin() + constraint_start - 1);
    auto prev_tl = po_tls.tls->find(prev_pa);
    if (prev_tl != po_tls.tls->end()) {
      for (auto &prev_entry : prev_tl->second) {
        pre_target_filter.filterTransitionsInPlace(prev_entry.second.trans_out,
                                                   pa, false);
      }
    } else {
      std::cout << "DirectEncoder encodeUntil: cannot find prev_pa TLs. "
                << prev_pa << std::endl;
    }
  }
}

void DirectEncoder::encodePast(AutomataSystem &s, const std::string pa,
                               const UnaryInfo &info, int base_index) {
  Filter base_filter = Filter(s.instances[base_index].first.states);
  auto start_pa_entry = std::find(po_tls.pa_order.get()->begin(),
                                  po_tls.pa_order.get()->end(), pa);
  if (start_pa_entry == po_tls.pa_order.get()->end()) {
    std::cout << "DirectEncoder encodePast: could not find start pa " << pa
              << std::endl;
    return;
  }
  // TLs of the current window
  PlanOrderedTLs curr_window;
  for (const auto &tl : *(po_tls.tls.get())) {
    curr_window.tls.get()->emplace(tl);
  }
  for (const auto &pa : *(po_tls.pa_order.get())) {
    curr_window.pa_order.get()->push_back(pa);
  }
  // maps to obtain the original tl entry id given the prefix of
  // new window prefix id
  std::string guard_constraint_sat = "";
  std::string clock = "clX" + info.name;
  if (std::find(s.globals.clocks.begin(), s.globals.clocks.end(), clock) ==
      s.globals.clocks.end()) {
    s.globals.clocks.push_back(clock);
  }
  // determine context (window begin and end)
  std::pair<int, int> context = calculateContext(info.specs, pa, "", false);
  std::size_t context_end = context.first;
  std::size_t constraint_end =
      start_pa_entry - po_tls.pa_order.get()->begin() - 1;
  std::size_t context_start = context.first + context.second;
  std::string context_pa_start =
      *(po_tls.pa_order.get()->begin() + context_start);
  std::string context_pa_end = *(po_tls.pa_order.get()->begin() + context_end);
  std::string constraint_end_pa =
      *(po_tls.pa_order.get()->begin() + constraint_end);
  // formulate constraints based on the given bounds
  bool upper_bounded =
      (info.specs.bounds.upper_bound != std::numeric_limits<int>::max());
  bool lower_bounded =
      (info.specs.bounds.lower_bound != 0 || info.specs.bounds.l_op != "&lt;=");
  std::string guard_upper_bound_crossed =
      clock + inverse_op(info.specs.bounds.r_op) +
      std::to_string(info.specs.bounds.upper_bound);
  guard_constraint_sat =
      (lower_bounded ? clock + reverse_op(info.specs.bounds.l_op) +
                           std::to_string(info.specs.bounds.lower_bound)
                     : "") +
      ((lower_bounded && upper_bounded) ? "&amp;&amp;" : "") +
      (upper_bounded ? clock + info.specs.bounds.r_op +
                           std::to_string(info.specs.bounds.upper_bound)
                     : "");
  std::string op_name = info.name + "F" + std::to_string(encode_counter);
  Filter target_filter(info.specs.targets);
  curr_window = po_tls.createWindow(context_pa_start, context_pa_end,
                                    base_filter, op_name);
  OrigMap orig_id = po_tls.createOrigMapping("");
  OrigMap to_orig = po_tls.createOrigMapping(op_name);
  to_orig.insert(orig_id.begin(), orig_id.end());
  if (upper_bounded) {
    if (context_end < constraint_end) {
      std::string past_context_pa =
          *(po_tls.pa_order.get()->begin() + context_end + 1);
      po_tls.addStateInvariantToWindow(
          past_context_pa, constraint_end_pa,
          clock + info.specs.bounds.r_op +
              std::to_string(info.specs.bounds.upper_bound));
    }
  }
  po_tls.createTransitionsToWindow(
      s.instances[base_index].first, *(curr_window.tls.get()), to_orig,
      context_pa_start, context_pa_end, target_filter, "", clock + " = 0");
  std::string last_pa = *(po_tls.pa_order.get()->begin() + context_end);
  PlanOrderedTLs::addOutgoingTransOfOrigTL(
      po_tls.tls.get()->find(last_pa)->second,
      curr_window.tls.get()->find(last_pa)->second, to_orig, "");
  for (auto &tl_entry : po_tls.tls.get()->find(last_pa)->second) {
    PlanOrderedTLs::removeTransitionsToNextTl(tl_entry.second.trans_out,
                                              last_pa);
  }
  if (lower_bounded) {
    TimeLine *last_tl;
    if (context_end < constraint_end) {
      last_tl = &(po_tls.tls.get()->find(constraint_end_pa)->second);
    } else {
      last_tl = &(curr_window.tls.get()->find(constraint_end_pa)->second);
    }
    for (auto &last_entry : *last_tl) {
      PlanOrderedTLs::modifyTransitionsToNextTl(
          last_entry.second.trans_out, constraint_end_pa,
          target_filter.getFilter(), guard_constraint_sat, "", "");
    }
  }
  po_tls.mergeWindow(*(curr_window.tls.get()), true);
}

void DirectEncoder::encodeSince(AutomataSystem &s, const std::string pa,
                                const BinaryInfo &info, int base_index) {
  auto start_pa_entry = std::find(po_tls.pa_order.get()->begin(),
                                  po_tls.pa_order.get()->end(), pa);
  if (start_pa_entry == po_tls.pa_order.get()->end()) {
    std::cout << "DirectEncoder encodeSince: could not find start pa " << pa
              << std::endl;
    return;
  }
  OrigMap to_orig = po_tls.createOrigMapping("");
  Filter target_filter(info.specs.targets);
  Filter pre_target_filter(info.pre_targets);
  // determine context (window begin and end)
  std::pair<int, int> context = calculateContext(info.specs, pa, "", false);
  std::size_t context_start = context.first + context.second;
  std::size_t context_end = context.first;
  std::size_t constraint_end =
      start_pa_entry - po_tls.pa_order.get()->begin() - 1;
  encodePast(s, pa, info.toUnary(), base_index);
  // Ensure the TA stays in the pre_target state until activation pa is reached
  for (size_t i = context_start; i <= constraint_end; i++) {
    auto pa_tl = po_tls.tls->find(*(po_tls.pa_order.get()->begin() + i));
    if (pa_tl != po_tls.tls->end()) {
      for (auto &tl_entry : pa_tl->second) {
        // inside the context window the past operator adds a new tl
        // representing that the target states were reached, this is when we
        // should stay in the pre_target states
        if (i <= context_end) {
          if (to_orig.find(tl_entry.first) == to_orig.end()) {
            pre_target_filter.filterAutomatonInPlace(tl_entry.second.ta, "");
            pre_target_filter.filterTransitionsInPlace(
                tl_entry.second.trans_out, "", true);
          } else {
            // we have to delete transitions leading to states that we deleted
            // above
            tl_entry.second.trans_out.erase(
                std::remove_if(
                    tl_entry.second.trans_out.begin(),
                    tl_entry.second.trans_out.end(),
                    [to_orig, pre_target_filter,
                     this](const Transition &t) bool {
                      std::string prefix =
                          Filter::getPrefix(t.dest_id, constants::BASE_SEP);
                      prefix.push_back(constants::BASE_SEP);
                      return (to_orig.find(prefix) == to_orig.end()) &&
                             !pre_target_filter.matchesId(t.dest_id);
                    }),
                tl_entry.second.trans_out.end());
          }
          // after the context window but before the pa begin  we also have to
          // remain in the pre_target states
        } else if (to_orig.find(tl_entry.first) != to_orig.end() &&
                   i > context_end) {
          pre_target_filter.filterAutomatonInPlace(tl_entry.second.ta, "");
          pre_target_filter.filterTransitionsInPlace(tl_entry.second.trans_out,
                                                     "", true);
        }
      }
    }
  }
  if (context_start > 0) {
    std::string prev_pa = *(po_tls.pa_order.get()->begin() + context_start - 1);
    auto prev_tl = po_tls.tls->find(prev_pa);
    if (prev_tl != po_tls.tls->end()) {
      for (auto &prev_entry : prev_tl->second) {
        pre_target_filter.filterTransitionsInPlace(prev_entry.second.trans_out,
                                                   pa, false);
      }
    } else {
      std::cout << "DirectEncoder encodeSince: cannot find prev_pa TLs. "
                << prev_pa << std::endl;
    }
  }
}

size_t DirectEncoder::getPlanTAIndex() { return plan_ta_index; }

AutomataSystem DirectEncoder::createFinalSystem(const AutomataSystem &s,
                                                SystemVisInfo &s_vis) {
  s_vis = SystemVisInfo(*(po_tls.tls.get()), *(po_tls.pa_order.get()));
  AutomataSystem res = s;
  res.instances.clear();
  std::vector<Automaton> automata;
  std::vector<Transition> interconnections;
  for (const auto &curr_tl : *(po_tls.tls.get())) {
    for (const auto &curr_copy : curr_tl.second) {
      automata.push_back(curr_copy.second.ta);
      interconnections.insert(interconnections.end(),
                              curr_copy.second.trans_out.begin(),
                              curr_copy.second.trans_out.end());
    }
  }
  res.instances.push_back(
      std::make_pair(mergeAutomata(automata, interconnections, "direct"), ""));
  return res;
}

DirectEncoder::DirectEncoder(AutomataSystem &s,
                             const std::vector<PlanAction> &plan,
                             const int base_pos) {
  Automaton plan_ta = generatePlanAutomaton(plan, constants::PLAN_TA_NAME);
  this->plan = plan;
  s.instances.push_back(std::make_pair(plan_ta, ""));
  plan_ta_index = s.instances.size() - 1;
  generateBaseTimeLine(s, base_pos, plan_ta_index);
}

DirectEncoder DirectEncoder::mergeEncodings(const DirectEncoder &enc2) const {
  return DirectEncoder(po_tls.mergePlanOrderedTLs(enc2.po_tls), plan,
                       plan_ta_index);
}

DirectEncoder::DirectEncoder(const PlanOrderedTLs &tls,
                             const ::std::vector<PlanAction> &plan,
                             size_t plan_ta_index)
    : plan(plan), plan_ta_index(plan_ta_index) {
  for (const auto &tl : *(tls.tls.get())) {
    po_tls.tls.get()->emplace(tl);
  }
  for (const auto &pa : *(tls.pa_order.get())) {
    po_tls.pa_order.get()->push_back(pa);
  }
}
