/** \file
 * Manipulation of TA states and transition by the means of a filter that
 * sieves out a subset of states and transitions.
 *
 * \author (2019) Tarik Viehmann
 */
#pragma once
#include "../timed-automata/timed_automata.h"

namespace taptenc {
/**
 * Manipulates TA states/transitions by sieving out some of them.
 */
class Filter {
private:
  /** Sieve to apply to the states/transition sources/destinations. */
  ::std::vector<State> filter;
  /**
   * If true, keeps the sieved out content instead of the sieved through stuff.
   * */
  bool invert_effect = false;
  /**
   * Wrapper to check queries in \a filter. Inverts the output if \a
   * invert_effect is true.
   */
  bool isFilterEndIt(const ::std::vector<State>::const_iterator &it) const;
  /**
   * Checks if a string has a specified suffix.
   *
   * @param name string to check
   * @param suffix suffix to search in \a name
   * @return true, iff \a suffix is a suffix of \a name
   */
  static bool hasSuffix(::std::string name, ::std::string suffix);
  /**
   * Checks if a string has a specified prefix.
   *
   * @param name string to check
   * @param prefix prefix to search in \a name
   * @return true, iff \a prefix is a prefix of \a name
   */
  static bool hasPrefix(::std::string name, ::std::string prefix);

public:
  /**
   * Instanciate a filter by specifying the filter states.
   *
   * @param input filter states
   * @param arg_invert_effect If true, keeps the sieved out content instead of
   *                          the sieved through stuff
   */
  Filter(std::vector<State> input, bool arg_invert_effect = false);
  /**
   * Checks if a prefix and suffix is contained in a string.
   *
   * @param name string to check for prefix and suffix
   * @param prefix prefix to search in \a name
   * @param suffix suffix to search in \a name
   */
  static bool matchesFilter(::std::string name, ::std::string prefix,
                            ::std::string suffix);
  /**
   * Removes a prefix from a string.
   *
   * @param name string to strip prefix from
   * @param prefix prefix to strip
   * @return \a name without \a prefix, if \a prefix is a prefix of \a name
   *         else \a name is returned
   */
  static ::std::string stripPrefix(::std::string name, ::std::string prefix);

  /**
   * Get the suffix of a string after the last occurance of a specified
   * character.
   *
   * @param name string to obtain suffix of
   * @param marker after which's last occurance the suffix is taken of
   * @return suffix of \a name after \a marker, or \a name if \a marker is not
   * contained in \a name
   */
  static ::std::string getSuffix(::std::string name, char marker);

  /**
   * Get the prefix of a string before the first occurance of a specified
   * character.
   *
   * @param name string to obtain prefix of
   * @param marker before which's first occurance the prefix is taken of
   * @return prefix of \a name before \a marker, or \a name if \a marker is not
   * contained in \a name
   */
  static ::std::string getPrefix(::std::string name, char marker);

  /**
   * Removes all transitions which source/dest id has a certain prefix and
   * matches the filter.
   *
   * @param trans transitions to filter
   * @param prefix prefix of source/dest ids that may be deleted if the filter
   *               matches. All transitions which source/dest id does not match
   *               the prefix are NOT deleted.
   * @param filter_source if true, filter the source ids of the transitions in
   *                      \a trans, else filter the dest ids
   */
  void filterTransitionsInPlace(::std::vector<Transition> &trans,
                                ::std::string prefix, bool filter_source) const;

  /**
   * Removes all states form an automaton that hava a certain prefix and match
   * a filter.
   *
   * @param source automaton to filter states of
   * @param prefix prefix to only filter out states matchin the prefix
   */
  void filterAutomatonInPlace(Automaton &source, ::std::string prefix) const;

  /**
   * Creates a copy of an automaton.
   *
   * @param source automaton to copy
   * @param ta_prefix prefix for the automaton copy
   * @param strip_constraints if true, removes the guards and updates from the
   *                          transitions
   * @return Automaton copy
   */
  static Automaton copyAutomaton(const Automaton &source,
                                 ::std::string ta_prefix,
                                 bool strip_constraints = true);

  /**
   * Creates a copy of an automaton with filtered states.
   *
   * TODO: call filterAutomatonInPlace()
   *
   * @param source automaton to copy
   * @param ta_prefix prefix for the automaton copy
   * @param filter_prefix prefix of states to copy
   * @param strip_constraints if true, removes the guards and updates from the
   *                          transitions
   * @return Automaton copy with filtered states
   */
  Automaton filterAutomaton(const Automaton &source, ::std::string ta_prefix,
                            ::std::string filter_prefix = "",
                            bool strip_constraints = true) const;

  /**
   * Adds guards and updates to transitions matching a filter.
   *
   * @param trans transitions to add annotations to
   * @param guard guard to add
   * @param update update tu add
   * @param prefix prefix of source/dest ids to add annotations to
   * @param filter_source if true, filter the source ids of the transitions in
   *                      \a trans, else filter the dest ids
   */
  void addToTransitions(::std::vector<Transition> &trans, ::std::string guard,
                        ::std::string update, ::std::string prefix,
                        bool filter_source) const;
  /**
   * Gets a copy of the filter states.
   */
  ::std::vector<State> getFilter() const;

  /**
   * Crates a new filter by adding all states from a ta that match the filter.
   *
   * Can be used after copying a TA to obtain a refined filter that matches the
   * new copy. Ignores reversed filter options!
   *
   * @param ta automaton to take as new filter baseline
   * @return Filter with states of \a ta matching the current filter
   */
  Filter updateFilter(const Automaton &ta) const;

  /**
   * Creates a filter containing all states from a TA that do not match the
   * current filter. Essentially contains all states that updateFilter() would
   * not countain. Ignores reversed filter options!
   *
   * @param ta automaton to take as new filter baseline
   */
  Filter reverseFilter(const Automaton &ta) const;

  /**
   * Check if a state id matches the filter.
   *
   * @param id state id
   * @return true, if id matches the filter, else return false
   */
  bool matchesId(const std::string id) const;
};
} // end namespace taptenc
