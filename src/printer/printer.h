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
public:
  void print(const AutomataSystem &s, SystemVisInfo &s_vis_info,
             ::std::string filename);
};

class XTAPrinter : Printer {
private:
  void printXTAstart(const AutomataSystem &s, ::std::string filename);
  void printXTAtemplate(const AutomataSystem &s, int index,
                        ::std::string filename);
  void
  printXTAsystem(const ::std::vector<::std::pair<Automaton, ::std::string>> &s,
                 ::std::string filename);
  void printUGI(const AutomataSystem &s, SystemVisInfo &s_vis_info,
                ::std::string filename);
  ::std::string toStringXTA(const State &s);
  ::std::string toStringXTA(const Transition &t);
  ::std::string toStringUGI(const State &s, const ::std::pair<int, int> &pos);
  ::std::string toStringUGI(const Transition &t,
                            const ::std::vector<::std::pair<int, int>> &poi);

public:
  void print(const AutomataSystem &s, SystemVisInfo &s_vis_info,
             ::std::string filename);
};
} // end namespace taptenc
