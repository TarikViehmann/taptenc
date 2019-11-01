/** \file
 * Utility functions to modify automata.
 *
 * TODO: Review functions, whether they belong in filter class
 *
 * \author: (2019) Tarik Viehmann
 */

#pragma once

#include "../constraints/constraints.h"
#include "../timed-automata/timed_automata.h"
#include <string>
#include <vector>

namespace taptenc {
/**
 * Contains utility functions to modify automatas.
 */
namespace encoderutils {

/**
 * Merges multiple automata together, states with identical names are melted
 * together.
 *
 * @param automata automata to merge together
 * @param interconnections transitions between the automata
 * @param prefix prefix of the merged automaton
 * @return Automaton consisting of all states and transitions from the
 *         automata in \a automata and the transitions from
 *         \a interconnections
 */
Automaton mergeAutomata(const ::std::vector<Automaton> &automata,
                        ::std::vector<Transition> &interconnections,
                        ::std::string prefix);

/**
 * Creates transitions from a TA to one of its copies from each state to its
 * copied state.
 *
 * @param source automaton with the source states
 * @param dest automaton with the dest states
 * @param filter states that should be connected.
 *        Other states are ignored when creating transitions.
 *        Only the state id suffix after constants::BASE_SEP is matched.
 * @param guard guard on the created transitions
 * @param update update on the created transitions
 * @param sync sync channel on the created transitions
 * @param passive is the sync emitting or receiving?
 */
::std::vector<Transition> createCopyTransitionsBetweenTAs(
    const Automaton &source, const Automaton &dest,
    const ::std::vector<State> &filter, const ClockConstraint &guard,
    ::std::string update, ::std::string sync, bool passive = true);

/**
 * Creates transitions from a TA to one of its copies by adding transitions from
 * the base automaton with a source id contained in the TA and dest id contained
 * in the target copy.
 *
 * @param base automaton with all transitions that should be considered.
 *        Typically this is the automaton where \a source and \a dest were based
 *        of.
 * @param source automaton with the source states
 * @param dest automaton with the dest states
 * @param filter states that should be connected.
 *        Other states are ignored when creating transitions.
 *        Only the state id suffix after constants::BASE_SEP is matched.
 * @param guard guard on the created transitions
 * @param update update on the created transitions
 */
::std::vector<Transition> createSuccessorTransitionsBetweenTAs(
    const Automaton &base, const Automaton &source, const Automaton &dest,
    const ::std::vector<State> &filter, const ClockConstraint &guard,
    ::std::string update);

/**
 * Adds transitions to the trap state.
 *
 * @param ta automaton with trap state
 * @param sources states form \a ta that should be connected to the trap
 * @param guard guard on the created transitions
 * @param update update on the created transitions
 * @param sync sync channel on the created transitions
 * @param passive is the sync emitting or receiving?
 */
void addTrapTransitions(Automaton &ta, const ::std::vector<State> &sources,
                        const ClockConstraint &guard, ::std::string update,
                        ::std::string sync, bool passive = true);

/**
 * Adds sync channels on each transition of an automaton.
 *
 * Allows to have a base automaton in a system which syncronizes all transitions
 * with any copy that is made from it.
 *
 * @param s automata system containing the base automaton
 * @param base_pos positio of hte automaton in \a s
 */
void addBaseSyncs(AutomataSystem &s, const int base_pos);

/**
 * Adds invariants to the specified states of a TA.
 *
 * @param ta automaton to add invariants to
 * @param filter states that should receive the invariant.
 *               A state from \a filter matches one from \a ta, if it is
 *               a prefix form the \a ta state.
 * @param inv invariant to add
 */
void addInvariants(Automaton &ta, const ::std::vector<State> filter,
                   const ClockConstraint &inv);
/**
 * Creates a formated prefix with constants::TL_SEP, constants::CONSTRAINT_SEP
 * and constants::BASE_SEP.
 *
 * @param op operator (between TL_SEP and CONSTRAINT_SEP)
 * @param sub next operator (between CONSTRAINT_SEP and BASE_SEP)
 * @param pa plan action (before TL_SEP)
 * @return \a pa + TL_SEP + \a op + CONSTRAINT_SEP + \a sub + BASE_SEP
 */
::std::string toPrefix(::std::string op, ::std::string sub = "",
                       ::std::string pa = "");

/**
 * Adds an operator to an existing prefix by adding it directly after
 * constants::TL_SEP.
 *
 * @param prefix a string containing constants::TL_SEP,
 * constants::CONSTRAINT_SEP and constants::BASE_SEP
 * @param op operator, gets placed between TL_SEP and the first CONSTRAINT_SEP
 *           of \a prefix
 * @param sub next operator, gets placed after \a op separated by CONSTRAINT_SEP
 * @return pa (from prefix) + TL_SEP + \a op + CONSTRAINT_SEP + \a sub + rest of
 *  \a prefix
 */
::std::string addToPrefix(::std::string prefix, ::std::string op,
                          ::std::string sub = "");
/**
 * Adds a base id to an existing id.
 *
 * This becomes necessary when building the product automaton of two TAs.
 * Typically not called directly but rather via mergeIds().
 *
 * @param id full id of a state
 * @param to_add base id of another sate (suffix after constants::BASE_SEP)
 * @return \a id + constants::COMPONENT_SEP + \a to_add
 */
::std::string addToBaseId(::std::string id, ::std::string to_add);

/**
 * Combines the action labels of two transitions.
 *
 * This becomes necessary when building the product automaton of two TAs.
 * The ordering of the parameters plays a role and should correspond to the
 * ordering specified when merging state ids together via mergeIds().
 *
 * @param action1 action label of one transition
 * @param action2 action label of another transition
 * @return \a action1 + constants::ACTION_SEP + \a action2
 */
::std::string mergeActions(::std::string action1, ::std::string action2);
/**
 * Merges two state ids together.
 *
 * @param id1 id to be merged into
 * @param id2 id to merge
 * @return id1 with id2's base id appended and id2's prefix (between
 *         constants::TL_SEP and constants::BASE_SEP) added to it's own prefix
 */
::std::string mergeIds(::std::string id1, ::std::string id2);

/**
 * Construct a plan automaton from a sequential plan.
 *
 * The plan automaton further encodes start and end actions.
 * They are required to express constraints that scope before the first real
 * plan action or after the last one.
 *
 * @param plan sequential plan to encode as automaton
 * @param name automaton name for the resulting TA
 */
Automaton generatePlanAutomaton(const ::std::vector<PlanAction> &plan,
                                ::std::string name);
} // namespace encoderutils
} // end namespace taptenc
