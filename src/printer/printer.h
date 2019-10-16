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
public:
  void print(const AutomataSystem &s, SystemVisInfo &s_vis_info,
             ::std::string filename);
};
} // end namespace taptenc
