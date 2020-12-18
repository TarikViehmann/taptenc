/** \file
 * Datastructures to represent clock constraints.
 *
 * \author (2019) Tarik Viehmann
 */

#pragma once

#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace taptenc {

/**
 * Datatype of timepoints. Requires std::numeric_limits< >::max() to be
 * defined.
 */
using timepoint = int;

/**
 * Clock representation.
 */
struct clock {
  std::string id;
  clock(::std::string arg_id);
};
typedef clock Clock;

enum ComparisonOp {
  /** <= or &lt;= in html encoding */
  LTE,
  /** < or &lt; in html encoding */
  LT,
  /** > or &gt; in html encoding */
  GT,
  /** >= or &gt;= in html encoding */
  GTE,
  /** == */
  EQ,
  /** != */
  NEQ
};
namespace computils {
/**
 * Returns the html encoding of the a comparison operator with leading and
 * trailing whitespace.
 *
 * @param op comparison operator to get htm encoded string frm
 * @return html encoding of \a op with leading and trailing whitespace
 */
::std::string toString(const ComparisonOp op);

/**
 * Reverses a comparison operator (reading from right to left instead left to
 * right).
 *
 * E.g. <= becomes >=.
 *
 * @param op comparison operator to reverse
 * @return reversed comparison operator of \a op
 */
ComparisonOp reverseOp(ComparisonOp op);

/**
 * Inverts a comparison operator (negating it).
 *
 * E.g. <= becomes >.
 *
 * @param op comparison operator to invert
 * @return inverted comparison operator of \a op
 */
ComparisonOp inverseOp(ComparisonOp op);
} // end namespace computils

/**
 * Clock Constraint types.
 */
enum CCType {
  /** Conjunction of exactly two clock constraints. */
  CONJUNCTION,
  /** Difference constraint (comparison of the clock difference of two clocks
   * against a constant.
   */
  DIFFERENCE,
  /**
   * Comparison of a clock against a constant.
   */
  SIMPLE_BOUND,
  /** Trivial constraint, that is always true. */
  TRUE,
  /** Arbitrary constraint stored as string. */
  UNPARSED
};

/**
 * Clock constraint base class. Represents the basic clock constraints of
 * timed automata syntax, but not the extended expressiveness of uppaal.
 */
struct clockConstraint {
  /** Holds the real type concrete object type. */
  CCType type;

  virtual ~clockConstraint() = default;

  /** Copies interface to allow clock constraint copies extended by more
   * constraints during encoding.
   */
  virtual ::std::unique_ptr<struct clockConstraint> createCopy() const = 0;
  /**
   * Converts constraints into the proper format for xml files.
   *
   * Operators &, <, > are replaced by their respective html encodings.
   */
  virtual ::std::string toString() const = 0;
};
typedef struct clockConstraint ClockConstraint;

/** True constraint. */
struct trueCC : public ClockConstraint {
  ::std::unique_ptr<struct clockConstraint> createCopy() const;
  ::std::string toString() const;
  trueCC();
};
typedef struct trueCC TrueCC;

/**
 * Unparsed constraint stored as string.
 *
 * Properly parsing a constraint from a string is not implemented yet. This can
 * be used as workaround when parsing from a string would be required or a
 * constraint has a form that is not representable with the available structs.
 */
struct unparsedCC : public ClockConstraint {
  ::std::string raw_cc;
  ::std::unique_ptr<struct clockConstraint> createCopy() const;
  ::std::string toString() const;
  unparsedCC(::std::string cc_string);
};
typedef struct unparsedCC UnparsedCC;

/**
 * Conjunction of exactly two clock constraints. Nesting is possible to
 * express arbitrary long conjunctions.
 */
struct conjunctionCC : public ClockConstraint {
  /** Subexpressions that form the conjunction. */
  std::pair<std::unique_ptr<clockConstraint>, std::unique_ptr<clockConstraint>>
      content;

  ::std::unique_ptr<ClockConstraint> createCopy() const;
  ::std::string toString() const;

  /**
   * Creates a new conjunction given two clock constraints.
   *
   * Copies the input clock constraints to form a completely independent
   * constraint.
   *
   * @param first lhs of the conjunction
   * @param second rhs of the conjunction
   */
  conjunctionCC(const ClockConstraint &first, const ClockConstraint &second);
};
typedef struct conjunctionCC ConjunctionCC;

/**
 * Comparison of a clock against a constant. The constant is always on the rhs
 * of the comparison operator.
 */
struct comparisonCC : public ClockConstraint {
  ::std::shared_ptr<Clock> clock;
  ComparisonOp comp;
  timepoint constant;
  /**
   * Creates a comparison of a clock on the left side against a constant on
   * the right side.
   *
   * @param arg_clock clock that forms the lhs of the comparison
   * @param arg_comp comparison operator
   * @param constant constant that forms the rhs of the comparison
   */
  comparisonCC(::std::shared_ptr<Clock> arg_clock, ComparisonOp arg_comp,
               timepoint constant);

  ::std::unique_ptr<ClockConstraint> createCopy() const;
  ::std::string toString() const;
};
typedef struct comparisonCC ComparisonCC;

/**
 * Comparison of a clock difference against a constant. The constant is always
 * on the rhs of the comparison operator.
 */
struct differenceCC : public ClockConstraint {
  ::std::shared_ptr<Clock> minuend;
  ::std::shared_ptr<Clock> subtrahend;
  ComparisonOp comp;
  timepoint difference;
  /**
   * Creates a comparison of a clock difference on the left side against a
   * constant on the right side.
   *
   * @param arg_minuend clock that forms the lhs of - operator on the lhs of
   *                    the comparison
   * @param arg_subtrahend clock that forms the rhs of - operator on the lhs of
   *                       the comparison
   * @param arg_comp comparison operator
   * @param arg_difference constant that forms the rhs of the comparison
   */
  differenceCC(::std::shared_ptr<Clock> arg_minuend,
               ::std::shared_ptr<Clock> arg_subtrahend, ComparisonOp arg_comp,
               timepoint arg_difference);

  ::std::unique_ptr<ClockConstraint> createCopy() const;
  ::std::string toString() const;
};
typedef struct differenceCC DifferenceCC;

/**
 * Wrapper to store lower and upper bounds.
 *
 * Read as lower bound {<,<=} content {<,<=} upper bound.
 * The content it self is not part of this datatype.
 */
struct bounds {
  ComparisonOp l_op;
  ComparisonOp r_op;
  timepoint lower_bound;
  timepoint upper_bound;

  /**
   * Creates default bounds (l = 0, u = tmax)
   */
  bounds();

  /**
   * Creates non-strct bounds.
   *
   * @param l lower bound
   * @param u upper bound
   */
  bounds(timepoint l, timepoint u);

  /**
   * Creates bounds with user defined comparison operatos.
   *
   * Only valid bounds can be created. Bounds are invalid if their operators are
   * neither ComparisonOp::LT nor ComparisonOp::LTE or if the upper bound is
   * infinity but the upper bound operator is non-strict.
   *
   * @param l lower bound
   * @param u upper bound
   * @param arg_l_op lower bound operator (ComparisonOp::LT or ComparisonOp::LTE
   *                 expected)
   * @param arg_r_op upper bound operator (ComparisonOp::LT or ComparisonOp::LTE
   *                 expected)
   */
  bounds(timepoint l, timepoint u, ComparisonOp arg_l_op,
         ComparisonOp arg_r_op);

  /**
   * Creates a constraint stating that the given clock is  within the bounds.
   *
   * Substitutes trivial lower and upper bound by TrueCC().
   *
   * @param clock_ptr clock to formulate constraint over
   * @return conjunction stating that the \a clock_ptr is within the bounds
   */
  ConjunctionCC
  createConstraintBoundsSat(const ::std::shared_ptr<Clock> &clock_ptr) const;

  bool operator==(const bounds &other);
  bool operator!=(const bounds &other);
};
typedef struct bounds Bounds;

/**
 * Action Name.
 */
struct actionName {
  ::std::string id;
  ::std::vector<::std::string> args;
  actionName(::std::string id, const ::std::vector<::std::string> &args);
  ::std::string toString() const;
  actionName ground(::std::vector<::std::string> ground_args) const;
};
typedef struct actionName ActionName;
/**
 * Plan action.
 */
struct planAction {
  ActionName name;
  Bounds absolute_time;
  Bounds duration;
  /** execution time that the plan transformation determines. */
  timepoint execution_time;
  /**
   * The amount of tolerable delay when executing the selected action.
   */
  Bounds delay_tolerance;
  /**
   * Creates a plan action.
   *
   * Inits execution time to 0 and delay tolerance to be
   * unbounded [0,infinity).
   *
   * @param arg_name name of the plan action
   * @param arg_absolute_time absolute time of action start
   * @param arg_duration action duration
   */
  planAction(ActionName name, const Bounds &arg_absolute_time,
             const Bounds &arg_duration);
};
typedef struct planAction PlanAction;

} // namespace taptenc
