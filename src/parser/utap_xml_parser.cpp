/** \file
 * XML parser utilizing the utap lib parser
 *
 * \author (2019) Tarik Viehmann
 */
#include "utap_xml_parser.h"
#include "../constraints/constraints.h"
#include "../timed-automata/timed_automata.h"
#include "utap/typechecker.h"
#include "utap/utap.h"
#include "utils.h"
#include <iostream>
#include <memory>
#include <string>

using namespace taptenc;
/**
 * Utility functions to convert utap constraints to taptenc clock constraints.
 */
namespace constraintconverterutils {

/**
 * Obtain a taptenc comparison operator from a utap kind_t operator.
 *
 * @param op utap kind_t operator encoding
 * @return taptenc comparison operator corresponding to \a op.
 *         If kind_t is not a recognized comparison operator a warning is
 *         generated and ComparisonOp::LT is returned.
 */
ComparisonOp getComparisonOpFromUtapKind(UTAP::Constants::kind_t op) {
  switch (op) {
  case UTAP::Constants::LT:
    return ComparisonOp::LT;
  case UTAP::Constants::LE:
    return ComparisonOp::LTE;
  case UTAP::Constants::GT:
    return ComparisonOp::GT;
  case UTAP::Constants::GE:
    return ComparisonOp::GTE;
  case UTAP::Constants::EQ:
    return ComparisonOp::EQ;
  case UTAP::Constants::NEQ:
    return ComparisonOp::NEQ;
  default:
    std::cout << "unknown Utap operator" << std::endl;
    return ComparisonOp::LT;
  }
}

/**
 * Tries to parse an utap expression_t into a taptenc clock constraint.
 *
 * @param expr utap expression representing a clock constraint
 * @return clock constraint of \a expr. If the parsing fails for any reason,
 *         a pointer to a TrueCC constraint is returned.
 */
std::unique_ptr<ClockConstraint> parseUtapConstraint(UTAP::expression_t expr) {
  if (expr.toString() == "" || expr.toString() == "1") {
    return std::make_unique<TrueCC>(TrueCC());
  } else {
    switch (expr.getKind()) {
    case UTAP::Constants::LT:
    case UTAP::Constants::LE:
    case UTAP::Constants::GT:
    case UTAP::Constants::GE:
    case UTAP::Constants::EQ:
    case UTAP::Constants::NEQ: {
      if (expr.getSize() != 2) {
        std::cout << "error parsing utap expression, expected 2 "
                     "subexpressions, but got "
                  << expr.getSize() << std::endl;
        return std::make_unique<TrueCC>(TrueCC());
      }
      std::shared_ptr<Clock> occ1;
      std::shared_ptr<Clock> occ2;
      timepoint rhs = 0;
      size_t clock_expr_count = 0;
      size_t constants_count = 0;
      bool flip_comparison_op;
      for (size_t i = 0; i < expr.getSize(); i++) {
        if (expr[i].getKind() == UTAP::Constants::MINUS) {
          if (expr.getSize() != 2) {
            std::cout << "error parsing utap difference expression, expected 2 "
                         "subexpressions, but got "
                      << expr.getSize() << std::endl;
            return std::make_unique<TrueCC>(TrueCC());
          }
          clock_expr_count++;
          if (expr[i][0].getType().isClock() &&
              expr[i][1].getType().isClock()) {
            occ1 = std::make_shared<Clock>(expr[i][0].toString());
            occ2 = std::make_shared<Clock>(expr[i][1].toString());
          } else {
            std::cout << "error parsing difference expression, expected 2 "
                         "clocks, but got "
                      << expr[i][0].getType() << " and " << expr[i][1].getType()
                      << std::endl;
            return std::make_unique<TrueCC>(TrueCC());
          }
        } else if (expr[i].getType().isClock()) {
          clock_expr_count++;
          occ1 = std::make_shared<Clock>(expr[i].toString());
        } else if (expr[i].getKind() == UTAP::Constants::CONSTANT) {
          rhs = expr.getValue();
          if (i != expr.getSize() - 1) {
            flip_comparison_op = true;
          }
          constants_count++;
        } else {
          std::cout << "error parsing comparison constraint" << std::endl;
          return std::make_unique<TrueCC>(TrueCC());
        }
      }
      if (constants_count != 1 || clock_expr_count != 1) {
        std::cout << "error parsing comparison constraint. Only Comparisons "
                     "beteen constants and clock expressions are supported."
                  << std::endl;
        return std::make_unique<TrueCC>(TrueCC());
      }
      ComparisonOp op = getComparisonOpFromUtapKind(expr.getKind());
      if (flip_comparison_op) {
        op = computils::reverseOp(op);
      }
      if (occ2.get() == nullptr) {
        return std::make_unique<ComparisonCC>(ComparisonCC(occ1, op, rhs));
      } else {
        return std::make_unique<DifferenceCC>(
            DifferenceCC(occ1, occ2, op, rhs));
      }
      break;
    }
    case UTAP::Constants::AND:
      return std::make_unique<ConjunctionCC>(
          ConjunctionCC(*parseUtapConstraint(expr[0]).get(),
                        *parseUtapConstraint(expr[1]).get()));
    default:
      return std::make_unique<TrueCC>(TrueCC());
    }
  }
}
} // end namespace constraintconverterutils

using namespace constraintconverterutils;

AutomataSystem utapxmlparser::readXMLSystem(::std::string filename) {
  if (filename.find(".xml") == std::string::npos) {
    filename += ".xml";
  }
  AutomataSystem res;
  UTAP::TimedAutomataSystem input_system;
  parseXMLFile(filename.c_str(), &input_system, true);
  UTAP::TypeChecker type_checker(&input_system);
  if (input_system.hasErrors()) {
    std::cout << "UTAPSystemParser readXMLSystem: Abort, input system contains "
                 "errors: "
              << std::endl;
    for (const auto &err : input_system.getErrors()) {
      std::cout << "\t"
                << "at line " << err.start.line << ", " << err.start.position
                << "to " << err.end.line << ", " << err.end.position
                << std::endl;
      std::cout << "\t"
                << "msg: " << err.message << std::endl;
    }
    return res;
  }
  auto processes = input_system.getProcesses();
  if (processes.size() == 0) {
    std::cout << "UTAPSystemParser readXMLSystem: Abort, input system empty or "
                 "wrong filename"
              << std::endl;
  }
  for (const auto &t : processes) {
    std::vector<State> states;
    std::vector<Transition> transitions;
    for (const auto &s : t.templ->states) {
      bool is_init = false;
      if (t.templ->init.getName() == s.uid.getName()) {
        is_init = true;
      }
      std::string state_name = toLowerCase(s.uid.getName());
      if (state_name != s.uid.getName()) {
        std::cout << "UTAPSystemParser readXMLSystem: Detected upper case "
                     "characters in state name: "
                  << s.uid.getName() << " converted to " << state_name;
      }
      states.push_back(State(s.uid.getName(),
                             *parseUtapConstraint(s.invariant).get(), false,
                             is_init));
    }
    for (const auto &tr : t.templ->edges) {
      std::string source_id = toLowerCase(tr.src->uid.getName());
      std::string dest_id = toLowerCase(tr.dst->uid.getName());
      transitions.push_back(Transition(
          source_id, dest_id, "", *parseUtapConstraint(tr.guard).get(),
          // TODO fix this
          //(tr.assign.toString().size() == 1) ? {} : tr.assign.toString(),
          {}, tr.sync.toString()));
    }
    Automaton curr_ta(states, transitions, t.uid.getName(), false);
    for (const auto &tr : t.templ->variables) {
      if (tr.uid.getType().isClock()) {
        if (tr.uid.getName() != "t(0)") {
          std::string curr_clock = toLowerCase(tr.uid.getName());
          if (curr_clock != tr.uid.getName()) {
            std::cout << "UTAPSystemParser readXMLSystem: Detected upper case "
                         "characters in clock name: "
                      << tr.uid.getName()
                      << " THIS IS NOT SUPPORTED and will lead to errors."
                      << std::endl;
          }
          curr_ta.clocks.insert(std::make_shared<Clock>(curr_clock));
        }
      } else {
        std::cout << "UTAPSystemParser readXMLSystem: process declarations "
                     "must only contain clocks! Found: "
                  << tr.uid.getType().toString() << std::endl;
      }
    }
    res.instances.push_back(std::make_pair(curr_ta, ""));
  }
  auto vars = input_system.getGlobals().variables;
  for (const auto &tr : vars) {
    if (tr.uid.getType().isClock()) {
      if (tr.uid.getName() != "t(0)") {
        res.globals.clocks.insert(std::make_shared<Clock>(tr.uid.getName()));
      }
    } else {
      std::cout << "UTAPSystemParser readXMLSystem: global declarations must "
                   "only contain clocks! Found: "
                << tr.uid.getType().toString() << std::endl;
    }
  }
  return res;
}
