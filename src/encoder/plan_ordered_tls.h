/** \file
 * Provides a utility class for TimeLines modifications.
 *
 * \author: (2019) Tarik Viehmann
 */
#pragma once
#include "filter.h"
#include "timed_automata.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace taptenc {
typedef ::std::unordered_map<::std::string, ::std::string> OrigMap;
/**
 * Bundles functionalities to manipulate TimeLines.
 *
 * This class realizes the atomic steps required to construct a direct encoding.
 */
class PlanOrderedTLs {
private:
  /**
   * Merges all TAs of a timeline together.
   * @param tl timeline to collaps
   * @param tl_name name of the tl (key within TimeLines)
   * @param outgoing all outgoing transitions of the timeline get pushed back
   *        into that vector.
   * @return a TA consisting off all TAs from the input TimeLine
   */
  static Automaton collapseTL(const TimeLine &tl, ::std::string tl_name,
                              ::std::vector<Transition> &outgoing);

  /**
   * Constructs a product TA by replacing each state of one TA by anoter TA.
   * @param source_ta TA where states should be replaced
   * @param ta_to_insert TA that gets inserted to all states of source_ta
   * @param add_succ_trans if true, adds transitions that simultaniously
   *                       change states in source_ta and ta_to_insert
   * @return TimeLine representation of the product, with one entry containing
   *         ta_to_insert for each state of source_ta
   */
  static TimeLine replaceStatesByTA(const Automaton &source_ta,
                                    const Automaton &ta_to_insert,
                                    bool add_succ_trans);

public:
  /** Unique access to a TimeLines instance. */
  ::std::unique_ptr<TimeLines> tls = ::std::make_unique<TimeLines>();
  /**
   * Unique access to a vector representing a sequential plan ordering.
   * The entries of \a pa_order are keys of \a tls.
   * */
  ::std::unique_ptr<::std::vector<::std::string>> pa_order =
      ::std::make_unique<::std::vector<::std::string>>();

  /**
   * Adds an invariant to all states within a window in the timelines.
   * @param start_pa plan action name to specify the start of window
   * @param end_pa plan action name to specify the end of window
   * @param inv invariant to add
   */
  void addStateInvariantToWindow(::std::string start_pa, ::std::string end_pa,
                                 const ClockConstraint &inv);
  /**
   * Adds outgoing transitions of the original timeline to a copied timeline.
   * ~~~
   * Example:                     TL
   *                              |
   *                              v
   *          Given:  orig:   x-x-x-x-x
   *                  new:      y-y
   *          Result: orig:   x-x-x-x-x
   *                  new:      y-y/
   * ~~~
   * @param orig_tl original timeline
   * @param new_tl copied timeline
   * @param to_orig map from new_tl keys to original keys in orig_tl
   * @param guard guard to add to the newly created transitions
   */
  static void addOutgoingTransOfOrigTL(const TimeLine &orig_tl,
                                       TimeLine &new_tl, const OrigMap &to_orig,
                                       const ClockConstraint &guard);
  /**
   * Copies a vector of transitions and adds a prefix to the copies.
   * @param trans original transitions
   * @param prefix prefix to add
   * @param on_inner_trans if false do not copy inner transitions of a timeline
   * @param on_outgoing_trans if false do not copy outgoing transitions of a
   *        timeline
   * @return copied transitions with added prefix
   */
  static ::std::vector<Transition>
  addToPrefixOnTransitions(const ::std::vector<Transition> &trans,
                           ::std::string prefix, bool on_inner_trans = true,
                           bool on_outgoing_trans = true);

  /**
   * Adds guards, updates and sync annotations to outgoing transitions.
   * Can also modify the destination of outgoing transitions by adding a prefix.
   * ~~~
   * Example usage: x-x-x-x  ->  x x-x-x
   *                  y-y         \y-y
   * ~~~
   * @param trans transitions to modify
   * @param curr_pa current plan action (to determine outgoing transitions)
   *        TODO can be refactored away
   * @param target_states specify states of a target window, useful if the
   *        destination prefix is modified and the new target automaton does
   *        not have the target state of a transition. Then the transition
   *        becomes obsolete and has to be deleted
   * @param guard guard to add
   * @param update update to add
   * @param sync sync to add
   * @param op_name add an operator to the prefix of the dest states
   */
  static void modifyTransitionsToNextTl(
      ::std::vector<Transition> &trans, ::std::string curr_pa,
      const ::std::vector<State> &target_states, const ClockConstraint &guard,
      const update_t &update, ::std::string sync, ::std::string op_name = "");

  /**
   * Removes transitions to next timeline.
   * ~~~
   * Example:  x-x-x-x-x  -> x-x-x x-x
   *             y-y           y-y
   * ~~~
   * @param trans outgoing transition
   * @param curr_pa current plan action
   *        TODO refactor this away
   */
  static void removeTransitionsToNextTl(::std::vector<Transition> &trans,
                                        ::std::string curr_pa);
  /**
   * Creates a map from each automata name (added by a prefix) in tls to the
   * same name
   * @param prefix prefix to add to the map keys
   * @return OrigMap for each TA in tls: TA.id -> addToPrefix(TA.id, prefix_add)
   */
  OrigMap createOrigMapping(::std::string prefix) const;

  /**
   * Creates copies of the timelines in tls within an interval specified
   * by plan acttion names.
   * @param start_pa plan action name that starts the window of tls to copy
   * @param end_pa plan action name that ends the window of tls to copy
   * @param target_filter state filter to apply on all copied automata
   * @param prefix_add prefix to be added to the original TA names
   * @return tls between start_pa and end_pa with added prefix and
   *         filtered states
   */
  PlanOrderedTLs createWindow(::std::string start_pa, ::std::string end_pa,
                              const Filter &target_filter,
                              ::std::string prefix_add) const;
  /**
   * Creates successor and copy transitions to other timelines.
   * @param base_ta the full automaton where all automata copies originated from
   * @param dest_tls target timelines
   * @param map_to_orig Mapping to match copies of target timelines to timeline
   *        entries of tls
   * @param start_pa plan action name that starts the window of tls to connect
   * @param end_pa plan action name that ends the window of tls to connect
   * @param target_filter filter describing the target states within dest_tls
   *        that should be reached
   * @param guard guard to add on the created transitions
   * @param update update to add on the created transitions
   * @param add_succ_trans if true, also adds successor transitions
   */
  void createTransitionsToWindow(
      const Automaton &base_ta, TimeLines &dest_tls,
      const ::std::unordered_map<std::string, ::std::string> &map_to_orig,
      ::std::string start_pa, ::std::string end_pa, const Filter &target_filter,
      const ClockConstraint &guard, const update_t &update,
      bool add_succ_trans);

  /**
   * Merges a timeline window into tls.
   * @param to_add timeline window
   * @param overwrite false: skip entries of to_add that have the same prefix as
   *                         a TL in tls
   *                  true: replace entries of tls by the ones in to_add in case
   *                         of existing prefix
   *
   */
  void mergeWindow(const TimeLines &to_add, bool overwrite = false);

  /** Prints the content of tls to std::cout, sorted by plan order.
   * TimeLines where the key does not match a plan action are listed afterwads.
   */
  void printTLs() const;

  /**
   * Merges togehter two direct encodings.
   * Caution: the merged result does not follow the typical pattern of
   * 'map_id' -> TA with prefix 'map_id' in the timelines
   * @param other other direct encoding
   *        Assumes other pa_order is equal to this pa_order
   * @return merge of this tls and other tls
   */
  PlanOrderedTLs mergePlanOrderedTLs(const PlanOrderedTLs &other) const;

  /**
   * Constructs a product TA between two TAs.
   * @param ta1 first ta of the product
   * @param ta2 second ta of the product
   * @param name name of the product ta
   * @param add_succ_trans if true, adds transitions that simultaniously
   *                       change states in ta1 and ta2
   * @return Product ta of ta1 and ta2 by replacing each state of ta1
   *         by a copy of ta2
   */
  static Automaton productTA(const Automaton &ta1, const Automaton &ta2,
                             ::std::string name, bool add_succ_trans);
};
} // end namespace taptenc
