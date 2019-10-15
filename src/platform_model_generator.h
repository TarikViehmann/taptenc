#include "enc_interconnection_info.h"
#include "timed_automata.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace taptenc {
namespace benchmarkgenerator {

Automaton generateCalibrationTA();
Automaton generatePerceptionTA();
Automaton generateCommTA();

::std::unordered_map<::std::string, ::std::vector<::std::unique_ptr<EncICInfo>>>
generatePerceptionConstraints(
    const Automaton &perception_ta,
    const ::std::unordered_set<::std::string> &pa_names);

::std::unordered_map<::std::string, ::std::vector<::std::unique_ptr<EncICInfo>>>
generateCommConstraints(const Automaton &comm_ta);

::std::unordered_map<::std::string, ::std::vector<::std::unique_ptr<EncICInfo>>>
generateCalibrationConstraints(const Automaton &calib_ta);

::std::unordered_map<::std::string, ::std::vector<::std::unique_ptr<EncICInfo>>>
generatePositionConstraints(const Automaton &pos_ta);
} // end namespace benchmarkgenerator
} // end namespace taptenc
