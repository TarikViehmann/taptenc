#include "printer.h"
#include "timed_automata.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

using namespace taptenc;

std::string XTAPrinter::toStringXTA(const State &s) {
  std::stringstream res;
  res << s.id;
  if (s.inv != "") {
    res << " {" << s.inv << "}";
  }
  return res.str();
}

std::string XTAPrinter::toStringXTA(const Transition &t) {
  std::stringstream res;
  res << t.source_id << " -> " << t.dest_id << " { ";
  if (t.sync != "" && t.passive)
    res << "sync " << t.sync << "?"
        << "; ";
  if (t.sync != "" && t.passive == false)
    res << "sync " << t.sync << "!"
        << "; ";
  if (t.guard != "")
    res << "guard " << t.guard << "; ";
  if (t.update != "")
    res << "assign " << t.update << "; ";
  res << "}";
  return res.str();
}

void XTAPrinter::printXTAstart(const AutomataSystem &s, std::string filename) {
  std::ofstream myfile;
  myfile.open(filename);
  std::unordered_set<std::string> all_clocks;
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

void XTAPrinter::printXTAtemplate(const AutomataSystem &s, int index,
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

void XTAPrinter::printXTAsystem(
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

void XTAPrinter::print(const AutomataSystem &s, SystemVisInfo &s_vis_info,
                       std::string filename) {
  printXTAstart(s, filename);
  for (auto it = s.instances.begin(); it != s.instances.end(); ++it) {
    printXTAtemplate(s, it - s.instances.begin(), filename);
  }
  std::cout << "useless output: " << s_vis_info.getStatePos(0, "").first
            << std::endl;
  printXTAsystem(s.instances, filename);
}