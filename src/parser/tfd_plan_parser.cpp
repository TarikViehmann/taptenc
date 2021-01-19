/** \file
 * Utility functions to parse plans generated from temporal fast downward.
 *
 * \author: (2021) Tarik Viehmann
 */

#include "tfd_plan_parser.h"
#include "../constraints/constraints.h"
#include <fstream>
#include <iostream>

using namespace taptenc;

namespace tfdplanparserutils {
PlanAction parseAction(const std::string &currline) {
  std::string rest_of_line = currline;
  size_t eow = rest_of_line.find_first_of(" \t");
  // skip absolute time
  rest_of_line = rest_of_line.substr(eow + 1);
  eow = rest_of_line.find_first_of(" ");
  // get "(<action-name>" and drop the opening bracket
  std::string action_name =
      rest_of_line.substr(0, eow).substr(1, std::string::npos);
  std::cout << "action name: " << action_name << std::endl;
  // skip action name
  rest_of_line = rest_of_line.substr(eow + 1);
  std::vector<std::string> action_args;
  bool args_parsed = false;
  if (action_name.back() != ')') {
    while (!args_parsed) {
      // get next arg
      eow = rest_of_line.find_first_of(" ");
      std::string curr_arg = rest_of_line.substr(0, eow);
      // if last arg, strip the trailing ")"
      if (curr_arg.back() == ')') {
        args_parsed = true;
        curr_arg.pop_back();
      }
      action_args.push_back(curr_arg);
      // skip the parsed arg
      rest_of_line = rest_of_line.substr(eow + 1);
    }
  } else {
    action_name.pop_back();
  }
  eow = rest_of_line.find_first_of("[");
  // skip gap to action duration
  rest_of_line = rest_of_line.substr(eow + 1);
  // obtain duration stream (rest of line minus the closing "]"
  std::string action_duration_str =
      rest_of_line.substr(0, rest_of_line.length() - 1);
  timepoint duration = 0;
  try {
    duration = static_cast<timepoint>(std::stof(action_duration_str));
  } catch (...) {
    std::cout << "TFDPlanParser::parseAction cannot parse timepoint "
              << action_duration_str << "in line\n"
              << currline << std::endl;
  }
  return PlanAction(ActionName(action_name, action_args), Bounds(),
                    Bounds(duration, duration));
}

} // namespace tfdplanparserutils

std::vector<PlanAction> tfdplanparser::readSequentialPlan(::std::string file) {
  std::vector<PlanAction> res;
  std::fstream fileStream;
  std::string currentReadLine;
  fileStream.open(file, std::fstream::in); // open the file in Input mode
  while (getline(fileStream, currentReadLine)) {
    if (getline(fileStream, currentReadLine)) {
      res.push_back(tfdplanparserutils::parseAction(currentReadLine));
    }
  }
  fileStream.close();
  return res;
}
