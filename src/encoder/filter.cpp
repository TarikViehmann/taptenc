#include "filter.h"
#include "timed_automata.h"
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

using namespace taptenc;

Filter::Filter(std::vector<State> arg_filter) { filter = arg_filter; }

std::vector<State> Filter::getFilter() { return filter; }

std::string Filter::stripPrefix(std::string name, std::string prefix) {
  if (name.size() >= prefix.size()) {
    if (name.substr(0, prefix.size()) == prefix) {
      return name.substr(prefix.size(), name.size());
    }
  }
  std::cout << "Filter stripPrefix: " << prefix << " is no prefix of " << name
            << std::endl;
  return name;
}

bool Filter::hasSuffix(std::string name, std::string suffix) {
  if (name.size() >= suffix.size()) {
    return name.substr(name.size() - suffix.size(), suffix.size()) == suffix;
  } else {
    return false;
  }
}

bool Filter::hasPrefix(std::string name, std::string prefix) {
  if (name.size() >= prefix.size()) {
    return name.substr(0, prefix.size()) == prefix;
  } else {
    return false;
  }
}

bool Filter::matchesFilter(std::string name, std::string prefix,
                           std::string suffix) {
  return hasPrefix(name, prefix) && hasSuffix(name, suffix);
}

std::string Filter::getSuffix(std::string name, char marker) {
  return name.substr(name.find_last_of(marker) + 1);
}

std::string Filter::getPrefix(std::string name, char marker) {
  return name.substr(0, name.find_first_of(marker));
}

void Filter::filterTransitionsInPlace(std::vector<Transition> &trans,
                                      std::string prefix, bool filter_source) {
  trans.erase(std::remove_if(trans.begin(), trans.end(),
                             [filter_source, prefix, this](Transition &t) bool {
                               std::string id =
                                   ((filter_source) ? t.source_id : t.dest_id);
                               bool res =
                                   hasPrefix(id, prefix) &&
                                   std::find_if(filter.begin(), filter.end(),
                                                [id, filter_source, prefix,
                                                 this](const State &s) bool {
                                                  return Filter::matchesFilter(
                                                      id, prefix, s.id);
                                                }) == filter.end();
                               return res;
                             }),
              trans.end());
}

void Filter::filterAutomatonInPlace(Automaton &source, std::string prefix) {
  source.states.erase(
      std::remove_if(source.states.begin(), source.states.end(),
                     [prefix, this](State &s) bool {
                       return std::find_if(
                                  filter.begin(), filter.end(),
                                  [prefix, s, this](const State &f_s) bool {
                                    return Filter::matchesFilter(s.id, prefix,
                                                                 f_s.id);
                                  }) == filter.end();
                     }),
      source.states.end());
  filterTransitionsInPlace(source.transitions, prefix, true);
  filterTransitionsInPlace(source.transitions, prefix, false);
}

Automaton Filter::copyAutomaton(const Automaton &source, std::string ta_prefix,
                                bool strip_constraints) {
  std::vector<State> res_states;
  std::vector<Transition> res_transitions;
  for (const auto &s : source.states) {
    res_states.push_back(State(ta_prefix + Filter::getSuffix(s.id, BASE_SEP),
                               strip_constraints ? s.inv : "", s.urgent,
                               s.initial));
  }
  for (const auto &trans : source.transitions) {
    auto source = std::find_if(res_states.begin(), res_states.end(),
                               [trans, ta_prefix](const State &s) bool {
                                 return Filter::matchesFilter(
                                     trans.source_id, "",
                                     Filter::stripPrefix(s.id, ta_prefix));
                               });
    auto dest = std::find_if(res_states.begin(), res_states.end(),
                             [trans, ta_prefix](const State &s) bool {
                               return Filter::matchesFilter(
                                   trans.dest_id, "",
                                   Filter::stripPrefix(s.id, ta_prefix));
                             });
    if (source != res_states.end() && dest != res_states.end()) {
      if (strip_constraints) {
        res_transitions.push_back(Transition(source->id, dest->id, trans.action,
                                             "", "", trans.sync, true));
      } else {
        res_transitions.push_back(Transition(source->id, dest->id, trans.action,
                                             trans.guard, trans.update,
                                             trans.sync, true));
      }
    }
  }
  Automaton res(res_states, res_transitions, ta_prefix, false);
  if (!strip_constraints) {
    res.clocks.insert(res.clocks.end(), source.clocks.begin(),
                      source.clocks.end());
    res.bool_vars.insert(res.bool_vars.end(), source.bool_vars.begin(),
                         source.bool_vars.end());
  }
  return res;
}

Automaton Filter::filterAutomaton(const Automaton &source,
                                  std::string ta_prefix,
                                  std::string filter_prefix,
                                  bool strip_constraints) {
  std::vector<State> res_states;
  std::vector<Transition> res_transitions;
  for (const auto &f_state : filter) {
    auto search = std::find_if(
        source.states.begin(), source.states.end(),
        [f_state, filter_prefix, this](const State &s) bool {
          return Filter::matchesFilter(s.id, filter_prefix, f_state.id);
        });
    if (search != source.states.end()) {
      res_states.push_back(State(ta_prefix + search->name,
                                 strip_constraints ? "" : search->inv,
                                 search->urgent, search->initial));
    } else {
      std::cout << "Filter filterAutomaton: filter state not found (id "
                << f_state.id << ")" << std::endl;
    }
  }
  for (const auto &trans : source.transitions) {
    auto source = std::find_if(
        res_states.begin(), res_states.end(),
        [trans, this, filter_prefix, ta_prefix](const State &s) bool {
          return Filter::matchesFilter(trans.source_id, filter_prefix,
                                       Filter::stripPrefix(s.id, ta_prefix));
        });
    auto dest = std::find_if(
        res_states.begin(), res_states.end(),
        [trans, this, filter_prefix, ta_prefix](const State &s) bool {
          return Filter::matchesFilter(trans.dest_id, filter_prefix,
                                       Filter::stripPrefix(s.id, ta_prefix));
        });
    if (source != res_states.end() && dest != res_states.end()) {
      if (strip_constraints) {
        res_transitions.push_back(Transition(source->id, dest->id, trans.action,
                                             "", "", trans.sync, true));
      } else {
        res_transitions.push_back(Transition(source->id, dest->id, trans.action,
                                             trans.guard, trans.update,
                                             trans.sync, true));
      }
    }
  }
  Automaton res(res_states, res_transitions, ta_prefix, false);
  if (!strip_constraints) {
    res.clocks.insert(res.clocks.end(), source.clocks.begin(),
                      source.clocks.end());
    res.bool_vars.insert(res.bool_vars.end(), source.bool_vars.begin(),
                         source.bool_vars.end());
  }
  return res;
}

void Filter::addToTransitions(std::vector<Transition> &trans, std::string guard,
                              std::string update, std::string prefix,
                              bool filter_source) {
  for (auto &tr : trans) {
    auto search = std::find_if(
        filter.begin(), filter.end(),
        [filter_source, tr, prefix, this](const State &s) bool {
          return Filter::matchesFilter(
              ((filter_source) ? tr.source_id : tr.dest_id), prefix, s.id);
        });
    if (search != filter.end()) {
      tr.guard = addConstraint(tr.guard, guard);
      tr.update = addUpdate(tr.update, update);
    }
  }
}
Filter Filter::updateFilter(const Automaton &ta) {
  std::vector<State> update_filter;
  for (const auto &f_state : filter) {
    auto search =
        std::find_if(ta.states.begin(), ta.states.end(),
                     [f_state, this](const State &s) bool {
                       return Filter::matchesFilter(s.id, "", f_state.id);
                     });
    if (search != ta.states.end()) {
      update_filter.push_back(*search);
    } else {
      std::cout << "Filter updateFilter: base id of filter not found (id "
                << f_state.id << ")" << std::endl;
    }
  }
  return Filter(update_filter);
}

Filter Filter::reverseFilter(const Automaton &ta) {
  std::vector<State> reverse_filter;
  for (const auto &ta_state : ta.states) {
    auto search = std::find_if(
        filter.begin(), filter.end(), [ta_state](const State &s) bool {
          return Filter::matchesFilter(ta_state.id, "", s.id);
        });
    if (search == ta.states.end()) {
      reverse_filter.push_back(ta_state);
    }
  }
  return Filter(reverse_filter);
}
