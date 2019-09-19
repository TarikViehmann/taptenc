#pragma once
#include "timed_automata.h"

namespace taptenc {
class Filter {
private:
  ::std::vector<State> filter;
  static bool hasSuffix(::std::string name, ::std::string suffix);
  static bool hasPrefix(::std::string name, ::std::string prefix);

public:
  Filter(std::vector<State> input);
  static bool matchesFilter(::std::string name, ::std::string prefix,
                            ::std::string suffix);
  static ::std::string stripPrefix(::std::string name, ::std::string prefix);
  static ::std::string getSuffix(::std::string name, char marker);
  static ::std::string getPrefix(::std::string name, char marker);
  void filterTransitionsInPlace(::std::vector<Transition> &trans,
                                ::std::string prefix, bool filter_source) const;
  void filterAutomatonInPlace(Automaton &source, ::std::string prefix) const;
  static Automaton copyAutomaton(const Automaton &source,
                                 ::std::string ta_prefix,
                                 bool strip_constraints = true);
  Automaton filterAutomaton(const Automaton &source, ::std::string ta_prefix,
                            ::std::string filter_prefix = "",
                            bool strip_constraints = true) const;
  void addToTransitions(::std::vector<Transition> &trans, ::std::string guard,
                        ::std::string update, ::std::string prefix,
                        bool filter_source) const;
  ::std::vector<State> getFilter() const;
  Filter updateFilter(const Automaton &ta) const;
  Filter reverseFilter(const Automaton &ta) const;
  bool matchesId(const std::string id) const;
};
} // end namespace taptenc
