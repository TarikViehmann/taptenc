#include "direct_encoder.h"
#include "constants.h"
#include "encoder.h"
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

void DirectEncoder::generateBaseTimeLine(AutomataSystem &s,
                                         const int base_index,
                                         const int plan_index) {
  std::cout << "DirectEncoder generateBaseTimeLine: Assuming plan automaton "
               "states are sorted by plan order!"
            << std::endl;
  Filter base_filter = Filter(s.instances[base_index].first.states);
  for (const auto &pa : s.instances[plan_index].first.states) {
    std::cout << pa.id << std::endl;
    std::unordered_map<std::string,
                       std::pair<Automaton, std::vector<Transition>>>
        tl;
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
    auto emp_ta = tl.emplace(std::make_pair(
        ta_prefix, std::make_pair(ta_copy, std::vector<Transition>())));
    if (emp_ta.second == true) {
      addInvariants(emp_ta.first->second.first,
                    emp_ta.first->second.first.states, pa.inv);
    } else {
      std::cout << "DirectEncoder generateBaseTimeLine: plan automaton has non "
                   "unique id (id "
                << pa.id << ")" << std::endl;
    }
    pa_order.push_back(pa.id);
    auto emp_tl = pa_tls.emplace(std::make_pair(pa.id, tl));
    if (emp_tl.second == false) {
      std::cout << "DirectEncoder generateBaseTimeLine: plan action timeline "
                   "already present (pa "
                << pa.id << ")" << std::endl;
    }
  }
  // generate query state
  std::unordered_map<std::string, std::pair<Automaton, std::vector<Transition>>>
      query_tl;
  Automaton query_ta(std::vector<State>(), std::vector<Transition>(), "query",
                     false);
  query_ta.states.push_back(State("query", ""));
  query_tl.emplace(std::make_pair(
      "query", std::make_pair(query_ta, std::vector<Transition>())));
  auto emp_tl = pa_tls.emplace(std::make_pair("query", query_tl));
  if (emp_tl.second == false) {
    std::cout << "DirectEncoder generateBaseTimeLine: query timeline "
                 "already present (no plan action can be named query) "
              << std::endl;
  }
  // make transitions from last plan actions to query
  for (auto &last_tl :
       pa_tls.find(s.instances[plan_index].first.states.back().id)->second) {
    for (auto &s : last_tl.second.first.states) {
      last_tl.second.second.push_back(
          Transition(s.id, "query", "", "", "", ""));
    }
  }
  // generate connections between TLs according to plan TA transitions
  for (const auto &pa_trans : s.instances[plan_index].first.transitions) {
    auto source_ta = pa_tls.find(pa_trans.source_id);
    auto dest_ta = pa_tls.find(pa_trans.dest_id);
    if (source_ta != pa_tls.end() && dest_ta != pa_tls.end()) {
      std::string source_ta_prefix = toPrefix("", "", pa_trans.source_id);
      std::string dest_ta_prefix = toPrefix("", "", pa_trans.dest_id);
      std::vector<Transition> successor_trans = createCopyTransitionsBetweenTAs(
          source_ta->second.find(source_ta_prefix)->second.first,
          dest_ta->second.find(dest_ta_prefix)->second.first,
          source_ta->second.find(source_ta_prefix)->second.first.states,
          pa_trans.guard, pa_trans.update, "");
      pa_tls[pa_trans.source_id]
          .find(source_ta_prefix)
          ->second.second.insert(pa_tls[pa_trans.source_id]
                                     .find(source_ta_prefix)
                                     ->second.second.end(),
                                 successor_trans.begin(),
                                 successor_trans.end());
    } else {
      std::cout << "DirectEncoder generateBaseTimeLine: pa "
                << pa_trans.source_id << " or " << pa_trans.dest_id
                << " has no timeline yet" << std::endl;
    }
  }
}

std::vector<Transition>
DirectEncoder::createTransitionsBackToOrigTL(std::vector<Transition> &trans,
                                             std::string prefix, std::string pa,
                                             std::string guard) {
  std::vector<Transition> res;
  for (const auto &tr : trans) {
    if (pa + Filter::getSuffix(tr.source_id, constants::BASE_SEP) ==
            tr.source_id &&
        pa + Filter::getSuffix(tr.dest_id, constants::BASE_SEP) != pa) {
      res.push_back(Transition(
          prefix + Filter::getSuffix(tr.source_id, constants::BASE_SEP),
          tr.dest_id, tr.action, addConstraint(tr.guard, guard), tr.update,
          tr.sync));
    }
  }
  return res;
}

std::vector<Transition>
DirectEncoder::addToPrefixOnTransitions(const std::vector<Transition> &trans,
                                        std::string to_add,
                                        bool only_inner_trans) {
  std::vector<Transition> res;
  for (const auto &tr : trans) {
    if (only_inner_trans == false ||
        (Filter::getPrefix(tr.source_id, constants::TL_SEP) ==
         Filter::getPrefix(tr.dest_id, constants::TL_SEP))) {
      res.push_back(Transition(addToPrefix(tr.source_id, to_add),
                               addToPrefix(tr.dest_id, to_add), tr.action,
                               tr.guard, tr.update, tr.sync));
    }
  }
  return res;
}

void DirectEncoder::modifyTransitionsToNextTl(std::vector<Transition> &trans,
                                              std::string curr_pa,
                                              std::string guard,
                                              std::string update,
                                              std::string sync) {
  for (auto &t : trans) {
    if (Filter::getPrefix(t.dest_id, constants::TL_SEP) != curr_pa) {
      t.guard = addConstraint(t.guard, guard);
      t.update = addUpdate(t.update, update);
      if (sync != "")
        t.sync = sync;
    }
  }
}

void DirectEncoder::removeTransitionsToNextTl(std::vector<Transition> &trans,
                                              std::string curr_pa) {
  trans.erase(std::remove_if(trans.begin(), trans.end(),
                             [curr_pa](Transition &t) bool {
                               return Filter::getPrefix(t.dest_id,
                                                        constants::TL_SEP) !=
                                      curr_pa;
                             }),
              trans.end());
}

std::pair<int, int> DirectEncoder::calculateContext(const EncICInfo &info,
                                                    std::string starting_pa,
                                                    std::string ending_pa,
                                                    int offset) {
  // start index needs to subtract one because of start action
  int start_index = stoi(Filter::getSuffix(starting_pa, constants::PA_SEP)) - 1;
  int end_index =
      (ending_pa == "")
          ? plan.size() + 2
          : stoi(Filter::getSuffix(ending_pa, constants::PA_SEP)) - 1;
  int offset_index = start_index;
  if ((long unsigned int)start_index >= plan.size()) {
    std::cout << "DirectEncoder calculateContext: starting pa " << starting_pa
              << " is out of range" << std::endl;
    return std::make_pair(0, 0);
  }
  if (info.type == ICType::Future) {
    int lb_acc = 0;
    int ub_acc = 0;
    for (auto pa = plan.begin() + start_index; pa != plan.end(); ++pa) {
      lb_acc += pa->duration.lower_bound;
      if (ub_acc != std::numeric_limits<int>::max()) {
        // Increase ub_acc only if it does not overflow
        ub_acc = (pa->duration.upper_bound <
                  std::numeric_limits<int>::max() - ub_acc)
                     ? pa->duration.upper_bound + ub_acc
                     : std::numeric_limits<int>::max();
      }
      if (ub_acc < offset) {
        offset_index++;
      }
      if (lb_acc >= info.bounds.upper_bound + offset ||
          pa - plan.begin() == end_index) {
        return std::make_pair(offset_index, pa - plan.begin() - start_index);
      }
    }
    // res needs to add one because of fin action
    return std::make_pair(offset_index, plan.size() - start_index + 1);
  } else {
    std::cout << "DirectEncoder calculateContext: unsopported type "
              << info.type << std::endl;
    return std::make_pair(0, 0);
  }
}

void DirectEncoder::encodeInvariant(AutomataSystem &,
                                    const std::vector<State> &targets,
                                    const std::string pa) {
  Filter target_filter = Filter(targets);
  auto search_tl = pa_tls.find(pa);
  if (search_tl != pa_tls.end()) {
    auto search_pa = std::find(pa_order.begin(), pa_order.end(), pa);
    if (search_pa == pa_order.end()) {
      std::cout << "DirectEncoder encodeInvariant: could not find pa " << pa
                << std::endl;
      return;
    }
    // restrict transitions from prev tl to target states
    if (search_pa - pa_order.begin() > 0) {
      for (auto &prev_tl_entry :
           pa_tls[pa_order[search_pa - pa_order.begin() - 1]]) {
        target_filter.filterTransitionsInPlace(prev_tl_entry.second.second, pa,
                                               false);
      }
    }
    for (auto &tl_entry : search_tl->second) {
      target_filter.filterAutomatonInPlace(tl_entry.second.first, "");
      target_filter.filterTransitionsInPlace(tl_entry.second.second, pa, true);
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
  auto search_tl = pa_tls.find(pa);
  if (search_tl != pa_tls.end()) {
    auto search_pa = std::find(pa_order.begin(), pa_order.end(), pa);
    if (search_pa == pa_order.end()) {
      std::cout << "DirectEncoder encodeNoOp: could not find pa " << pa
                << std::endl;
      return;
    }
    // restrict transitions from prev tl to target states
    if (search_pa - pa_order.begin() > 0) {
      for (auto &prev_tl_entry :
           pa_tls[pa_order[search_pa - pa_order.begin() - 1]]) {
        target_filter.filterTransitionsInPlace(prev_tl_entry.second.second, pa,
                                               false);
      }
    }
  } else {
    std::cout << " DirectEncoder encodeNoOp: could not find timeline of pa "
              << pa << std::endl;
  }
}


void DirectEncoder::encodeFuture(AutomataSystem &s, const std::string pa,
                                 const EncICInfo &info, int base_index) {
  int context = calculateContext(info, pa).second;
  Filter target_filter = Filter(info.targets);
  Filter base_filter = Filter(s.instances[base_index].first.states);
  auto search_tl = pa_tls.find(pa);
  if (search_tl != pa_tls.end()) {
    auto search_pa = std::find(pa_order.begin(), pa_order.end(), pa);
    if (search_pa == pa_order.end()) {
      std::cout << "DirectEncoder encodeFuture: could not find pa " << pa
                << std::endl;
      return;
    }
    // formulate clock constraints
    std::string clock = "clX" + info.name;
    bool upper_bounded =
        (info.bounds.upper_bound != std::numeric_limits<int>::max());
    bool lower_bounded =
        (info.bounds.lower_bound != 0 || info.bounds.l_op != "&lt;=");
    std::string guard_upper_bound_crossed =
        clock + inverse_op(info.bounds.r_op) +
        std::to_string(info.bounds.upper_bound);
    std::string guard_constraint_sat =
        (lower_bounded ? clock + reverse_op(info.bounds.l_op) +
                             std::to_string(info.bounds.lower_bound)
                       : "") +
        ((lower_bounded && upper_bounded) ? "&amp;&amp;" : "") +
        (upper_bounded ? clock + info.bounds.r_op +
                             std::to_string(info.bounds.upper_bound)
                       : "");
    // reset clock
    if (search_pa - pa_order.begin() > 0) {
      for (auto &prev_tl_entry :
           pa_tls[pa_order[search_pa - pa_order.begin() - 1]]) {
        base_filter.addToTransitions(
            prev_tl_entry.second.second, "", clock + " = 0",
            pa_order[search_pa - pa_order.begin() - 1], true);
      }
    }
    // determine context (mockup)
    std::size_t context_start = search_pa - pa_order.begin();
    std::size_t context_past_end =
        std::min(search_pa - pa_order.begin() + context + 1,
                 (long int)pa_order.size() - 1);
    std::unordered_map<std::string,
                       std::pair<Automaton, std::vector<Transition>>>
        new_tls;
    std::size_t tls_copied = 0;
    auto curr_future_tl = search_tl;
    encode_counter++;
    // create copies for each tl within the context
    while (Filter::getPrefix(curr_future_tl->first, constants::TL_SEP) !=
           pa_order[context_past_end]) {
      for (auto &tl_entry : curr_future_tl->second) {
        std::string op_name = info.name + "F" + std::to_string(encode_counter);
        std::string new_prefix = addToPrefix(tl_entry.first, op_name);
        for (const auto &orig_s : tl_entry.second.first.states) {
          if (orig_s.inv == "") {
            std::cout << "WARNING: " << orig_s.name << "has no invariant!"
                      << std::endl;
          }
        }
        Automaton cp_automaton =
            Filter::copyAutomaton(tl_entry.second.first, new_prefix, false);
        if (upper_bounded) {
          addInvariants(tl_entry.second.first, base_filter.getFilter(),
                        clock + info.bounds.r_op +
                            std::to_string(info.bounds.upper_bound));
        }
        cp_automaton.clocks.push_back(clock);
        std::vector<Transition> trans_orig_to_cp =
            createCopyTransitionsBetweenTAs(tl_entry.second.first, cp_automaton,
                                            target_filter.getFilter(),
                                            guard_constraint_sat, "", "");
        std::vector<Transition> cp_to_other_cp;
        std::vector<Transition> cp_to_orig;
        // check if next tl also gets copied
        if (tls_copied + context_start + 1 < context_past_end) {
          cp_to_other_cp =
              addToPrefixOnTransitions(tl_entry.second.second, op_name);
        } else {
          cp_to_other_cp =
              addToPrefixOnTransitions(tl_entry.second.second, op_name, true);
          cp_to_orig = createTransitionsBackToOrigTL(
              tl_entry.second.second, new_prefix, tl_entry.first);
          removeTransitionsToNextTl(
              tl_entry.second.second,
              Filter::getPrefix(tl_entry.first, constants::TL_SEP));
        }

        cp_to_other_cp.insert(cp_to_other_cp.end(), trans_orig_to_cp.begin(),
                              trans_orig_to_cp.end());
        cp_to_other_cp.insert(cp_to_other_cp.end(), cp_to_orig.begin(),
                              cp_to_orig.end());
        auto emp = new_tls.emplace(std::make_pair(
            new_prefix, make_pair(cp_automaton, cp_to_other_cp)));
        if (emp.second == false) {
          std::cout << "DirectEncoder encodeFuture: failed to add prefix: "
                    << new_prefix << std::endl;
        }
      }
      tls_copied++;
      curr_future_tl = pa_tls.find(pa_order[context_start + tls_copied]);
      if (curr_future_tl == pa_tls.end()) {
        std::cout << "DirectEncoder encodeFuture: cannot find next tl"
                  << std::endl;
      }
    }
    std::cout << "DirectEncoder encodeFuture: done with copying timelines, "
              << new_tls.size() << " tls created for activation " << pa
              << std::endl;
    for (const auto &new_tl : new_tls) {
      auto emplaced =
          pa_tls[Filter::getPrefix(new_tl.first, constants::TL_SEP)].emplace(
              new_tl);
      if (emplaced.second == false) {
        std::cout
            << "DirectEncoder encodeFuture: final merge failed with automaton "
            << new_tl.first << std::endl;
      }
    }
  } else {
    std::cout << " DirectEncoder encodeFuture: could not find timeline of pa "
              << pa << std::endl;
  }
}

void DirectEncoder::encodePast(AutomataSystem &s, std::vector<State> &targets,
                               const std::string pa, const Bounds bounds,
                               int base_index) {
  Filter target_filter = Filter(targets);
  Filter base_filter = Filter(s.instances[base_index].first.states);
  auto search_tl = pa_tls.find(pa);
  if (search_tl != pa_tls.end()) {
    auto search_pa = std::find(pa_order.begin(), pa_order.end(), pa);
    if (search_pa == pa_order.end()) {
      std::cout << "DirectEncoder encodePast: cannot find pa " << pa
                << std::endl;
      return;
    }
    // formulate clock constraints
    std::string clock = "clX" + pa;
    std::string bvar = "bX" + pa;
    bool upper_bounded =
        (bounds.upper_bound != std::numeric_limits<int>::max());
    bool lower_bounded = (bounds.lower_bound != 0 || bounds.l_op != "&lt;=");
    std::string guard_upper_bound_crossed =
        clock + inverse_op(bounds.r_op) + std::to_string(bounds.upper_bound);
    std::string guard_constraint_sat =
        (lower_bounded ? clock + reverse_op(bounds.l_op) +
                             std::to_string(bounds.lower_bound)
                       : "") +
        ((lower_bounded && upper_bounded) ? "&amp;&amp;" : "") +
        (upper_bounded
             ? clock + bounds.r_op + std::to_string(bounds.upper_bound)
             : "");
    // determine context (mockup)
    std::size_t context_end = search_pa - pa_order.begin() - 1;
    std::size_t context_prior_start =
        std::max(search_pa - pa_order.begin() - CONTEXT - 1, (long int)0);
    std::unordered_map<std::string,
                       std::pair<Automaton, std::vector<Transition>>>
        new_tls;
    std::size_t tls_copied = 0;
    auto curr_past_tl = pa_tls.find(pa_order[context_end]);
    if (curr_past_tl == pa_tls.end()) {
      std::cout << "DirectEncoder encodePast: cannot find previous tl"
                << std::endl;
    }
    // create copies for each tl within the context
    encode_counter++;
    while (Filter::getPrefix(curr_past_tl->first, constants::TL_SEP) !=
           pa_order[context_prior_start]) {
      for (auto &tl_entry : curr_past_tl->second) {
        std::string op_name = pa + "P" + std::to_string(encode_counter);
        std::string new_prefix = addToPrefix(tl_entry.first, op_name);
        Automaton cp_automaton =
            Filter::copyAutomaton(tl_entry.second.first, new_prefix, false);
        if (upper_bounded) {
          addInvariants(cp_automaton, base_filter.getFilter(),
                        clock + bounds.r_op +
                            std::to_string(bounds.upper_bound));
        }
        // target states reached in copy tls
        target_filter.addToTransitions(cp_automaton.transitions, "",
                                       bvar + " = true", "", false);

        cp_automaton.clocks.push_back(clock);
        cp_automaton.bool_vars.push_back(bvar);
        std::vector<Transition> trans_orig_to_cp =
            createCopyTransitionsBetweenTAs(tl_entry.second.first, cp_automaton,
                                            base_filter.getFilter(), "",
                                            clock + " = 0", "");
        // target states already reached in orig tls
        target_filter.addToTransitions(trans_orig_to_cp, "", bvar + " = true",
                                       "", true);

        std::vector<Transition> cp_to_other_cp;
        std::vector<Transition> cp_to_orig;
        // check if this is the latest tl
        if (tls_copied > 0) {
          cp_to_other_cp =
              addToPrefixOnTransitions(tl_entry.second.second, op_name);
        } else {
          cp_to_other_cp =
              addToPrefixOnTransitions(tl_entry.second.second, op_name, true);
          cp_to_orig = createTransitionsBackToOrigTL(
              tl_entry.second.second, new_prefix, tl_entry.first);
          // add constraints that ensure a target state was indeed visited
          // before
          base_filter.addToTransitions(cp_to_orig, bvar + " == true",
                                       bvar + " = false", "", true);

          removeTransitionsToNextTl(tl_entry.second.second, tl_entry.first);
        }
        cp_to_other_cp.insert(cp_to_other_cp.end(), trans_orig_to_cp.begin(),
                              trans_orig_to_cp.end());
        cp_to_other_cp.insert(cp_to_other_cp.end(), cp_to_orig.begin(),
                              cp_to_orig.end());
        new_tls.emplace(std::make_pair(
            new_prefix, make_pair(cp_automaton, cp_to_other_cp)));
      }
      tls_copied++;
      curr_past_tl = pa_tls.find(pa_order[context_end - tls_copied]);
      if (curr_past_tl == pa_tls.end()) {
        std::cout << "DirectEncoder encodePast: cannot find next tl"
                  << std::endl;
      }
    }
    std::cout << "DirectEncoder encodePast: done with copying timelines, "
              << new_tls.size() << " tls created for activation " << pa
              << std::endl;
    for (const auto &new_tl : new_tls) {
      auto emplaced =
          pa_tls[Filter::getPrefix(new_tl.first, constants::TL_SEP)].emplace(
              new_tl);
      if (emplaced.second == false) {
        std::cout
            << "DirectEncoder encodePast: final merge failed with automaton "
            << new_tl.first << std::endl;
      }
    }
  } else {
    std::cout << " DirectEncoder encodePast: could not find timeline of pa "
              << pa << std::endl;
  }
}

size_t DirectEncoder::getPlanTAIndex() { return plan_ta_index; }

AutomataSystem DirectEncoder::createFinalSystem(const AutomataSystem &s,
                                                SystemVisInfo &s_vis) {
  s_vis = SystemVisInfo(pa_tls, s.instances[plan_ta_index].first.states);
  AutomataSystem res = s;
  res.instances.clear();
  std::vector<Automaton> automata;
  std::vector<Transition> interconnections;
  for (const auto &curr_tl : pa_tls) {
    for (const auto &curr_copy : curr_tl.second) {
      automata.push_back(curr_copy.second.first);
      interconnections.insert(interconnections.end(),
                              curr_copy.second.second.begin(),
                              curr_copy.second.second.end());
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
