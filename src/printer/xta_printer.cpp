/** \file
 * Printer to produce uppaal 3.0 xta output format.
 *
 * xta is an output format that only contains the syntactical definitions of an
 * automata system. Display information such as state coordinates are printed
 * to a different file with file extension ugi. This is not implemented yet,
 * however, newer versions of uppaal do not require an ugi file.
 *
 * \author (2019) Tarik Viehmann
 */

#include "../timed-automata/timed_automata.h"
#include "printer.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

using namespace taptenc;

namespace taptenc {
namespace xtaprinterutils {

/**
 * Creates an xta formatted string (according to uppaal 3.0 syntax) that
 * holds information of a state.
 *
 * @param s state to xml format
 * @return xml formatted string of \a s accoding to uppaal 3.0 xml format
 */
std::string toStringXTA(const State &s) {
  std::stringstream res;
  res << s.id;
  if (s.inv.get()->type != CCType::TRUE) {
    res << " {" << s.inv.get()->toString() << "}";
  }
  return res.str();
}

/**
 * Creates an xta formatted string (according to uppaal 3.0 syntax) that holds
 * information of a transition.
 *
 * Nails to shape transitions are not supported.
 *
 * @param t transition to xta format
 * @return xta formatted string of \a s accoding to uppaal 3.0 format
 */
std::string toStringXTA(const Transition &t) {
  std::stringstream res;
  res << t.source_id << " -> " << t.dest_id << " { ";
  if (t.guard.get()->type != CCType::TRUE)
    res << "guard " << t.guard.get()->toString() << "; ";
  if (t.sync != "" && t.passive)
    res << "sync " << t.sync << "?"
        << "; ";
  if (t.sync != "" && t.passive == false)
    res << "sync " << t.sync << "!"
        << "; ";
  if (t.update.size() > 0)
    res << "assign " << t.updateToString() << "; ";
  res << "}";
  return res.str();
}

/**
 * Appends the global definitions formatted according to uppaal 3.0 xta syntax
 * to a file.
 *
 * @param s automata system
 * @param filename name of file to append the formatted info
 */
void printXTAstart(const AutomataSystem &s, std::string filename) {
  std::ofstream myfile;
  myfile.open(filename);
  std::unordered_set<std::shared_ptr<Clock>> all_clocks;
  for (auto it = s.globals.clocks.begin(); it != s.globals.clocks.end(); ++it) {
    auto emplaced = all_clocks.emplace(*it);
    if (emplaced.second == false) {
      std::cout << "XTAPrinter: Clock " << *it
                << " was defined multiple times in system globals "
                << std::endl;
    }
  }
  for (auto it = s.instances.begin(); it != s.instances.end(); ++it) {
    for (auto cl = it->first.clocks.begin(); cl != it->first.clocks.end();
         ++cl) {
      auto emplaced = all_clocks.emplace(*cl);
      if (emplaced.second == false) {
        std::cout << "XTAPrinter: Clock " << *cl
                  << " was defined multiple times (redefinition in "
                  << it->first.prefix << ")" << std::endl;
      }
    }
  }
  if (all_clocks.size() > 0) {
    myfile << "clock ";
    for (auto it = all_clocks.begin(); it != all_clocks.end(); ++it) {
      if (it != all_clocks.begin()) {
        myfile << ", ";
      }
      myfile << *it;
    }
    myfile << "; " << std::endl;
  }
  bool empty = true;

  for (auto it = s.globals.channels.begin(); it != s.globals.channels.end();
       ++it) {
    if (it->type == ChanType::Broadcast) {
      if (empty == false) {
        myfile << ", ";
      }
      if (empty == true) {
        std::cout
            << "XTAPrinter: Broadcast channels are not supported in XTA files"
            << std::endl;
        myfile << "broadcast chan ";
      }
      myfile << it->name;
      empty = false;
    }
  }
  if (empty == false) {
    myfile << "; " << std::endl;
  }
  empty = true;
  for (auto it = s.globals.channels.begin(); it != s.globals.channels.end();
       ++it) {
    if (it->type == ChanType::Binary) {
      if (empty == false) {
        myfile << ", ";
      }
      if (empty == true) {
        myfile << "chan ";
      }
      myfile << it->name;
      empty = false;
    }
  }
  if (empty == false) {
    myfile << "; " << std::endl;
  }
  std::unordered_set<std::string> all_bool_vars;
  for (auto it = s.instances.begin(); it != s.instances.end(); ++it) {
    for (auto bv = it->first.bool_vars.begin(); bv != it->first.bool_vars.end();
         ++bv) {
      auto emplaced = all_bool_vars.emplace(*bv);
      if (emplaced.second == false) {
        std::cout << "XTAPrinter: bool variable " << *bv
                  << " was defined multiple times (redefinition in "
                  << it->first.prefix << ")" << std::endl;
      } else {
        myfile << "bool " << *bv << " = false;" << std::endl;
      }
    }
  }
  myfile.close();
}

/**
 * Appends a xml encoded automaton template to a file.
 *
 * Currently templates are not really supported, because of the modeling of
 * AutomataSystem::instances.
 *
 * @param s Automata System that contains the template in questiom
 * @param index template index in AutomataSystem::instances of \a s
 * @param filename name of file to append the formatted template
 */
void printXTAtemplate(const AutomataSystem &s, int index,
                      std::string filename) {
  std::ofstream myfile;
  myfile.open(filename, std::ios_base::app);
  myfile << "process " << s.instances[index].first.prefix << "("
         << s.instances[index].second << ") {" << std::endl;
  bool initial_state_set = false;
  std::string init_id;
  myfile << "state ";
  for (auto toplevelit = s.instances[index].first.states.begin();
       toplevelit != s.instances[index].first.states.end(); ++toplevelit) {
    if (toplevelit != s.instances[index].first.states.begin()) {
      myfile << ", ";
    }
    myfile << toStringXTA(*toplevelit);
    if (initial_state_set == false) {
      init_id = toplevelit->id;
      initial_state_set = true;
    }
  }
  myfile << ";" << std::endl;
  myfile << "init " << init_id << ";" << std::endl;
  myfile << "trans" << std::endl;
  for (auto toplevelit = s.instances[index].first.transitions.begin();
       toplevelit != s.instances[index].first.transitions.end(); ++toplevelit) {
    if (toplevelit != s.instances[index].first.transitions.begin()) {
      myfile << "," << std::endl;
    }
    myfile << "    " << toStringXTA(*toplevelit);
  }
  myfile << ";" << std::endl << "}" << std::endl;
  myfile.close();
}

/**
 * Appends xta encoding of a system declaration to file.
 *
 * Currently a non-empty parameter list of an entry in \a instances requires
 * the associated automaton to already contain the assigned parameter values.
 *
 * @param instances instances consisting of an automaton and a string
 *        containing the parameter list.
 * @param filename name of file to append the formatted system definition
 */
void printXTAsystem(
    const std::vector<std::pair<Automaton, std::string>> &instances,
    std::string filename) {
  std::ofstream myfile;
  myfile.open(filename, std::ios_base::app);
  std::string system = "system ";
  for (auto toplevelit = instances.begin(); toplevelit != instances.end();
       ++toplevelit) {
    if (toplevelit != instances.begin()) {
      system += ", ";
    }
    system += toplevelit->first.prefix;
  }
  myfile << system << ";" << std::endl;
  myfile.close();
}
} // end namespace xtaprinterutils
} // end namespace taptenc

void XTAPrinter::print(const AutomataSystem &s, SystemVisInfo &s_vis_info,
                       std::string filename) {
  xtaprinterutils::printXTAstart(s, filename);
  for (auto it = s.instances.begin(); it != s.instances.end(); ++it) {
    xtaprinterutils::printXTAtemplate(s, it - s.instances.begin(), filename);
  }
  std::cout << "useless output: " << s_vis_info.getStatePos(0, "").first
            << std::endl;
  xtaprinterutils::printXTAsystem(s.instances, filename);
}
