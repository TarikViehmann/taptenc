/** \file
 * Interface to call uppaal tools.
 *
 * Since there is no powerful C++ API the tools have to be invoked via system
 * calls.
 *
 * @author (2019) Tarik Viehmann
 */
#include "uppaal_calls.h"
#include "printer/printer.h"
#include "timed-automata/timed_automata.h"
#include "utils.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace taptenc {
namespace uppaalcalls {
void deleteEmptyLines(const std::string &file_name) {
  std::string bufferString = "";

  // file
  std::fstream fileStream;
  std::string currentReadLine;
  fileStream.open(file_name, std::fstream::in); // open the file in Input mode

  // Read all the lines till the end of the file
  while (getline(fileStream, currentReadLine)) {
    // Check if the line is empty
    if (!currentReadLine.empty()) {
      currentReadLine = trim(currentReadLine);
      bufferString = bufferString + currentReadLine + "\n";
    }
  }
  fileStream.close();
  fileStream.open(file_name,
                  std::ios::out |
                      std::ios::trunc); // open file in Output mode. This line
                                        // will delete all data inside the file.
  fileStream << bufferString;
  fileStream.close();
}

std::string getEnvVar(std::string const &key) {
  char *val = std::getenv(key.c_str());
  if (val == NULL) {
    std::cout << "Environment Variable " << key << " not set!" << std::endl;
    return "";
  }
  return std::string(val);
}

void solve(const AutomataSystem &sys, std::string file_name,
           std::string query_str) {
  XMLPrinter printer;
  SystemVisInfo sys_vis_info(sys);
  printer.print(sys, sys_vis_info, file_name + ".xml");
  solve(file_name, query_str);
}
void solve(std::string file_name, std::string query_str) {
  std::ofstream myfile;
  myfile.open(file_name + ".q", std::ios_base::trunc);
  myfile << query_str;
  myfile.close();
  std::string call_get_if = "UPPAAL_COMPILE_ONLY=1 " +
                            getEnvVar("VERIFYTA_DIR") + "/verifyta " +
                            file_name + ".xml - > " + file_name + ".if";
  std::system(call_get_if.c_str());
  std::string call_get_trace = getEnvVar("VERIFYTA_DIR") +
                               "/verifyta -t 2  -f " + file_name + " -Y " +
                               file_name + ".xml " + file_name + ".q";
  std::system(call_get_trace.c_str());
  deleteEmptyLines(file_name + "-1.xtr");
  std::string call_make_trace_readable = "tracer " + file_name + ".if " +
                                         file_name + "-1.xtr > " + file_name +
                                         ".trace";
  std::system(call_make_trace_readable.c_str());
}
} // end namespace uppaalcalls
} // end namespace taptenc
