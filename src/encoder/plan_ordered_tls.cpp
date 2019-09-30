#include "plan_ordered_tls.h"
#include "constants.h"
#include "encoder_utils.h"
#include "filter.h"
#include "timed_automata.h"
#include "utils.h"
#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#define CONTEXT 2

using namespace taptenc;
void PlanOrderedTLs::printTLs() const {
  std::cout << std::endl;
  for (const auto &pa : *(pa_order.get())) {
    const auto &entry = tls.get()->find(pa);
    if (entry != tls.get()->end()) {
      std::cout << "TL: " << entry->first << std::endl;
      for (const auto &ta : entry->second) {
        std::cout << "    TA: " << ta.first << std::endl;
      }
    }
  }
  for (const auto &entry : *(tls.get())) {
    const auto &search =
        std::find(pa_order.get()->begin(), pa_order.get()->end(), entry.first);
    if (search == pa_order.get()->end()) {
      std::cout << "extra TL: " << entry.first << std::endl;
      for (const auto &ta : entry.second) {
        std::cout << "\t TA: " << ta.first << std::endl;
      }
    }
  }
  std::cout << std::endl;
}

OrigMap PlanOrderedTLs::createOrigMapping(std::string prefix) const {
  OrigMap res;
  for (const auto &curr_tl : *(tls.get())) {
    for (const auto &tl_entry : curr_tl.second) {
      if (tl_entry.second.ta.prefix == constants::QUERY) {
        continue;
      }
      std::string ta_prefix =
          prefix == ""
              ? tl_entry.second.ta.prefix
              : encoderutils::addToPrefix(tl_entry.second.ta.prefix, prefix);
      res[ta_prefix] = tl_entry.second.ta.prefix;
    }
  }
  return res;
}

PlanOrderedTLs PlanOrderedTLs::createWindow(std::string start_pa,
                                            std::string end_pa,
                                            const Filter &target_filter,
                                            std::string prefix_add) const {
  PlanOrderedTLs new_window;
  auto start_pa_entry =
      std::find(pa_order.get()->begin(), pa_order.get()->end(), start_pa);
  if (start_pa_entry == pa_order.get()->end()) {
    std::cout << "PlanOrderedTLs createWindow: could not find start pa "
              << start_pa << std::endl;
    return new_window;
  }
  auto end_pa_entry = std::find(start_pa_entry, pa_order.get()->end(), end_pa);
  if (end_pa_entry == pa_order.get()->end()) {
    std::cout << "PlanOrderedTLs createWindow: could not find end pa " << end_pa
              << std::endl;
    std::cout << "PlanOrderedTLs createWindow: prefix add" << prefix_add
              << std::endl;
    return new_window;
  }
  std::size_t context_start = start_pa_entry - pa_order.get()->begin();
  std::size_t context_end = end_pa_entry - pa_order.get()->begin();
  auto curr_tl = tls.get()->find(*(pa_order.get()->begin() + context_start));
  std::string op_name =
      Filter::getPrefix(prefix_add, constants::CONSTRAINT_SEP);
  if (curr_tl != tls.get()->end()) {
    // iterate over orig TLs of window
    int tls_copied = 0;
    while (context_start + tls_copied <= context_end) {
      TimeLine copy_tls;
      TimeLine new_tls;
      // copy all original tls
      for (auto &tl_entry : curr_tl->second) {
        std::string ta_prefix =
            encoderutils::addToPrefix(tl_entry.second.ta.prefix, prefix_add);
        Automaton copy_ta = target_filter.filterAutomaton(tl_entry.second.ta,
                                                          ta_prefix, "", false);
        std::vector<Transition> cp_to_other_cp;
        // also copy the transitions connecting different orig TLs
        if (tls_copied + context_start < context_end) {
          cp_to_other_cp =
              addToPrefixOnTransitions(tl_entry.second.trans_out, op_name);
          target_filter.filterTransitionsInPlace(cp_to_other_cp, "", false);
        } else {
          cp_to_other_cp = addToPrefixOnTransitions(tl_entry.second.trans_out,
                                                    op_name, true, false);
        }
        target_filter.filterTransitionsInPlace(cp_to_other_cp, "", true);
        auto emp = new_tls.emplace(
            std::make_pair(ta_prefix, TlEntry(copy_ta, cp_to_other_cp)));
        if (emp.second == false) {
          std::cout << "PlanOrderedTLs createWindow: failed to add prefix: "
                    << prefix_add << std::endl;
        }
      }
      // insert the new tls and also save them in the curr_window
      for (const auto &new_tl : new_tls) {
        (*new_window.tls
              .get())[Filter::getPrefix(new_tl.first, constants::TL_SEP)]
            .emplace(new_tl);
      }
      tls_copied++;
      if (context_start + tls_copied < pa_order.get()->size()) {
        curr_tl =
            tls.get()->find(pa_order.get()->at(context_start + tls_copied));
        if (curr_tl == tls.get()->end()) {
          std::cout << "PlanOrderedTLs createWindow: cannot find next tl"
                    << std::endl;
        }
      } else {
        curr_tl = tls.get()->end();
      }
    }
  } else {
    std::cout << "PlanOrderedTLs createWindow: done cannot find start tl. "
                 "prefix "
              << prefix_add << " context start pa "
              << *(pa_order.get()->begin() + context_start) << std::endl;
  }
  for (const auto &pa : *(pa_order.get())) {
    new_window.pa_order.get()->push_back(pa);
  }
  return new_window;
}

void PlanOrderedTLs::createTransitionsToWindow(
    const Automaton &base_ta, TimeLines &dest_tls,
    const std::unordered_map<std::string, std::string> &map_to_orig,
    std::string start_pa, std::string end_pa, const Filter &target_filter,
    std::string guard, std::string update) {
  auto start_pa_entry =
      std::find(pa_order.get()->begin(), pa_order.get()->end(), start_pa);
  if (start_pa_entry == pa_order.get()->end()) {
    std::cout << "PlanOrderedTLs createTransitionsBetweenWindows: could not "
                 "find start pa "
              << start_pa << std::endl;
    return;
  }
  auto end_pa_entry = std::find(start_pa_entry, pa_order.get()->end(), end_pa);
  if (end_pa_entry == pa_order.get()->end()) {
    std::cout << "PlanOrderedTLs createTransitionsBetweenWindows: could not "
                 "find end pa "
              << end_pa << std::endl;
    return;
  }
  std::size_t context_start = start_pa_entry - pa_order.get()->begin();
  std::size_t context_end = end_pa_entry - pa_order.get()->begin();
  auto source_tl = tls.get()->find(*(pa_order.get()->begin() + context_start));
  int i = 0;
  while (context_start + i <= context_end && source_tl != tls.get()->end()) {
    auto dest_tl =
        dest_tls.find(*(pa_order.get()->begin() + context_start + i));
    if (dest_tl != dest_tls.end()) {
      for (auto &source_entry : source_tl->second) {
        auto dest_entry = std::find_if(
            dest_tl->second.begin(), dest_tl->second.end(),
            [source_entry,
             map_to_orig](const std::pair<std::string, TlEntry> &d) bool {
              auto orig_source_entry = map_to_orig.find(source_entry.first);
              auto orig_dest_entry = map_to_orig.find(d.first);
              return orig_source_entry != map_to_orig.end() &&
                     orig_dest_entry != map_to_orig.end() &&
                     orig_source_entry->second == orig_dest_entry->second;
            });
        if (dest_entry != dest_tl->second.end()) {
          std::vector<Transition> res;
          res = encoderutils::createCopyTransitionsBetweenTAs(
              source_entry.second.ta, dest_entry->second.ta,
              dest_entry->second.ta.states, guard, update, "");
          std::vector<Transition> res_succ =
              encoderutils::createSuccessorTransitionsBetweenTAs(
                  base_ta, source_entry.second.ta, dest_entry->second.ta,
                  source_entry.second.ta.states, guard, update);
          target_filter.filterTransitionsInPlace(res, dest_entry->first, false);
          target_filter.filterTransitionsInPlace(res_succ, dest_entry->first,
                                                 false);
          source_entry.second.trans_out.insert(
              source_entry.second.trans_out.end(), res.begin(), res.end());
          source_entry.second.trans_out.insert(
              source_entry.second.trans_out.end(), res_succ.begin(),
              res_succ.end());
        }
      }
    }
    i++;
    if (context_start + i < pa_order.get()->size()) {
      source_tl =
          tls.get()->find(*(pa_order.get()->begin() + context_start + i));
    } else {
      break;
    }
  }
}

void PlanOrderedTLs::mergeWindow(const TimeLines &to_add, bool overwrite) {
  for (const auto &tl : to_add) {
    auto dest_tl = tls.get()->find(tl.first);
    if (dest_tl == tls.get()->end()) {
      tls.get()->insert(tl);
    } else {
      for (const auto &tl_entry : tl.second) {
        auto dest_tl_entry = dest_tl->second.find(tl_entry.first);
        if (dest_tl_entry == dest_tl->second.end()) {
          dest_tl->second.insert(tl_entry);
        } else if (overwrite) {
          dest_tl_entry->second = tl_entry.second;
        } else {
          // skip
        }
      }
    }
  }
}

void PlanOrderedTLs::addOutgoingTransOfOrigTL(const TimeLine &orig_tl,
                                              TimeLine &new_tl,
                                              const OrigMap &to_orig,
                                              std::string guard) {
  for (auto &tl_entry : new_tl) {
    auto orig_name = to_orig.find(tl_entry.first);
    if (orig_name != to_orig.end()) {
      const auto &orig_entry = orig_tl.find(orig_name->second);
      if (orig_entry != orig_tl.end()) {
        for (const auto &tr : orig_entry->second.trans_out) {
          if (Filter::getPrefix(tr.source_id, constants::TL_SEP) !=
              Filter::getPrefix(tr.dest_id, constants::TL_SEP)) {
            std::string source_base_name =
                Filter::getSuffix(tr.source_id, constants::BASE_SEP);
            const auto &source_state = std::find_if(
                tl_entry.second.ta.states.begin(),
                tl_entry.second.ta.states.end(),
                [source_base_name](const State &s) bool {
                  return Filter::getSuffix(s.id, constants::BASE_SEP) ==
                         source_base_name;
                });
            if (source_state != tl_entry.second.ta.states.end()) {
              tl_entry.second.trans_out.push_back(Transition(
                  tl_entry.first +
                      Filter::getSuffix(tr.source_id, constants::BASE_SEP),
                  tr.dest_id, tr.action, addConstraint(tr.guard, guard),
                  tr.update, tr.sync));
            }
          }
        }
      } else {
        std::cout
            << "PlanOrderedTLs addOutgoingTransOfOrigTL: cannot find orig "
               "tl entry "
            << orig_name->second << std::endl;
      }
    } else {
      std::cout
          << "PlanOrderedTLs addOutgoingTransOfOrigTL: orig mapping not found: "
          << tl_entry.first << std::endl;
    }
  }
}

std::vector<Transition> PlanOrderedTLs::addToPrefixOnTransitions(
    const std::vector<Transition> &trans, std::string to_add,
    bool on_inner_trans, bool on_outgoing_trans) {
  std::vector<Transition> res;
  for (const auto &tr : trans) {
    bool is_inner = Filter::getPrefix(tr.source_id, constants::TL_SEP) ==
                    Filter::getPrefix(tr.dest_id, constants::TL_SEP);
    if ((is_inner && on_inner_trans) || (!is_inner && on_outgoing_trans)) {
      res.push_back(Transition(encoderutils::addToPrefix(tr.source_id, to_add),
                               encoderutils::addToPrefix(tr.dest_id, to_add),
                               tr.action, tr.guard, tr.update, tr.sync));
    }
  }
  return res;
}

void PlanOrderedTLs::modifyTransitionsToNextTl(
    std::vector<Transition> &trans, std::string curr_pa,
    const std::vector<State> &target_states, std::string guard,
    std::string update, std::string sync, std::string op_name) {
  for (auto &t : trans) {
    if (Filter::getPrefix(t.dest_id, constants::TL_SEP) != curr_pa) {
      t.guard = addConstraint(t.guard, guard);
      t.update = addUpdate(t.update, update);
      if (sync != "")
        t.sync = sync;
      if (op_name != "") {
        t.dest_id = encoderutils::addToPrefix(t.dest_id, op_name);
      }
    }
  }
  trans.erase(std::remove_if(trans.begin(), trans.end(),
                             [target_states](const Transition &t) bool {
                               return std::find_if(
                                          target_states.begin(),
                                          target_states.end(),
                                          [t](const State &s) bool {
                                            return Filter::matchesFilter(
                                                t.dest_id, "", s.id);
                                          }) == target_states.end();
                             }),
              trans.end());
}

void PlanOrderedTLs::removeTransitionsToNextTl(std::vector<Transition> &trans,
                                               std::string curr_pa) {
  trans.erase(std::remove_if(trans.begin(), trans.end(),
                             [curr_pa](Transition &t) bool {
                               return Filter::getPrefix(t.dest_id,
                                                        constants::TL_SEP) !=
                                      curr_pa;
                             }),
              trans.end());
}

void PlanOrderedTLs::addStateInvariantToWindow(std::string start_pa,
                                               std::string end_pa,
                                               std::string inv) {
  auto start_pa_entry =
      std::find(pa_order.get()->begin(), pa_order.get()->end(), start_pa);
  if (start_pa_entry == pa_order.get()->end()) {
    std::cout
        << "PlanOrderedTLs addStateInvariantToWindow: could not find start pa "
        << start_pa << std::endl;
    return;
  }
  auto end_pa_entry = std::find(start_pa_entry, pa_order.get()->end(), end_pa);
  if (end_pa_entry == pa_order.get()->end()) {
    std::cout
        << "PlanOrderedTLs addStateInvariantToWindow: could not find end pa "
        << end_pa << std::endl;
    return;
  }
  size_t curr_pa_index = start_pa_entry - pa_order.get()->begin();
  size_t end_pa_index = end_pa_entry - pa_order.get()->begin();
  while (curr_pa_index <= end_pa_index) {
    auto curr_tl = tls.get()->find(*(pa_order.get()->begin() + curr_pa_index));
    if (curr_tl == tls.get()->end()) {
      std::cout << "PlanOrderedTLs addStateInvariantToWindow: TLs for pa "
                << *(pa_order.get()->begin() + curr_pa_index) << " not found"
                << std::endl;
      break;
    }
    for (auto &tl_entry : curr_tl->second) {
      encoderutils::addInvariants(tl_entry.second.ta, tl_entry.second.ta.states,
                                  inv);
    }
    curr_pa_index++;
  }
}
