#include "utap_xml_parser.h"
#include "timed_automata.h"
#include "utap/typechecker.h"
#include "utap/utap.h"
#include "utils.h"
#include <iostream>
#include <string>
using namespace taptenc;
AutomataSystem utapxmlparser::readXMLSystem(std::string filename) {
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
      states.push_back(State(s.uid.getName(),
                             convertCharsToHTML(s.invariant.toString()), false,
                             is_init));
    }
    for (const auto &tr : t.templ->edges) {
      std::string guard = convertCharsToHTML(tr.guard.toString());
      if (guard.substr(0, 1) == "1" && guard.size() == 1) {
        guard = "";
      }
      transitions.push_back(Transition(
          tr.src->uid.getName(), tr.dst->uid.getName(), "", guard,
          (tr.assign.toString().size() == 1) ? "" : tr.assign.toString(),
          tr.sync.toString()));
    }
    Automaton curr_ta(states, transitions, t.uid.getName(), false);
    for (const auto &tr : t.templ->variables) {
      if (tr.uid.getType().isClock()) {
        if (tr.uid.getName() != "t(0)") {
          curr_ta.clocks.push_back(tr.uid.getName());
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
        res.globals.clocks.push_back(tr.uid.getName());
      }
    } else {
      std::cout << "UTAPSystemParser readXMLSystem: global declarations must "
                   "only contain clocks! Found: "
                << tr.uid.getType().toString() << std::endl;
    }
  }
  return res;
}
