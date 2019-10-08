#pragma once

#include "timed_automata.h"
#include <string>
#include <vector>

namespace taptenc {
namespace encoderutils {
Automaton mergeAutomata(const ::std::vector<Automaton> &automata,
                        ::std::vector<Transition> &interconnections,
                        ::std::string prefix);
::std::vector<Transition>
createCopyTransitionsBetweenTAs(const Automaton &source, const Automaton &dest,
                                const ::std::vector<State> &filter,
                                ::std::string guard, ::std::string update,
                                ::std::string sync, bool passive = true);
::std::vector<Transition> createSuccessorTransitionsBetweenTAs(
    const Automaton &base, const Automaton &source, const Automaton &dest,
    const ::std::vector<State> &filter, ::std::string guard,
    ::std::string update);
void addTrapTransitions(Automaton &ta, const ::std::vector<State> &sources,
                        ::std::string guard, ::std::string update,
                        ::std::string sync, bool passive = true);
void addBaseSyncs(AutomataSystem &s, const int base_pos);
void addInvariants(Automaton &ta, const ::std::vector<State> filter,
                   ::std::string inv);
::std::string toPrefix(::std::string op, ::std::string sub = "",
                       ::std::string pa = "");
::std::string addToPrefix(::std::string prefix, ::std::string op,
                          ::std::string sub = "");
::std::string addToBaseId(::std::string id, ::std::string to_add);
::std::string mergeActions(::std::string action1, ::std::string action2);
::std::string mergeIds(::std::string id1, ::std::string id2);
Automaton generatePlanAutomaton(const ::std::vector<PlanAction> &plan,
                                ::std::string name);
} // namespace encoderutils
} // end namespace taptenc
