/** \file
 * Encodes temporal constraints by creating copies of the platform TA to
 * represent different timelines.
 *
 * \author: (2019) Tarik Viehmann
 */
#pragma once
#include "../timed-automata/timed_automata.h"
#include "../timed-automata/vis_info.h"
#include "enc_interconnection_info.h"
#include "encoder_utils.h"
#include "filter.h"
#include "plan_ordered_tls.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace taptenc {
typedef ::std::unordered_map<::std::string, ::std::string> OrigMap;
/**
 * Encodes temporal constraints by creating copies of the platform TAs to
 * represent different timelines.
 *
 * The basic idea is to represent a the operation of a platform TA during a plan
 * by building a cross product between the plan TA and the platform TA.
 * Constraints may either be encoded by deleting/modifying exisitng states and
 * transitions (e.g. in case of an invariant during a plan action) or have a
 * window where the constraint restricts the platform usage and that does not
 * coincide with an action begin or duration. In this case one can create one
 * timeline (= copy of the exisiting construction) to represent the time window
 * in which the constraint may be active and add transitions to it according to
 * the activation window.
 *
 * ~~~
 * Example schematics of such a construction:
 * Step 1: Plan with actions P1, P2, P3
 *         =>
 *         Plan Automaton: P1->P2->P3
 * Step 2: Component Automaton M and Plan Automaton
 *         =>
 *         M->M->M
 * Step 3: Add designated query state q
 *         =>
 *         M->M->M->q
 * Step 4: Constraint "be in part M1 of M sometimes after P2 started"
 *         =>
 *         M->M ->M       <-- Timeline where the constraint is not satisfied yet
 *            |   |
 *            v   v
 *            M1->M1->q   <-- Timeline where the constraint is satisfied
 * Result: any a run that reaches q satisfies the
 *         constraint
 * ~~~
 *
 * The resulting automaton system may consist of many states and clock, but the
 * reachability query that has to be solved to obtain a valid plan
 * transformation is simple.
 */
class DirectEncoder {
private:
  /** The datastructure to represent the encoding.*/
  PlanOrderedTLs po_tls;
  /**
   * Plan with action durations.
   *
   * Required to calculate the context of constraints (see calculateContext()).
   */
  ::std::vector<PlanAction> plan;
  /** Stores the position of the plan TA inside the automata system used to
   * construct the encoder instance.
   */
  size_t plan_ta_index;
  /**
   * Counts the number of encoded constraints.
   *
   * Used to ensure unique clock names to avoid problems when one constraint
   * fires multiple times during a plan with overlapping active window.
   */
  size_t encode_counter = 0;
  /**
   * Creates a product TA between platform TA and plan TA.
   *
   * The result is stored within \a po_tls, so as seperate automata copies.
   *~~~
   * Let M be a platform model TA. a1,...,an plan actions.
   * Plan action  :  a1  a2  a3  ...   an
   * Base Timeline:  M - M - M - ... - M
   *
   * po_tls[ai][ai].ta = M
   *~~~
   * @param s Automata system that contains the platform model and the plan
   * @param base_pos position of the platform model in
   *        AutomataSystem::instances of \a s
   * \param plan_pos position of the plan in AutomataSystem::instances of \a s
   */
  void generateBaseTimeLine(AutomataSystem &s, const int base_pos,
                            const int plan_pos);

  /**
   * Calculate the window of plan actions during which a temporal constraint is
   * active.
   *
   * @param specs the time constraints to determine the context of
   * @param starting_pa the plan action which triggers the constraint activation
   * @param ending_pa If specified, gives a bound of the window.
   *                  The context ends before the specified ending plan
   *                  action.
   * @param look_ahead determines the direction to build the window.
   *                   true: context starts from \a starting_pa and propagates
   *                         to the future
   *                   false: context propagates to the past of \a starting_pa
   * @param lb_offset initial lower bound of time that is already elapsed from
   *                  \a starting_pa.
   * @param ub_offset initial upper bound of time that is already elapsed from
   *                  \a starting_pa.
   */
  ::std::pair<int, int> calculateContext(const TargetSpecs &specs,
                                         ::std::string starting_pa,
                                         ::std::string ending_pa = "",
                                         bool look_ahead = true,
                                         int lb_offset = 0, int ub_offset = 0);

  /**
   * Constructs a direct encoder by copying the the arguments to member
   * variables.
   *
   * Only internally used.
   */
  DirectEncoder(const PlanOrderedTLs &tls,
                const ::std::vector<PlanAction> &plan,
                const size_t plan_ta_index);

public:
  size_t getPlanTAIndex();
  DirectEncoder copy();

  /**
   * Encode an until chain.
   *
   * An until chain is classified by a sequence of target states with
   * associated temporal constraints:
   *~~~
   * Example:
   * Let M1, M2, M3 be a sequence of target states with bounds [1,2] each
   * Let P0, P1, P2, P3, P4 be a sequence of plan actions with bounds [1,1] each
   * Let P1 be the starting_pa and P4 be the ending_pa
   * Plan    :P0 P1 P2 P3 P4
   * Encoding: M-M1-M1    M
   *                |     |
   *                M2-M2 |
   *                   | /
   *                   M3
   *~~~
   * The encoding is done within the \a tls member and is not applied to the
   * automata system.
   *
   * @param s automata system containing the platform model
   * @param info temporal information on the different parts of the chain
   * @param start_pa plan action where the until chain is activated
   * @param end_pa plan action that ends the until chain
   * @param base_pos position of the platform TA in AutomataSystem::instances
   *        of \a s
   */
  void encodeUntilChain(AutomataSystem &s, const ChainInfo &info,
                        const ::std::string start_pa,
                        const ::std::string end_pa, const int base_pos = 0);

  /**
   * Encode an Invariant.
   * An invariant is classified by a set of target states and a plan action
   * where it is applied, meaning that during the plan action only the target
   * states can be visited.
   *
   * @param s automata system containing the platform model
   * @param targets states from the platform model that form the invariant
   * @param pa plan action name where the invariant should be encoded at
   */
  void encodeInvariant(AutomataSystem &s, const ::std::vector<State> &targets,
                       const ::std::string pa);
  /**
   * Encodes a constraint expressing that upon start of a plan action some
   * target states should be reached.
   *
   * After the states are reached the platform can leave those target state even
   * while the plan action is still running (which is the difference compared to
   * the invariant encoding, see encodeInvariant())
   *
   * @param s automata system containing the platform model
   * @param targets states from the platform model
   * @param pa plan action name upon which start the target state have to be
   *           reached
   */
  void encodeNoOp(AutomataSystem &s, const ::std::vector<State> &targets,
                  const ::std::string pa);
  /**
   * Encodes a basic constraint stating that in the future some target states
   * should be met.
   *
   *~~~
   * Example:
   * Let M1 be the target states with bounds [2,3]
   * Let P0, P1, P2, P3, P4 be a sequence of plan actions with bounds [1,1] each
   * Let P1 be the starting_pa
   * Plan    : P0 P1 P2 P3 P4
   * Encoding: M--M--M--M  M
   *                 !  ! /
   *                 M--M
   *
   * where ! denotes transitions to the target states M1 within M
   *~~~
   *
   * @param s automata system containing the platform model
   * @param pa plan action name upon which start the target state have to be
   * @param info states from the platform model that should be reached together
   *        with time bounds
   * @param base_index index of the platform TA in AutomataSystem::instances of
   * \a s
   * @param add_succ_trans adds successor transitions, if set to true, which
   *        is not necessary to encode future constraints, hence it is false per
   *default
   */
  void encodeFuture(AutomataSystem &s, const ::std::string pa,
                    const UnaryInfo &info, int base_index = 0,
                    bool add_succ_trans = false);
  /**
   * Encodes a constraint stating that upon reaching some plan action the
   * platform should be first in one set of target states and then later,
   * within some bounds in another set of target states.
   *
   *~~~
   * Example:
   * Let M1 and M2 be the target states with bounds [2,3]
   * Let P0, P1, P2, P3, P4 be a sequence of plan actions with bounds [1,1] each
   * Let P1 be the starting_pa
   * Plan    : P0 P1 P2 P3 P4
   * Encoding: M--M1-M1-M1 M
   *                 !  ! /
   *                 M--M
   *
   * where ! denotes transitions to the target states M2 within M
   *~~~
   *
   * @param s automata system containing the platform model
   * @param pa plan action name upon which start the constraint fires
   * @param info states from the platform model that should be reached together
   *        with time bounds
   * @param base_index index of the platform TA in AutomataSystem::instances of
   * \a s
   */
  void encodeUntil(AutomataSystem &s, const ::std::string pa,
                   const BinaryInfo &info, int base_index = 0);

  /**
   * Encodes a constraint stating that upon reaching some plan action the
   * platform should have been in set of target states since
   * within some bounds it got there from another set of target states.
   *
   *~~~
   * Example:
   * Let M1 and M2 be the target states with bounds [2,3]
   * Let P0, P1, P2, P3, P4 be a sequence of plan actions with bounds [1,1] each
   * Let P4 be the starting_pa
   * Plan    : P0 P1 P2 P3 P4
   * Encoding: M--M--M     M
   *              !  !    /
   *              M1-M1-M1
   *
   * where ! denotes transitions from the target states M2 within M
   *~~~
   *
   * @param s automata system containing the platform model
   * @param pa plan action name upon which start the constraint fires
   * @param info states from the platform model that should be reached together
   *        with time bounds
   * @param base_index index of the platform TA in AutomataSystem::instances of
   * \a s
   */
  void encodeSince(AutomataSystem &s, const ::std::string pa,
                   const BinaryInfo &info, int base_index = 0);

  /**
   * Encodes a constraint stating that upon reaching some plan action the
   * platform should have been in set of target states in the past within some
   * bounds.
   *
   *~~~
   * Example:
   * Let M1 be the target states with bounds [2,3]
   * Let P0, P1, P2, P3, P4 be a sequence of plan actions with bounds [1,1] each
   * Let P4 be the starting_pa
   * Plan    : P0 P1 P2 P3 P4
   * Encoding: M--M--M     M
   *              !  !    /
   *              M--M--M
   *
   * where ! denotes transitions from the target states M1 wiithin M
   *~~~
   *
   * @param s automata system containing the platform model
   * @param pa plan action name upon which start the constraint fires
   * @param info states from the platform model that should be reached together
   *        with time bounds
   * @param base_index index of the platform TA in AutomataSystem::instances of
   * \a s
   * @param add_succ_trans adds successor transitions, if set to true, which
   *        is not necessary to encode past constraints, hence it is false per
   *default
   */
  void encodePast(AutomataSystem &s, const ::std::string pa,
                  const UnaryInfo &info, int base_index = 0,
                  bool add_succ_trans = false);

  /**
   * Creates a DirectEncoder instance based of an automata system containing a
   * platform model and a sequential plan.
   *
   * The resulting instance can then be used without further preparation to
   * encode constraints.
   *
   * @param s automata system containing the platform model
   * @param plan sequential plan, assuming the ordering in the vector
   *             corresponds to the ordering within the plan
   * @param base_pos position of the platform model TA in \a s
   */
  DirectEncoder(AutomataSystem &s, const ::std::vector<PlanAction> &plan,
                const int base_pos = 0);

  DirectEncoder() = default;

  /**
   * Converts the PlanOrderedTLs representation of the encoding to an automata
   * systen containing only one automaton and creates visual information for
   * that system.
   *
   * Before converting the representation, iterative pruning is applied to
   * ignore automata copies without outgoing transitions.
   *
   * @param s automata system containing the platform model and plan automaton
   * @param s_vis instance to store the visual information associated with the
   *              output automata system
   * @return automata system containing the automata that contains all encoding
   *         information
   */
  AutomataSystem createFinalSystem(const AutomataSystem &s,
                                   SystemVisInfo &s_vis);
  /**
   * Create a DirectEncoder Instance containing a merged encoding of this and
   * the argument encoding.
   *
   * Mergin is done by constructing the product automata between encodings.
   * The resulting encoder should not be used for further encoding steps except
   * for further merges.
   *
   * @param enc2 encoding to merge with
   * @return encoder containing the merge between \a this and \a enc2
   */
  DirectEncoder mergeEncodings(const DirectEncoder &enc2) const;
};
} // end namespace taptenc
