/** \file
 * Datastructures to represent clock constraints.
 *
 * \author (2019) Tarik Viehmann
 */

#include "constraints.h"
#include "../constants.h"
#include <cassert>
#include <iostream>
#include <limits>
#include <memory>
#include <vector>

using namespace taptenc;

clock::clock(::std::string arg_id) : id(arg_id) {}

// Comparison Utils

::std::string computils::toString(const ComparisonOp comp) {
  switch (comp) {
  case LTE:
    return " &lt;= ";
    break;
  case LT:
    return " &lt; ";
    break;
  case GT:
    return " &gt; ";
    break;
  case GTE:
    return " &gt;= ";
    break;
  case EQ:
    return " == ";
    break;
  case NEQ:
    return " != ";
    break;
  default:
    return "unknown_op";
    break;
  }
}

ComparisonOp computils::reverseOp(ComparisonOp op) {
  if (op == ComparisonOp::LT)
    return ComparisonOp::GT;
  if (op == ComparisonOp::GT)
    return ComparisonOp::LT;
  if (op == ComparisonOp::LTE)
    return ComparisonOp::GTE;
  if (op == ComparisonOp::GTE)
    return ComparisonOp::LTE;
  if (op == ComparisonOp::EQ)
    return ComparisonOp::EQ;
  if (op == ComparisonOp::NEQ)
    return ComparisonOp::NEQ;
  std::cout << "reverseOp: unknown op:" << toString(op) << std::endl;
  return op;
}

ComparisonOp computils::inverseOp(ComparisonOp op) {
  if (op == ComparisonOp::LT)
    return ComparisonOp::GTE;
  if (op == ComparisonOp::GT)
    return ComparisonOp::LTE;
  if (op == ComparisonOp::LTE)
    return ComparisonOp::GT;
  if (op == ComparisonOp::GTE)
    return ComparisonOp::LT;
  if (op == ComparisonOp::EQ)
    return ComparisonOp::NEQ;
  if (op == ComparisonOp::NEQ)
    return ComparisonOp::EQ;
  std::cout << "inverseOp: unknown op:" << toString(op) << std::endl;
  return op;
}

// Constraints

// TrueCC
trueCC::trueCC() { type = CCType::TRUE; }

::std::string trueCC::toString() const { return ""; }

::std::unique_ptr<ClockConstraint> trueCC::createCopy() const {
  return std::make_unique<TrueCC>(TrueCC());
}

// UnparsedCC
unparsedCC::unparsedCC(::std::string cc_string) {
  type = CCType::UNPARSED;
  raw_cc = cc_string;
}

::std::string unparsedCC::toString() const { return raw_cc; }

::std::unique_ptr<ClockConstraint> unparsedCC::createCopy() const {
  return std::make_unique<UnparsedCC>(UnparsedCC(raw_cc));
}

// ConjunctionCC
conjunctionCC::conjunctionCC(const ClockConstraint &first,
                             const ClockConstraint &second) {
  ::std::unique_ptr<ClockConstraint> first_copy = first.createCopy();
  ::std::unique_ptr<ClockConstraint> second_copy = second.createCopy();
  content.first = std::move(first_copy);
  content.second = std::move(second_copy);
  type = CCType::CONJUNCTION;
}

::std::string conjunctionCC::toString() const {
  std::string res = "";
  std::string lhs = content.first->toString();
  std::string rhs = content.second->toString();
  res += lhs;
  if (lhs.size() > 0 && rhs.size() > 0) {
    res += " &amp;&amp; ";
  }
  res += rhs;
  return res;
}

::std::unique_ptr<ClockConstraint> conjunctionCC::createCopy() const {
  return std::make_unique<ConjunctionCC>(
      ConjunctionCC(*content.first, *content.second));
}

// ComparisonCC
comparisonCC::comparisonCC(::std::shared_ptr<Clock> arg_clock,
                           ComparisonOp arg_comp, timepoint arg_constant)
    : clock(arg_clock), comp(arg_comp), constant(arg_constant) {
  type = CCType::SIMPLE_BOUND;
}

::std::unique_ptr<ClockConstraint> comparisonCC::createCopy() const {
  ComparisonCC res(clock, comp, constant);
  return std::make_unique<ComparisonCC>(res);
}

::std::string comparisonCC::toString() const {
  if (constant == std::numeric_limits<int>::max() ||
      (constant == 0 && comp == ComparisonOp::GTE)) {
    return "";
  } else {
    return clock->id + computils::toString(comp) + std::to_string(constant);
  }
}

// DifferenceCC
differenceCC::differenceCC(::std::shared_ptr<Clock> arg_minuend,
                           ::std::shared_ptr<Clock> arg_subtrahend,
                           ComparisonOp arg_comp, timepoint arg_difference)
    : minuend(arg_minuend), subtrahend(arg_subtrahend), comp(arg_comp),
      difference(arg_difference) {
  type = CCType::DIFFERENCE;
}

::std::unique_ptr<ClockConstraint> DifferenceCC::createCopy() const {
  return std::make_unique<DifferenceCC>(
      DifferenceCC(minuend, subtrahend, comp, difference));
}

::std::string DifferenceCC::toString() const {
  return minuend.get()->id + " - " + subtrahend.get()->id +
         computils::toString(comp) + std::to_string(difference);
}

// Bounds

bounds::bounds()
: bounds(0, std::numeric_limits<timepoint>::max())
{}

bounds::bounds(timepoint l, timepoint u) {
  l_op = ComparisonOp::LTE;
  r_op = u == ::std::numeric_limits<timepoint>::max() ? ComparisonOp::LT
                                                      : ComparisonOp::LTE;
  lower_bound = l;
  upper_bound = u;
}

bounds::bounds(timepoint l, timepoint u, ComparisonOp arg_l_op,
               ComparisonOp arg_r_op) {
  assert(arg_l_op == ComparisonOp::LT || arg_l_op == ComparisonOp::LTE);
  assert(arg_r_op == ComparisonOp::LT || arg_r_op == ComparisonOp::LTE);
  assert(arg_r_op == ComparisonOp::LT ||
         u != std::numeric_limits<timepoint>::max());
  l_op = arg_l_op;
  r_op = arg_r_op;
  lower_bound = l;
  upper_bound = u;
}

ConjunctionCC bounds::createConstraintBoundsSat(
    const ::std::shared_ptr<Clock> &clock_ptr) const {
  bool lower_bounded = (lower_bound != 0 || l_op == ComparisonOp::LTE);
  bool upper_bounded = (upper_bound != std::numeric_limits<int>::max());
  std::unique_ptr<ClockConstraint> below_upper_bound =
      std::make_unique<TrueCC>(TrueCC());
  std::unique_ptr<ClockConstraint> lower_bound_reached =
      std::make_unique<TrueCC>(TrueCC());
  if (upper_bounded) {
    below_upper_bound =
        std::make_unique<ComparisonCC>(clock_ptr, r_op, upper_bound);
  }
  if (lower_bounded) {
    lower_bound_reached = std::make_unique<ComparisonCC>(
        clock_ptr, computils::reverseOp(l_op), lower_bound);
  }
  return ConjunctionCC(*below_upper_bound.get(), *lower_bound_reached.get());
}

bool bounds::operator == (const bounds &other) {
  return this->l_op == other.l_op
         && this->r_op == other.r_op
         && this->lower_bound == other.lower_bound
         && this->upper_bound == other.upper_bound;
}

bool bounds::operator != (const bounds &other) {
	return !(*this == other);
}

// Action Name
actionName::actionName(::std::string id,
                       const ::std::vector<::std::string> &args)
    : id(id), args(args) {}

::std::string actionName::toString() const {
  std::string res = id + constants::OPEN_BRACKET;
  bool first = true;
  for (const auto &arg : args) {
    if (!first) {
      res += constants::VAR_SEP + arg;
    } else {
      res += arg;
      first = false;
    }
  }
  return res + constants::CLOSED_BRACKET;
}
ActionName actionName::ground(::std::vector<::std::string> ground_args) const {
  if (ground_args.size() != args.size()) {
    return ActionName(id, args);
  }
  std::vector<std::string> res_args(args.size(), "");
  for (long unsigned int i = 0; i < args.size(); i++) {
    if (args[i][0] == constants::VAR_PREFIX) {
      res_args[i] = ground_args[i];
    } else {
      res_args[i] = args[i];
    }
  }
  return ActionName(id, res_args);
}

// Plan Action
planAction::planAction(ActionName arg_name, const Bounds &arg_absolute_time,
                       const Bounds &arg_duration)
    : name(arg_name), absolute_time(arg_absolute_time), duration(arg_duration),
      delay_tolerance(0, std::numeric_limits<timepoint>::max()) {}
