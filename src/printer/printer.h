/** \file
 * Printer classes for different output file formats.
 *
 * \author (2019) Tarik Viehmann
 */
#pragma once

#include "timed_automata.h"
#include "utils.h"
#include "vis_info.h"
#include <string>
#include <vector>

namespace taptenc {
/**
 * Interface to print an automata system to a file.
 *
 * Printers for different file formats can be realized by inheriting from this
 * base class.
 */
class Printer {
public:
  /**
   * Prints an automata system to a file according to some visualization
   * information.
   *
   * Subclasses of Printer are expected to implement this function such that a
   * simple call to this function yields a file in the file format realized by
   * the subclass.
   *
   * @param s automata system to print
   * @param s_vis_info visualization information for \a s
   * @param filename name of the resulting file including file extension
   */
  virtual void print(const AutomataSystem &s, SystemVisInfo &s_vis_info,
                     ::std::string filename) = 0;
};
/**
 * xml printer to produce xml files compatible with uppaal 4.0 syntax.
 *
 * See also #taptenc::xmlprinterutils for used helper functions.
 */
class XMLPrinter : Printer {
public:
  /**
   * Prints an automata system to a xml file compatible with uppaal 4.0 syntax.
   *  \copydetails Printer::print()
   */
  void print(const AutomataSystem &s, SystemVisInfo &s_vis_info,
             ::std::string filename);
};

/**
 * xta printer to produce xta files compatible with uppaal 3.0 syntax.
 *
 * See also #taptenc::xtaprinterutils for used helper functions.
 */
class XTAPrinter : Printer {
public:
  /**
   * Prints an automata system to a xta file compatible with uppaal 3.0 syntax.
   *
   * Does NOT print an associated ugi file containing display information,
   * because this is not supported yet.
   *
   *  \copydetails Printer::print()
   */
  void print(const AutomataSystem &s, SystemVisInfo &s_vis_info,
             ::std::string filename);
};
} // end namespace taptenc
