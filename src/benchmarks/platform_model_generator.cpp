/** \file
 * Hand-crafted benchmark generator utils.
 *
 * \author (2019) Tarik Viehmann
 */
#include "platform_model_generator.h"
#include "constants.h"
#include "constraints/constraints.h"
#include "encoder/enc_interconnection_info.h"
#include "timed-automata/timed_automata.h"
#include "utils.h"
#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <time.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace taptenc;
using namespace std;

/**
 * Returns an iterator pointing to the state with matching id.
 *
 * @param states states to search
 * @param id id to find
 * @return iterator pointing to the state (or iterator to end)
 */
std::vector<State>::const_iterator
getStateItById(const std::vector<State> &states, std::string id) {
  return std::find_if(states.begin(), states.end(),
                      [id](const State &s) { return s.id == id; });
}

Automaton benchmarkgenerator::generateCalibrationTA() {
  vector<State> states;
  vector<Transition> transitions;
  std::shared_ptr<Clock> cal_clock = std::make_shared<Clock>("cal");
  states.push_back(State("precise", TrueCC(), false, true));
  states.push_back(State("use1", TrueCC()));
  states.push_back(State("semi_precise", TrueCC()));
  states.push_back(State("use2", TrueCC()));
  states.push_back(State("uncalibrated", TrueCC()));
  states.push_back(
      State("calibrate", ComparisonCC(cal_clock, ComparisonOp::LTE, 20)));
  transitions.push_back(
      Transition("precise", "use1", "no_op", TrueCC(), {}, "", true));
  transitions.push_back(
      Transition("use1", "semi_precise", "no_op", TrueCC(), {}, "", true));
  transitions.push_back(
      Transition("semi_precise", "use2", "no_op", TrueCC(), {}, "", true));
  transitions.push_back(
      Transition("use2", "uncalibrated", "no_op", TrueCC(), {}, "", true));
  transitions.push_back(Transition("uncalibrated", "calibrate", "calibrate",
                                   TrueCC(), {cal_clock}, "", true));
  transitions.push_back(Transition("semi_precise", "calibrate", "calibrate",
                                   TrueCC(), {cal_clock}, "", true));
  transitions.push_back(
      Transition("calibrate", "precise", "no_op",
                 ComparisonCC(cal_clock, ComparisonOp::GTE, 20), {}, "", true));
  Automaton test(states, transitions, "main", false);

  test.clocks.insert(
      {cal_clock, std::make_shared<Clock>(constants::GLOBAL_CLOCK)});
  return test;
}

Automaton benchmarkgenerator::generatePerceptionTA() {
  vector<State> states;
  vector<Transition> transitions;
  std::shared_ptr<Clock> cam_clock = std::make_shared<Clock>("cam");
  std::shared_ptr<Clock> icp_clock = std::make_shared<Clock>("icp");
  std::shared_ptr<Clock> glob_clock =
      std::make_shared<Clock>(constants::GLOBAL_CLOCK);
  states.push_back(State("cam_off", TrueCC(), false, true)); // 0
  states.push_back(                                          // 1
      State("cam_boot", ComparisonCC(cam_clock, ComparisonOp::LTE, 6)));
  states.push_back(State("cam_on", TrueCC())); // 2
  states.push_back(                            // 3
      State("take_pic", ComparisonCC(cam_clock, ComparisonOp::LTE, 1)));
  states.push_back( // 4
      State("start_icp", ComparisonCC(icp_clock, ComparisonOp::LTE, 10)));
  states.push_back( // 5
      State("end_icp", ComparisonCC(icp_clock, ComparisonOp::LTE, 1)));
  states.push_back( // 6
      State("puck_check", ComparisonCC(cam_clock, ComparisonOp::LTE, 1)));
  transitions.push_back(Transition("cam_off", "cam_boot", "turn_on", TrueCC(),
                                   {cam_clock, icp_clock}, "", true));
  transitions.push_back(
      Transition("cam_boot", "cam_on", "no_op",
                 ComparisonCC(cam_clock, ComparisonOp::GTE, 4),
                 {cam_clock, icp_clock}, "", true));
  transitions.push_back(Transition(
      "cam_on", "take_pic", "save_pic",
      ComparisonCC(icp_clock, ComparisonOp::GTE, 5),
      // ConjunctionCC(ComparisonCC(icp_clock, ComparisonOp::GTE, 5),
      //               DifferenceCC(icp_clock, cam_clock, ComparisonOp::LT, 0)),
      {icp_clock, cam_clock}, "", true));
  transitions.push_back(
      Transition("take_pic", "cam_on", "upload", TrueCC(), {}, "", true));
  transitions.push_back(Transition("cam_on", "start_icp", "icp_start", TrueCC(),
                                   {icp_clock}, "", true));
  transitions.push_back(Transition(
      "start_icp", "end_icp", "publish",
      ComparisonCC(icp_clock, ComparisonOp::GTE, 5), {icp_clock}, "", true));
  transitions.push_back(
      Transition("end_icp", "cam_on", "no_op",
                 ComparisonCC(icp_clock, ComparisonOp::LTE, 1), {}, "", true));
  transitions.push_back(Transition("cam_on", "cam_off", "turn_off", TrueCC(),
                                   {cam_clock, icp_clock}, "", true));
  transitions.push_back(
      Transition("cam_off", "puck_check", "check_puck",
                 ComparisonCC(cam_clock, ComparisonOp::GTE, 2),
                 {cam_clock, icp_clock}, "", true));
  transitions.push_back(
      Transition("puck_check", "cam_off", "no_op", TrueCC(), {}, "", true));
  Automaton test(states, transitions, "main", false);

  test.clocks.insert({icp_clock, cam_clock, glob_clock});
  return test;
}

Automaton benchmarkgenerator::generateCommTA(::std::string machine) {
  vector<State> comm_states;
  vector<Transition> comm_transitions;
  std::shared_ptr<Clock> send_clock = std::make_shared<Clock>("send");
  comm_states.push_back(State("idle", TrueCC(), false, true));
  comm_states.push_back(
      State("prepare", ComparisonCC(send_clock, ComparisonOp::LTE, 30)));
  comm_states.push_back(
      State("prepared", ComparisonCC(send_clock, ComparisonOp::LTE, 0)));
  comm_transitions.push_back(Transition("idle", "prepare",
                                        "instruct_" + machine, TrueCC(),
                                        {send_clock}, "", true));
  comm_transitions.push_back(Transition(
      "prepare", "prepared", "",
      ComparisonCC(send_clock, ComparisonOp::GTE, 30), {send_clock}, "", true));
  comm_transitions.push_back(
      Transition("prepared", "idle", "", TrueCC(), {}, "", true));
  Automaton comm_ta =
      Automaton(comm_states, comm_transitions, "comm_ta", false);
  comm_ta.clocks.insert(
      {send_clock, std::make_shared<Clock>(constants::GLOBAL_CLOCK)});
  return comm_ta;
}

vector<unique_ptr<EncICInfo>> benchmarkgenerator::generatePerceptionConstraints(
    const Automaton &perception_ta) {
  unordered_set<string> cam_off_exceptions{"pick", "put", "endpick", "endput"};
  vector<State> pic_filter;
  vector<State> vision_filter;
  vector<State> no_vision_filter;
  vector<State> cam_off_filter;
  vector<State> cam_on_filter;
  vector<State> puck_sense_filter;
  pic_filter.insert(pic_filter.end(), {perception_ta.states[3]});
  vision_filter.insert(vision_filter.end(), {perception_ta.states[5]});
  no_vision_filter.insert(no_vision_filter.end(),
                          {perception_ta.states[0], perception_ta.states[1],
                           perception_ta.states[2], perception_ta.states[3],
                           perception_ta.states[6]});
  cam_off_filter.insert(cam_off_filter.end(),
                        {perception_ta.states[0], perception_ta.states[6]});
  cam_on_filter.insert(cam_on_filter.end(),
                       {perception_ta.states[1], perception_ta.states[2],
                        perception_ta.states[3], perception_ta.states[4],
                        perception_ta.states[5]});
  puck_sense_filter.insert(puck_sense_filter.end(), {perception_ta.states[6]});
  Bounds full_bounds(0, numeric_limits<int>::max());
  Bounds vision_bounds(5, 10); // numeric_limits<int>::max());
  Bounds puck_sense_bounds(0, 5);
  Bounds remain_bounds(10, 10);
  Bounds end_bounds(0, 5);
  Bounds null_bounds(0, 0);
  // ---------------------- Normal Constraints ------------------------------
  vector<unique_ptr<EncICInfo>> activations;
  // puck sense at begin and end of goto
  // easy variant: NoOp constraint, hard variant: Future constraint
  activations.emplace_back(make_unique<UnaryInfo>(
      "sense_n", ICType::Future,
      vector<ActionName>(
          {ActionName("startgoto",
                      {std::string() + constants::VAR_PREFIX + "m",
                       std::string() + constants::VAR_PREFIX + "n"}),
           ActionName("endgoto",
                      {std::string() + constants::VAR_PREFIX + "m",
                       std::string() + constants::VAR_PREFIX + "n"})}),
      TargetSpecs(puck_sense_bounds, puck_sense_filter)));
  // until chain to do icp
  activations.emplace_back(make_unique<ChainInfo>(
      "icp_c", ICType::UntilChain,
      vector<ActionName>(
          {ActionName("startpick",
                      {std::string() + constants::VAR_PREFIX + "o",
                       std::string() + constants::VAR_PREFIX + "m"}),
           ActionName("startgetshelf",
                      {std::string() + constants::VAR_PREFIX + "o",
                       std::string() + constants::VAR_PREFIX + "m"}),
           ActionName("startput",
                      {std::string() + constants::VAR_PREFIX + "o",
                       std::string() + constants::VAR_PREFIX + "m"}),
           ActionName("startpay",
                      {std::string() + constants::VAR_PREFIX + "o",
                       std::string() + constants::VAR_PREFIX + "m"})}),
      vector<TargetSpecs>{// already be in start icp
                          TargetSpecs(full_bounds, {perception_ta.states[4]}),
                          // be done with icp and remain for 0 seconds
                          TargetSpecs(null_bounds, {perception_ta.states[5]}),
                          // do not do icp again until pick action is done
                          TargetSpecs(remain_bounds, no_vision_filter)},
      vector<ActionName>(
          {ActionName("endpick", {std::string() + constants::VAR_PREFIX + "o",
                                  std::string() + constants::VAR_PREFIX + "m"}),
           ActionName("endgetshelf",
                      {std::string() + constants::VAR_PREFIX + "o",
                       std::string() + constants::VAR_PREFIX + "m"}),
           ActionName("endput", {std::string() + constants::VAR_PREFIX + "o",
                                 std::string() + constants::VAR_PREFIX + "m"}),
           ActionName("endpay",
                      {std::string() + constants::VAR_PREFIX + "o",
                       std::string() + constants::VAR_PREFIX + "m"})})));
  activations.emplace_back(make_unique<ChainInfo>(
      "pic_c", ICType::UntilChain,
      vector<ActionName>(
          {ActionName("startpick",
                      {std::string() + constants::VAR_PREFIX + "o",
                       std::string() + constants::VAR_PREFIX + "m"}),
           ActionName("startgetshelf",
                      {std::string() + constants::VAR_PREFIX + "o",
                       std::string() + constants::VAR_PREFIX + "m"}),
           ActionName("startput",
                      {std::string() + constants::VAR_PREFIX + "o",
                       std::string() + constants::VAR_PREFIX + "m"}),
           ActionName("startpay",
                      {std::string() + constants::VAR_PREFIX + "o",
                       std::string() + constants::VAR_PREFIX + "m"})}),
      // take a pic somewhere during grasping tasks
      vector<TargetSpecs>{// first allow unrestrained control
                          TargetSpecs(full_bounds, perception_ta.states),
                          // then visit the state to take a pic
                          TargetSpecs(full_bounds, {perception_ta.states[3]}),
                          // then the platform is free to move again
                          TargetSpecs(full_bounds, perception_ta.states)},
      vector<ActionName>(
          {ActionName("endpick", {std::string() + constants::VAR_PREFIX + "o",
                                  std::string() + constants::VAR_PREFIX + "m"}),
           ActionName("endgetshelf",
                      {std::string() + constants::VAR_PREFIX + "o",
                       std::string() + constants::VAR_PREFIX + "m"}),
           ActionName("endput", {std::string() + constants::VAR_PREFIX + "o",
                                 std::string() + constants::VAR_PREFIX + "m"}),
           ActionName("endpay",
                      {std::string() + constants::VAR_PREFIX + "o",
                       std::string() + constants::VAR_PREFIX + "m"})})));
  // Have no running camera during driving
  activations.emplace_back(make_unique<ChainInfo>(
      "no_cam_c", ICType::UntilChain,
      vector<ActionName>({ActionName(
          "startgoto", {std::string() + constants::VAR_PREFIX + "m",
                        std::string() + constants::VAR_PREFIX + "n"})}),
      vector<TargetSpecs>{TargetSpecs(full_bounds, cam_off_filter)},
      vector<ActionName>({ActionName(
          "endgoto", {std::string() + constants::VAR_PREFIX + "m",
                      std::string() + constants::VAR_PREFIX + "n"})})));
  return activations;
}

vector<unique_ptr<EncICInfo>>
benchmarkgenerator::generateCommConstraints(const Automaton &comm_ta,
                                            ::std::string machine) {
  vector<State> comm_not_prepared_filter(
      {comm_ta.states[0], comm_ta.states[1]});

  Bounds full_bounds(0, numeric_limits<int>::max());

  // ---------------------- Until Chain -------------------------------------
  vector<unique_ptr<EncICInfo>> comm_constraints;
  // prepare the machine precisely once after put before pick or before plan
  // ends
  comm_constraints.emplace_back(make_unique<ChainInfo>(
      "prep_" + machine, ICType::UntilChain,
      vector<ActionName>({ActionName(
          "endput", {std::string() + constants::VAR_PREFIX + "o", machine})}),
      vector<TargetSpecs>{TargetSpecs(full_bounds, comm_not_prepared_filter),
                          TargetSpecs(full_bounds, {comm_ta.states[2]})},
      vector<ActionName>({
          ActionName("startpick",
                     {std::string() + constants::VAR_PREFIX + "o", machine}),
          ActionName("endplan", {std::string() + constants::VAR_PREFIX + "p",
                                 std::string() + constants::VAR_PREFIX + "q"}),
      })));
  // do not start any preparation in between picks and puts (or until plan ends
  // and before the first delivery to the machine)
  comm_constraints.emplace_back(make_unique<ChainInfo>(
      "prep_" + machine, ICType::UntilChain,
      vector<ActionName>({
          ActionName("endpick",
                     {std::string() + constants::VAR_PREFIX + "o", machine}),
          ActionName("startplan",
                     {std::string() + constants::VAR_PREFIX + "r",
                      std::string() + constants::VAR_PREFIX + "s"}),
      }),
      vector<TargetSpecs>{TargetSpecs(full_bounds, {comm_ta.states[0]})},
      vector<ActionName>({
          ActionName("endput",
                     {std::string() + constants::VAR_PREFIX + "o2", machine}),
          ActionName("endplan", {std::string() + constants::VAR_PREFIX + "p",
                                 std::string() + constants::VAR_PREFIX + "q"}),
      })));

  return comm_constraints;
}

vector<unique_ptr<EncICInfo>>
benchmarkgenerator::generateCalibrationConstraints(const Automaton &calib_ta) {
  vector<State> no_calib_filter({calib_ta.states[0], calib_ta.states[1],
                                 calib_ta.states[2], calib_ta.states[3],
                                 calib_ta.states[4]});
  vector<State> use_filter({calib_ta.states[1], calib_ta.states[3]});
  vector<State> no_use_filter({calib_ta.states[0], calib_ta.states[2],
                               calib_ta.states[4], calib_ta.states[5]});

  Bounds null_bounds(0, 0);
  Bounds full_bounds(0, numeric_limits<int>::max());

  // ---------------------- Until Chain -------------------------------------
  vector<unique_ptr<EncICInfo>> calib_activations;
  // do not be in a usage state during goto
  calib_activations.emplace_back(make_unique<ChainInfo>(
      "no_use_c", ICType::UntilChain,
      vector<ActionName>({ActionName(
          "startgoto", {std::string() + constants::VAR_PREFIX + "m",
                        std::string() + constants::VAR_PREFIX + "n"})}),
      vector<TargetSpecs>{TargetSpecs(full_bounds, no_use_filter)},
      vector<ActionName>({ActionName(
          "endgoto", {std::string() + constants::VAR_PREFIX + "m",
                      std::string() + constants::VAR_PREFIX + "n"})})));
  // do not calibrate between grasping tasks that place stuff into machines
  calib_activations.emplace_back(make_unique<ChainInfo>(
      "no_cal_c", ICType::UntilChain,
      vector<ActionName>(
          {ActionName("endpick", {std::string() + constants::VAR_PREFIX + "o",
                                  std::string() + constants::VAR_PREFIX + "m"}),
           ActionName("endgetshelf",
                      {std::string() + constants::VAR_PREFIX + "o",
                       std::string() + constants::VAR_PREFIX + "m"})}),
      vector<TargetSpecs>{TargetSpecs(full_bounds, no_calib_filter)},
      vector<ActionName>({ActionName(
          "startput", {std::string() + constants::VAR_PREFIX + "o",
                       std::string() + constants::VAR_PREFIX + "n"})})));
  // force best possible precision for slide puts
  calib_activations.emplace_back(make_unique<ChainInfo>(
      "force_cal_c", ICType::UntilChain,
      vector<ActionName>(
          {ActionName("endpick", {std::string() + constants::VAR_PREFIX + "o",
                                  std::string() + constants::VAR_PREFIX + "m"}),
           ActionName("endgetshelf",
                      {std::string() + constants::VAR_PREFIX + "o",
                       std::string() + constants::VAR_PREFIX + "m"})}),
      vector<TargetSpecs>{TargetSpecs(full_bounds, calib_ta.states),
                          TargetSpecs(full_bounds, {calib_ta.states[0]}),
                          TargetSpecs(null_bounds, calib_ta.states)},
      vector<ActionName>({ActionName(
          "startpay", {std::string() + constants::VAR_PREFIX + "o",
                       std::string() + constants::VAR_PREFIX + "n"})})));
  // remain in usage states during grasping tasks
  calib_activations.emplace_back(make_unique<ChainInfo>(
      "use_c", ICType::UntilChain,
      vector<ActionName>(
          {ActionName("startpick",
                      {std::string() + constants::VAR_PREFIX + "o",
                       std::string() + constants::VAR_PREFIX + "m"}),
           ActionName("startgetshelf",
                      {std::string() + constants::VAR_PREFIX + "o",
                       std::string() + constants::VAR_PREFIX + "m"}),
           ActionName("startput",
                      {std::string() + constants::VAR_PREFIX + "o",
                       std::string() + constants::VAR_PREFIX + "m"}),
           ActionName("startpay",
                      {std::string() + constants::VAR_PREFIX + "o",
                       std::string() + constants::VAR_PREFIX + "m"})}),
      vector<TargetSpecs>{TargetSpecs(full_bounds, use_filter)},
      vector<ActionName>(
          {ActionName("endpick", {std::string() + constants::VAR_PREFIX + "o",
                                  std::string() + constants::VAR_PREFIX + "m"}),
           ActionName("endgetshelf",
                      {std::string() + constants::VAR_PREFIX + "o",
                       std::string() + constants::VAR_PREFIX + "m"}),
           ActionName("endput", {std::string() + constants::VAR_PREFIX + "o",
                                 std::string() + constants::VAR_PREFIX + "m"}),
           ActionName("endpay",
                      {std::string() + constants::VAR_PREFIX + "o",
                       std::string() + constants::VAR_PREFIX + "m"})})));
  return calib_activations;
}

vector<unique_ptr<EncICInfo>>
benchmarkgenerator::generatePositionConstraints(const Automaton &pos_ta) {
  vector<unique_ptr<EncICInfo>> pos_constraints;
  vector<State> moving_filter;
  vector<State> standing_filter;
  auto standing_state = getStateItById(pos_ta.states, "standing");
  auto moving_state = getStateItById(pos_ta.states, "moving");
  if (standing_state == pos_ta.states.end() ||
      moving_state == pos_ta.states.end()) {
    std::cout << "benchmarkgenerator generatePositionConstraints: cannot find "
                 "constraint relevant states"
              << std::endl;
    return pos_constraints;
  } else {
    moving_filter.push_back(*moving_state);
    standing_filter.push_back(*standing_state);

    Bounds no_bounds(0, numeric_limits<int>::max());

    // pos_constraints.emplace_back(make_unique<UnaryInfo>(
    //     "pos_standing", ICType::Invariant,
    //     vector<string>({"pick", "put", "getshelf", "pay", "end"}),
    //     TargetSpecs(no_bounds, standing_filter)));
    // pos_constraints.emplace_back(make_unique<UnaryInfo>(
    //     "pos_moving", ICType::Invariant, vector<string>({"goto"}),
    //     TargetSpecs(no_bounds, moving_filter)));

    return pos_constraints;
  }
}
