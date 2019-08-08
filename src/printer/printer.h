#pragma once

#include "timed_automata.h"
#include "utils.h"
#include "vis_info.h"
#include <string>
#include <vector>

namespace taptenc {
class Printer {
public:
  virtual void print(const AutomataSystem &s, SystemVisInfo &s_vis_info,
                     ::std::string filename) = 0;
};
class XMLPrinter : Printer {
private:
  ::std::string header =
      "<?xml version=\"1.0\" encoding=\"utf-8\"?><!DOCTYPE nta PUBLIC "
      "\'-//Uppaal Team//DTD Flat System 1.1//EN\' "
      "\'http://www.it.uu.se/research/group/darts/uppaal/flat-1_1.dtd\'><nta>";

  void printXMLstart(const AutomataGlobals &g, ::std::string filename);
  void printXMLend(::std::string filnemane);
  void printXMLtemplate(const AutomataSystem &s, SystemVisInfo &s_vis_info,
                        int index, ::std::string filename);
  void
  printXMLsystem(const ::std::vector<::std::pair<Automaton, ::std::string>> &s,
                 ::std::string filename);
  ::std::string toString(const State &s, const ::std::pair<int, int> &pos);
  ::std::string toString(const Transition &t,
                         const ::std::vector<::std::pair<int, int>> &poi);

public:
  void print(const AutomataSystem &s, SystemVisInfo &s_vis_info,
             ::std::string filename);
};
} // end namespace taptenc
