#include "printer.h"
#include "timed_automata.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace taptenc;

std::string XMLPrinter::toString(const State &s,
                                 const std::pair<int, int> &pos) {
  std::stringstream res;
  res << "<location id=\"" << s.id << "\" x=\"" << pos.first << "\" y=\""
      << pos.second << "\">";
  if (s.id != "") {
    res << "<name x=\"" << pos.first << "\" y=\"" << pos.second - 20 << "\">"
        << s.id << "</name>";
  }
  if (s.inv != "") {
    res << "<label kind=\"invariant\" x=\"" << pos.first << "\" y=\""
        << pos.second + 10 << "\">" << s.inv << "</label>";
  }
  if (s.urgent) {
    res << "<urgent/>" << std::endl;
  }
  res << "</location>";
  return res.str();
}

std::string XMLPrinter::toString(const Transition &t,
                                 const std::vector<std::pair<int, int>> &v) {
  std::stringstream res;
  if (v.size() == 0) {
    std::cout
        << "XMLPrinter toString(Transition): unexpected empty std::vector, "
           "mid_point missing!"
        << std::endl;
    return "";
  }
  res << "<transition>";
  res << "<source ref=\"" << t.source_id << "\"/>";
  res << "<target ref=\"" << t.dest_id << "\"/>";
  if (t.sync != "" && t.passive)
    res << "<label kind=\"synchronisation\" x=\"" << v[0].first << "\" y=\""
        << v[0].second + 10 << "\">" << t.sync << "?"
        << "</label>" << std::endl;
  if (t.sync != "" && t.passive == false)
    res << "<label kind=\"synchronisation\" x=\"" << v[0].first << "\" y=\""
        << v[0].second + 10 << "\">" << t.sync << "!"
        << "</label>" << std::endl;
  if (t.guard != "")
    res << "<label kind=\"guard\" x=\"" << v[0].first << "\" y=\""
        << v[0].second - 20 << "\">" << t.guard << "</label>" << std::endl;
  if (t.update != "")
    res << "<label kind=\"assignment\" x=\"" << v[0].first << "\" y=\""
        << v[0].second - 40 << "\">" << t.update << "</label>" << std::endl;
  if (v.size() > 0) {
    for (auto it = v.begin() + 1; it != v.end(); ++it) {
      res << "<nail x=\"" << it->first << "\" y=\"" << it->second << "\"/>"
          << std::endl;
    }
  }
  res << "</transition>" << std::endl;
  return res.str();
}

void XMLPrinter::printXMLstart(const AutomataGlobals &g, std::string filename) {
  std::ofstream myfile;
  myfile.open(filename);
  myfile << header;
  myfile << "<declaration>";
  if (g.clocks.size() > 0) {
    myfile << "clock ";
    for (auto it = g.clocks.begin(); it != g.clocks.end(); ++it) {
      if (it != g.clocks.begin()) {
        myfile << ", ";
      }
      myfile << *it;
    }
    myfile << "; " << std::endl;
  }
  bool empty = true;
  for (auto it = g.channels.begin(); it != g.channels.end(); ++it) {
    if (it->type == ChanType::Broadcast) {
      if (empty == false) {
        myfile << ", ";
      }
      if (empty == true) {
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
  for (auto it = g.channels.begin(); it != g.channels.end(); ++it) {
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
  myfile << "</declaration>" << std::endl;
  myfile.close();
}
void XMLPrinter::printXMLend(std::string filename) {
  std::ofstream myfile;
  myfile.open(filename, std::ios_base::app);
  myfile << "</nta>" << std::endl;
  myfile.close();
}

void XMLPrinter::printXMLtemplate(const AutomataSystem &s,
                                  SystemVisInfo &s_vis_info, int index,
                                  std::string filename) {
  std::ofstream myfile;
  myfile.open(filename, std::ios_base::app);
  myfile << "<template>";
  myfile << "<name x=\"0\" y=\"0\">" << s.instances[index].first.prefix
         << "</name>" << std::endl;
  myfile << "<declaration>" << std::endl;
  for (auto toplevelit = s.instances[index].first.clocks.begin();
       toplevelit != s.instances[index].first.clocks.end(); ++toplevelit) {
    myfile << "clock " << *toplevelit << ";" << std::endl;
  }
  for (auto toplevelit = s.instances[index].first.bool_vars.begin();
       toplevelit != s.instances[index].first.bool_vars.end(); ++toplevelit) {
    myfile << "bool " << *toplevelit << " = false;" << std::endl;
  }
  myfile << "</declaration>" << std::endl;
  bool initial_state_set = false;
  std::string init_id;
  for (auto toplevelit = s.instances[index].first.states.begin();
       toplevelit != s.instances[index].first.states.end(); ++toplevelit) {
    myfile << toString(*toplevelit,
                       s_vis_info.getStatePos(index, toplevelit->id))
           << std::endl;
    if (initial_state_set == false && toplevelit->initial == true) {
      init_id = toplevelit->id;
      initial_state_set = true;
    }
  }
  if (initial_state_set == false) {
    std::cout
        << "XMLPrinter printXMLTemplate: no initial state found (template: "
        << s.instances[index].first.prefix << ")" << std::endl;
    init_id = s.instances[index].first.states.begin()->id;
    initial_state_set = true;
  }
  myfile << "<init ref=\"" << init_id << "\"/>";
  for (auto toplevelit = s.instances[index].first.transitions.begin();
       toplevelit != s.instances[index].first.transitions.end(); ++toplevelit) {
    myfile << toString(*toplevelit,
                       s_vis_info.getTransitionPos(index, toplevelit->source_id,
                                                   toplevelit->dest_id))
           << std::endl;
  }
  myfile << "</template>" << std::endl;
  myfile.close();
}

void XMLPrinter::printXMLsystem(
    const std::vector<std::pair<Automaton, std::string>> &instances,
    std::string filename) {
  std::ofstream myfile;
  myfile.open(filename, std::ios_base::app);
  myfile << "<system>" << std::endl;
  std::string system = "system ";
  for (auto toplevelit = instances.begin(); toplevelit != instances.end();
       ++toplevelit) {
    if (toplevelit != instances.begin()) {
      system += ", ";
    }
    std::string component = "sys_" + toplevelit->first.prefix;
    myfile << component << " = " << toplevelit->first.prefix << "("
           << toplevelit->second << ");" << std::endl;
    system += component;
  }
  myfile << system << ";" << std::endl;
  myfile << "</system>" << std::endl;
  myfile.close();
}

void XMLPrinter::print(const AutomataSystem &s, SystemVisInfo &s_vis_info,
                       std::string filename) {
  printXMLstart(s.globals, filename);
  for (auto it = s.instances.begin(); it != s.instances.end(); ++it) {
    printXMLtemplate(s, s_vis_info, it - s.instances.begin(), filename);
  }
  printXMLsystem(s.instances, filename);
  printXMLend(filename);
}
