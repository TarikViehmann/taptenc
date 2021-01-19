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
#include <chrono>
#include <filesystem>
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

std::string exec(const char *cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
  if (!pipe)
    throw std::runtime_error("popen() failed!");
  while (!feof(pipe.get())) {
    if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
      result += buffer.data();
  }
  return result;
}

::std::vector<timedelta> solve(const AutomataSystem &sys, std::string file_name,
                               std::string query_str) {
  XMLPrinter printer;
  SystemVisInfo sys_vis_info(sys);
  printer.print(sys, sys_vis_info, file_name + ".xml");
  return solve(file_name, query_str);
}

::std::vector<timedelta> solve(std::string file_name, std::string query_str) {
  std::vector<timedelta> res;
  std::ofstream myfile;
  std::filesystem::remove(file_name + ".q");
  myfile.open(file_name + ".q", std::ios_base::trunc);
  myfile << query_str;
  myfile.close();
  std::filesystem::remove(file_name + ".if");
  std::string call_get_if = "UPPAAL_COMPILE_ONLY=1 " +
                            getEnvVar("VERIFYTA_DIR") + "/verifyta " +
                            file_name + ".xml - > " + file_name + ".if";

  auto t1 = std::chrono::high_resolution_clock::now();
  std::system(call_get_if.c_str());
  auto t2 = std::chrono::high_resolution_clock::now();
  res.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1));
  std::string call_get_trace = getEnvVar("VERIFYTA_DIR") +
                               "/verifyta -t 2  -f " + file_name + " -Y " +
                               file_name + ".xml " + file_name + ".q";
  std::filesystem::remove(file_name + "-1.xtr");
  t1 = std::chrono::high_resolution_clock::now();
  // std::system(call_get_trace.c_str());
  std::string output_str = exec(call_get_trace.c_str());
  t2 = std::chrono::high_resolution_clock::now();
  res.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1));
  if (output_str.find("formula is satisfied")) {

    t1 = std::chrono::high_resolution_clock::now();
    deleteEmptyLines(file_name + "-1.xtr");
    std::filesystem::remove(file_name + ".trace");
    std::string call_make_trace_readable = "tracer " + file_name + ".if " +
                                           file_name + "-1.xtr > " + file_name +
                                           ".trace";
    std::system(call_make_trace_readable.c_str());
    t2 = std::chrono::high_resolution_clock::now();
    res.push_back(
        std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1));
  }
  return res;
}
} // end namespace uppaalcalls
} // end namespace taptenc
