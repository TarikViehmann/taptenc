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

/**
 * Adds clock updates of special state clock to each transition.
 *
 * @param trans transitions to add clock updates to
 */
void addStateClock(vector<Transition> &trans,
                   const ::std::shared_ptr<Clock> &clock_ptr) {
  for (auto &t : trans) {
    t.update = addUpdate(t.update, {clock_ptr});
  }
}

Automaton benchmarkgenerator::generateCalibrationTA() {
  vector<State> states;
  vector<Transition> transitions;
  std::shared_ptr<Clock> cal_clock = std::make_shared<Clock>("cal");
  states.push_back(State("uncalibrated", TrueCC(), false, true));
  states.push_back(
      State("calibrating", ComparisonCC(cal_clock, ComparisonOp::LT, 10)));
  states.push_back(
      State("calibrated", ComparisonCC(cal_clock, ComparisonOp::LTE, 30)));
  transitions.push_back(Transition("uncalibrated", "calibrating", "calibrate",
                                   TrueCC(), {cal_clock}, "", true));
  transitions.push_back(Transition("calibrating", "calibrated", "",
                                   ComparisonCC(cal_clock, ComparisonOp::GT, 7),
                                   {cal_clock}, "", true));
  transitions.push_back(Transition(
      "calibrated", "uncalibrated", "",
      ComparisonCC(cal_clock, ComparisonOp::EQ, 30), {cal_clock}, "", true));
  transitions.push_back(Transition(
      "calibrated", "uncalibrated", "uncalibrate",
      ComparisonCC(cal_clock, ComparisonOp::LT, 30), {cal_clock}, "", true));
  std::shared_ptr<Clock> state_clock =
      std::make_shared<Clock>(constants::STATE_CLOCK);
  addStateClock(transitions, state_clock);
  Automaton test(states, transitions, "main", false);

  test.clocks.insert({cal_clock,
                      std::make_shared<Clock>(constants::GLOBAL_CLOCK),
                      state_clock});
  return test;
}

Automaton benchmarkgenerator::generatePerceptionTA() {
  vector<State> states;
  vector<Transition> transitions;
  std::shared_ptr<Clock> cam_clock = std::make_shared<Clock>("cam");
  std::shared_ptr<Clock> pic_clock = std::make_shared<Clock>("pic");
  std::shared_ptr<Clock> icp_clock = std::make_shared<Clock>("icp");
  std::shared_ptr<Clock> sense_clock = std::make_shared<Clock>("sense");
  states.push_back(State("idle", TrueCC(), false, true));
  states.push_back(
      State("cam_boot", ComparisonCC(cam_clock, ComparisonOp::LT, 5)));
  states.push_back(State("cam_on", TrueCC()));
  states.push_back(
      State("save_pic", ComparisonCC(pic_clock, ComparisonOp::LTE, 2)));
  states.push_back(
      State("icp_start", ComparisonCC(icp_clock, ComparisonOp::LTE, 10)));
  states.push_back(
      State("icp_end", ComparisonCC(icp_clock, ComparisonOp::LTE, 3)));
  states.push_back(
      State("puck_sense", ComparisonCC(sense_clock, ComparisonOp::LT, 5)));
  transitions.push_back(Transition("idle", "cam_boot", "power_on_cam", TrueCC(),
                                   {cam_clock}, "", true));
  transitions.push_back(Transition("cam_boot", "cam_on", "",
                                   ComparisonCC(cam_clock, ComparisonOp::GT, 2),
                                   {cam_clock}, "", true));
  transitions.push_back(Transition("cam_on", "save_pic", "store_pic", TrueCC(),
                                   {pic_clock}, "", true));
  transitions.push_back(Transition("save_pic", "cam_on", "",
                                   ComparisonCC(pic_clock, ComparisonOp::GT, 1),
                                   {}, "", true));
  transitions.push_back(Transition("cam_on", "icp_start", "start_icp", TrueCC(),
                                   {icp_clock}, "", true));
  transitions.push_back(
      Transition("icp_start", "icp_end", "",
                 ConjunctionCC(ComparisonCC(icp_clock, ComparisonOp::LT, 10),
                               ComparisonCC(icp_clock, ComparisonOp::GT, 5)),
                 {icp_clock}, "", true));
  transitions.push_back(Transition("icp_end", "cam_on", "",
                                   ComparisonCC(icp_clock, ComparisonOp::GT, 1),
                                   {}, "", true));
  transitions.push_back(Transition("cam_on", "idle", "power_off_cam", TrueCC(),
                                   {cam_clock}, "", true));
  transitions.push_back(Transition("idle", "puck_sense", "check_puck",
                                   ComparisonCC(cam_clock, ComparisonOp::GT, 2),
                                   {sense_clock}, "", true));
  transitions.push_back(
      Transition("puck_sense", "idle", "",
                 ComparisonCC(sense_clock, ComparisonOp::GT, 1), {}, "", true));
  std::shared_ptr<Clock> state_clock =
      std::make_shared<Clock>(constants::STATE_CLOCK);
  addStateClock(transitions, state_clock);
  Automaton test(states, transitions, "main", false);

  test.clocks.insert({icp_clock, cam_clock, pic_clock, sense_clock,
                      std::make_shared<Clock>(constants::GLOBAL_CLOCK),
                      state_clock});
  return test;
}

Automaton benchmarkgenerator::generateCommTA() {
  vector<State> comm_states;
  vector<Transition> comm_transitions;
  std::shared_ptr<Clock> send_clock = std::make_shared<Clock>("send");
  comm_states.push_back(State("idle", TrueCC(), false, true));
  comm_states.push_back(
      State("prepare", ComparisonCC(send_clock, ComparisonOp::LTE, 30)));
  comm_states.push_back(
      State("prepared", ComparisonCC(send_clock, ComparisonOp::LTE, 0)));
  comm_states.push_back(
      State("error", ComparisonCC(send_clock, ComparisonOp::LTE, 0)));
  comm_transitions.push_back(Transition("idle", "prepare", "send_prepare",
                                        TrueCC(), {send_clock}, "", true));
  comm_transitions.push_back(
      Transition("prepare", "prepared", "",
                 ConjunctionCC(ComparisonCC(send_clock, ComparisonOp::LT, 30),
                               ComparisonCC(send_clock, ComparisonOp::GT, 10)),
                 {send_clock}, "", true));
  comm_transitions.push_back(Transition(
      "prepare", "error", "", ComparisonCC(send_clock, ComparisonOp::EQ, 30),
      {send_clock}, "", true));
  comm_transitions.push_back(
      Transition("error", "idle", "",
                 ComparisonCC(send_clock, ComparisonOp::EQ, 30), {}, "", true));
  comm_transitions.push_back(
      Transition("prepared", "idle", "", TrueCC(), {}, "", true));
  std::shared_ptr<Clock> state_clock =
      std::make_shared<Clock>(constants::STATE_CLOCK);
  addStateClock(comm_transitions, state_clock);
  Automaton comm_ta =
      Automaton(comm_states, comm_transitions, "comm_ta", false);
  comm_ta.clocks.insert({send_clock,
                         std::make_shared<Clock>(constants::GLOBAL_CLOCK),
                         state_clock});
  return comm_ta;
}

unordered_map<string, vector<unique_ptr<EncICInfo>>>
benchmarkgenerator::generatePerceptionConstraints(
    const Automaton &perception_ta,
    const ::std::unordered_set<::std::string> &pa_names) {
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
  puck_sense_filter.insert(puck_sense_filter.end(),
                           {perception_ta.states[6], perception_ta.states[5]});
  Bounds full_bounds(0, numeric_limits<int>::max());
  Bounds vision_bounds(5, 10); // numeric_limits<int>::max());
  Bounds puck_sense_bounds(2, 5, ComparisonOp::LTE, ComparisonOp::LTE);
  Bounds end_bounds(0, 5);
  if (pa_names.size() == 100000) {
    end_bounds.lower_bound = 100;
  }
  // ---------------------- Normal Constraints ------------------------------
  unordered_map<string, vector<unique_ptr<EncICInfo>>> activations;
  // cam off by default
  for (string pa : pa_names) {
    if (cam_off_exceptions.find(pa) == cam_off_exceptions.end()) {
      activations[pa].emplace_back(make_unique<UnaryInfo>(
          "coff", ICType::Invariant, TargetSpecs(end_bounds, cam_off_filter)));
    }
  }
  // puck sense after pick and put
  activations["endpick"].emplace_back(make_unique<UnaryInfo>(
      "sense1", ICType::Future,
      TargetSpecs(puck_sense_bounds, puck_sense_filter)));
  activations["endput"].emplace_back(make_unique<UnaryInfo>(
      "sense2", ICType::Future,
      TargetSpecs(puck_sense_bounds, puck_sense_filter)));
  // until chain to do icp
  activations["pick"].emplace_back(make_unique<ChainInfo>(
      "icp_chain", ICType::UntilChain,
      // start with a booted cam
      vector<TargetSpecs>{TargetSpecs(vision_bounds, cam_on_filter),
                          // be done with icp after vision_bounds
                          TargetSpecs(full_bounds, vision_filter),
                          // do not do ICP again until pick action is done
                          TargetSpecs(full_bounds, no_vision_filter)},
      "endpick"));
  // // until chain to do icp
  activations["pick"].emplace_back(make_unique<ChainInfo>(
      "pic_chain", ICType::UntilChain,
      // start however
      vector<TargetSpecs>{TargetSpecs(full_bounds, perception_ta.states),
                          // after unspecified time, save a pic
                          TargetSpecs(full_bounds, pic_filter),
                          // do whatever until end
                          TargetSpecs(full_bounds, perception_ta.states)},
      "endpick"));
  // until chain to do icp
  activations["put"].emplace_back(make_unique<ChainInfo>(
      "icp_chain2", ICType::UntilChain,
      // start with a booted cam
      vector<TargetSpecs>{TargetSpecs(vision_bounds, cam_on_filter),
                          // be done with icp after vision_bounds
                          TargetSpecs(full_bounds, vision_filter),
                          // do not do ICP again until pick action is done
                          TargetSpecs(full_bounds, no_vision_filter)},
      "endput"));
  // // until chain to do icp
  activations["put"].emplace_back(make_unique<ChainInfo>(
      "pic_chain2", ICType::UntilChain,
      // start however
      vector<TargetSpecs>{TargetSpecs(full_bounds, perception_ta.states),
                          // after unspecified time, save a pic
                          TargetSpecs(full_bounds, pic_filter),
                          // do whatever until end
                          TargetSpecs(full_bounds, perception_ta.states)},
      "endput"));
  return activations;
}

unordered_map<string, vector<unique_ptr<EncICInfo>>>
benchmarkgenerator::generateCommConstraints(const Automaton &comm_ta) {
  vector<State> comm_idle_filter;
  vector<State> comm_preparing_filter;
  vector<State> comm_prepared_filter;
  comm_idle_filter.push_back(comm_ta.states[0]);
  comm_prepared_filter.push_back(comm_ta.states[2]);
  comm_preparing_filter.push_back(comm_ta.states[1]);

  Bounds full_bounds(0, numeric_limits<int>::max());
  Bounds no_bounds(0, numeric_limits<int>::max());
  Bounds vision_bounds(5, 10); // numeric_limits<int>::max());
  Bounds cam_off_bounds(2, 5);
  Bounds puck_sense_bounds(2, 5, ComparisonOp::LTE, ComparisonOp::LTE);
  Bounds goto_bounds(15, 45);
  Bounds pick_bounds(13, 18);
  Bounds discard_bounds(3, 6);
  Bounds end_bounds(0, 5);

  // ---------------------- Until Chain -------------------------------------
  unordered_map<string, vector<unique_ptr<EncICInfo>>> comm_activations;
  // until chain to prepare
  comm_activations["endput"].emplace_back(make_unique<ChainInfo>(
      "prepare_chain", ICType::UntilChain,
      vector<TargetSpecs>{TargetSpecs(no_bounds, comm_idle_filter),
                          TargetSpecs(no_bounds, comm_preparing_filter),
                          TargetSpecs(no_bounds, comm_prepared_filter),
                          TargetSpecs(no_bounds, comm_idle_filter)},
      "pick"));
  comm_activations["endpick"].emplace_back(make_unique<ChainInfo>(
      "idle_chain", ICType::UntilChain,
      vector<TargetSpecs>{TargetSpecs(no_bounds, comm_idle_filter)}, "put"));
  comm_activations[constants::START_PA].emplace_back(make_unique<ChainInfo>(
      "start_chain", ICType::UntilChain,
      vector<TargetSpecs>{TargetSpecs(no_bounds, comm_idle_filter)}, "pick"));
  comm_activations["last"].emplace_back(make_unique<ChainInfo>(
      "end_chain", ICType::UntilChain,
      vector<TargetSpecs>{TargetSpecs(no_bounds, comm_idle_filter)},
      constants::END_PA));

  return comm_activations;
}

unordered_map<string, vector<unique_ptr<EncICInfo>>>
benchmarkgenerator::generateCalibrationConstraints(const Automaton &calib_ta) {
  vector<State> calib_filter;
  calib_filter.push_back(calib_ta.states[2]);

  Bounds no_bounds(0, numeric_limits<int>::max());

  // ---------------------- Until Chain -------------------------------------
  unordered_map<string, vector<unique_ptr<EncICInfo>>> calib_activations;
  // until chain to prepare
  calib_activations["pick"].emplace_back(make_unique<ChainInfo>(
      "calib_pick_c", ICType::UntilChain,
      vector<TargetSpecs>{TargetSpecs(no_bounds, calib_filter)}, "endpick"));
  calib_activations["put"].emplace_back(make_unique<ChainInfo>(
      "calib_put_c", ICType::UntilChain,
      vector<TargetSpecs>{TargetSpecs(no_bounds, calib_filter)}, "endput"));

  return calib_activations;
}

unordered_map<string, vector<unique_ptr<EncICInfo>>>
benchmarkgenerator::generatePositionConstraints(const Automaton &pos_ta) {
  unordered_map<string, vector<unique_ptr<EncICInfo>>> pos_activations;
  vector<State> moving_filter;
  vector<State> standing_filter;
  auto standing_state = getStateItById(pos_ta.states, "standing");
  auto moving_state = getStateItById(pos_ta.states, "moving");
  if (standing_state == pos_ta.states.end() ||
      moving_state == pos_ta.states.end()) {
    std::cout << "benchmarkgenerator generatePositionConstraints: cannot find "
                 "constraint relevant states"
              << std::endl;
    return pos_activations;
  } else {
    moving_filter.push_back(*moving_state);
    standing_filter.push_back(*standing_state);

    Bounds no_bounds(0, numeric_limits<int>::max());

    // ---------------------- Until Chain -------------------------------------
    // until chain to prepare
    pos_activations["pick"].emplace_back(
        make_unique<UnaryInfo>("pos_pick", ICType::Invariant,
                               TargetSpecs(no_bounds, standing_filter)));
    pos_activations["put"].emplace_back(make_unique<UnaryInfo>(
        "pos_put", ICType::Invariant, TargetSpecs(no_bounds, standing_filter)));
    pos_activations["discard"].emplace_back(
        make_unique<UnaryInfo>("pos_discard", ICType::Invariant,
                               TargetSpecs(no_bounds, standing_filter)));
    pos_activations["endpick"].emplace_back(
        make_unique<UnaryInfo>("pos_pick", ICType::Invariant,
                               TargetSpecs(no_bounds, standing_filter)));
    pos_activations["endput"].emplace_back(make_unique<UnaryInfo>(
        "pos_put", ICType::Invariant, TargetSpecs(no_bounds, standing_filter)));
    pos_activations["last"].emplace_back(make_unique<UnaryInfo>(
        "pos_put", ICType::Invariant, TargetSpecs(no_bounds, standing_filter)));
    pos_activations["enddiscard"].emplace_back(
        make_unique<UnaryInfo>("pos_discard", ICType::Invariant,
                               TargetSpecs(no_bounds, standing_filter)));
    pos_activations["endgoto"].emplace_back(
        make_unique<UnaryInfo>("pos_discard", ICType::Invariant,
                               TargetSpecs(no_bounds, standing_filter)));
    pos_activations["goto"].emplace_back(make_unique<UnaryInfo>(
        "pos_goto", ICType::Invariant, TargetSpecs(no_bounds, moving_filter)));

    return pos_activations;
  }
}
