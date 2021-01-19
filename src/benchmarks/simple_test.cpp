#include "constants.h"
#include "encoders.h"
#include "filter.h"
#include "plan_ordered_tls.h"
#include "platform_model_generator.h"
#include "printer.h"
#include "tfd_plan_parser.h"
#include "timed_automata.h"
#include "transformation.h"
#include "uppaal_calls.h"
#include "utap_trace_parser.h"
#include "utap_xml_parser.h"
#include "vis_info.h"
#include <algorithm>
#include <assert.h>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace taptenc;
using namespace std;

::std::vector<PlanAction> getTestPlan() {
  Bounds instant_bounds(0, 10);
  Bounds abs_bounds(10, 50);
  ::std::vector<PlanAction> plan;
  plan.push_back(PlanAction(ActionName("startGstack", {"a", "b"}), abs_bounds,
                            abs_bounds));
  Bounds trivial_bounds(0, std::numeric_limits<int>::max());
  plan.push_back(
      PlanAction(ActionName("endGstack", {"a", "b"}), abs_bounds, abs_bounds));
  plan.push_back(PlanAction(ActionName("startGstack", {"a", "b"}), abs_bounds,
                            abs_bounds));
  plan.push_back(
      PlanAction(ActionName("endGstack", {"a", "b"}), abs_bounds, abs_bounds));
  return plan;
}
Automaton getTestTA() {
  vector<State> states;
  vector<Transition> transitions;
  states.push_back(State("off", TrueCC(), false, true));
  states.push_back(State("on", TrueCC()));
  transitions.push_back(
      Transition("off", "on", "power_on", TrueCC(), {}, "", true));
  transitions.push_back(
      Transition("on", "off", "power_off", TrueCC(), {}, "", true));
  Automaton test(states, transitions, "main", false);

  test.clocks.insert({std::make_shared<Clock>(constants::GLOBAL_CLOCK)});
  return test;
}

vector<unique_ptr<EncICInfo>> getTestConstraints(const Automaton &test_ta) {
  vector<unique_ptr<EncICInfo>> test_constraints;
  test_constraints.emplace_back(make_unique<BinaryInfo>(
      "testconstraint0", ICType::Until,
      vector<ActionName>({ActionName(
          "startGstack", {std::string() + constants::VAR_PREFIX + "o",
                          std::string() + constants::VAR_PREFIX + "t"})}),
      TargetSpecs(Bounds(2, 5), {test_ta.states[1]}),
      vector<State>({test_ta.states[0]})));
  return test_constraints;
}

int main() {
  /* initialize random seed: */
  srand(time(NULL));
  if (uppaalcalls::getEnvVar("VERIFYTA_DIR") == "") {
    cout << "ERROR: VERIFYTA_DIR not set!" << endl;
    return -1;
  }
  // Init the Automata:
  vector<string> system_names({"sys_test"});
  vector<Automaton> platform_tas({getTestTA()});
  // Init the constraints
  vector<vector<unique_ptr<EncICInfo>>> platform_constraints;
  platform_constraints.emplace_back(getTestConstraints(platform_tas[0]));

  XMLPrinter printer;
  vector<uppaalcalls::timedelta> time_observed;
  vector<PlanAction> plan = getTestPlan();
  auto res = taptenc::transformation::transform_plan(plan, platform_tas,
                                                     platform_constraints);
  for (const auto &entry : res) {
    std::cout << entry.first << " : ";
    for (const auto &act : entry.second) {
      std::cout << act << " ";
    }
    std::cout << std::endl;
  }
  return 0;
}
