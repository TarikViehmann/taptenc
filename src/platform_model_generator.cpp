#include "platform_model_generator.h"
#include "constants.h"
#include "enc_interconnection_info.h"
#include "timed_automata.h"
#include <memory>
#include <string>
#include <time.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace taptenc;
using namespace std;
void addStateClock(vector<Transition> &trans) {
  for (auto &t : trans) {
    t.update = addUpdate(t.update, string(constants::STATE_CLOCK) + " = 0");
  }
}

Automaton benchmarkgenerator::generateCalibrationTA() {
  vector<State> states;
  vector<Transition> transitions;
  states.push_back(State("uncalibrated", "", false, true));
  states.push_back(State("calibrating", "cal &lt; 10"));
  states.push_back(State("calibrated", "cal &lt;= 30"));
  transitions.push_back(Transition("uncalibrated", "calibrating", "calibrate",
                                   "", "cal = 0", "", true));
  transitions.push_back(Transition("calibrating", "calibrated", "",
                                   "cal &gt; 7", "cal = 0", "", true));
  transitions.push_back(Transition("calibrated", "uncalibrated", "",
                                   "cal == 30", "cal = 0", "", true));
  transitions.push_back(Transition("calibrated", "uncalibrated", "uncalibrate",
                                   "cal &lt; 30", "cal = 0", "", true));
  addStateClock(transitions);
  Automaton test(states, transitions, "main", false);

  test.clocks.insert(test.clocks.end(),
                     {"cal", "globtime", constants::STATE_CLOCK});
  return test;
}

Automaton benchmarkgenerator::generatePerceptionTA() {
  vector<State> states;
  vector<Transition> transitions;
  states.push_back(State("idle", "", false, true));
  states.push_back(State("cam_boot", "cam &lt; 5"));
  states.push_back(State("cam_on", ""));
  states.push_back(State("save_pic", "pic &lt;= 2"));
  states.push_back(State("icp_start", "icp &lt;= 10"));
  states.push_back(State("icp_end", "icp &lt;= 3"));
  states.push_back(State("puck_sense", "sense &lt; 5"));
  transitions.push_back(
      Transition("idle", "cam_boot", "power_on_cam", "", "cam = 0", "", true));
  transitions.push_back(
      Transition("cam_boot", "cam_on", "", "cam &gt; 2", "cam = 0", "", true));
  transitions.push_back(
      Transition("cam_on", "save_pic", "store_pic", "", "pic = 0", "", true));
  transitions.push_back(
      Transition("save_pic", "cam_on", "", "pic &gt; 1", "", "", true));
  transitions.push_back(
      Transition("cam_on", "icp_start", "start_icp", "", "icp = 0", "", true));
  transitions.push_back(Transition("icp_start", "icp_end", "",
                                   "icp &lt; 10 &amp;&amp; icp &gt; 5",
                                   "icp = 0", "", true));
  transitions.push_back(
      Transition("icp_end", "cam_on", "icp &gt; 1", "", "", "", true));
  transitions.push_back(
      Transition("cam_on", "idle", "power_off_cam", "", "cam = 0", "", true));
  transitions.push_back(Transition("idle", "puck_sense", "check_puck",
                                   "cam &gt; 2", "sense = 0", "", true));
  transitions.push_back(
      Transition("puck_sense", "idle", "", "sense &gt; 1", "", "", true));
  addStateClock(transitions);
  Automaton test(states, transitions, "main", false);

  test.clocks.insert(test.clocks.end(), {"icp", "cam", "pic", "sense",
                                         "globtime", constants::STATE_CLOCK});
  return test;
}

Automaton benchmarkgenerator::generateCommTA() {
  vector<State> comm_states;
  vector<Transition> comm_transitions;
  comm_states.push_back(State("idle", "", false, true));
  comm_states.push_back(State("prepare", "send &lt; 30"));
  comm_states.push_back(State("prepared", "send &lt;= 0"));
  comm_states.push_back(State("error", "send &lt;= 0"));
  comm_transitions.push_back(
      Transition("idle", "prepare", "send_prepare", "", "send = 0", "", true));
  comm_transitions.push_back(Transition("prepare", "prepared", "",
                                        "send &lt; 30 &amp;&amp; send &gt; 10",
                                        "send = 0", "", true));
  comm_transitions.push_back(
      Transition("prepare", "error", "", "send == 30", "send = 0", "", true));
  comm_transitions.push_back(
      Transition("error", "idle", "", "send == 30", "", "", true));
  comm_transitions.push_back(
      Transition("prepared", "idle", "", "", "", "", true));
  addStateClock(comm_transitions);
  Automaton comm_ta =
      Automaton(comm_states, comm_transitions, "comm_ta", false);
  comm_ta.clocks.insert(comm_ta.clocks.end(),
                        {"send", "globtime", constants::STATE_CLOCK});
  return comm_ta;
}

unordered_map<string, vector<unique_ptr<EncICInfo>>>
benchmarkgenerator::generatePerceptionConstraints(
    const Automaton &perception_ta, const unordered_set<string> &pa_names) {
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
  Bounds puck_sense_bounds(2, 5, "&lt;=", "&lt;=");
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
  Bounds puck_sense_bounds(2, 5, "&lt;=", "&lt;=");
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
