/** \file
 * Benchmark taptenc on the household robot domain.
 *
 * \author (2021) Tarik Viehmann
 */
#include "constraints.h"
#include "encoders.h"
#include "platform_model_generator.h"
#include "tfd_plan_parser.h"
#include "timed_automata.h"
#include "transformation.h"
#include "uppaal_calls.h"
#include "utils.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace taptenc;
using namespace taptenc::householdmodels;
using namespace std;

int main(int argc, char **argv) {
  auto t1 = std::chrono::high_resolution_clock::now();
  string planfile;
  string solutionfile;
  if (argc != 3) {
    cout << "usage: ./household <planfile> <resultfile>\nwhere <planfile> is a "
            "solution file from Temporal Fast Downward."
         << endl;
    return 0;
  } else {
    planfile = argv[1];
    solutionfile = argv[2];
  }
  /* initialize random seed: */
  srand(time(NULL));
  if (uppaalcalls::getEnvVar("VERIFYTA_DIR") == "") {
    cout << "ERROR: VERIFYTA_DIR not set!" << endl;
    return -1;
  }
  // Init the Automata:
  vector<string> system_names({"sys_test"});
  vector<Automaton> platform_tas({generateHouseholdTA()});
  // Init the constraints
  vector<vector<unique_ptr<EncICInfo>>> platform_constraints;
  platform_constraints.emplace_back(
      generateHouseholdConstraints(platform_tas[0]));

  vector<PlanAction> plan = tfdplanparser::insertWaitActions(
      tfdplanparser::readSequentialPlan(planfile));
  auto res = taptenc::transformation::transform_plan(plan, platform_tas,
                                                     platform_constraints);
  ofstream myfile(solutionfile);
  if (myfile.is_open()) {
    for (const auto &entry : res) {
      for (const auto &act : entry.second) {
        if (act.find("wait") == string::npos &&
            act.find("no_op") == string::npos) {
          std::string action = subBackSpecialChars(act);
          action = action.substr(
              0, action.find_first_of(string(1, constants::PA_SEP)));
          myfile << entry.first.earliest_start << ": " << action << endl;
        }
      }
    }
    myfile.close();
  } else {
    cout << "Unable to open output file";
    return -2;
  }
  auto t2 = std::chrono::high_resolution_clock::now();
  std::cout
      << "total time taptenc: "
      << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
      << " (ms)" << std::endl;
  return 0;
}
