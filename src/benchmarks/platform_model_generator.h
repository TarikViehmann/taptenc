/** \file
 * Hand-crafted benchmark generator utils.
 *
 * \author (2019) Tarik Viehmann
 */
#include "encoder/enc_interconnection_info.h"
#include "timed-automata/timed_automata.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace taptenc {
/**
 * Utility functions to create hand-crafted benchmarks from the household
 * domain.
 */
namespace householdmodels {
/**
 * Generates a platform TA to model the need to align and move the robot head
 * to perform grasping tasks.
 *
 * @return household platform TA
 */
Automaton generateHouseholdTA();
/**
 * Generates constraints for the household TA.
 *
 * @param hh_ta automaton that models the household robot
 * @return constraint activation mapping for the household TA
 */
::std::vector<::std::unique_ptr<EncICInfo>>
generateHouseholdConstraints(const Automaton &hh_ta);

} // end namespace householdmodels

/**
 * Utility functions to create hand-crafted benchmarks from the RCLL domain.
 */
namespace rcllmodels {

/**
 * Generates a calibration platform TA to model the need to recalibrate
 * gripper axis over time.
 *
 * @return calibration platform TA
 */
Automaton generateCalibrationTA();

/**
 * Generates a perception platform TA to model various perception related
 * features.
 *
 * Features are:
 *  - ICP procedure to obtain data for gripping
 *  - picture snapshot saving for Neural Network training
 *  - puck sensing when the camera is offline
 *
 *  @return perception platform TA
 */
Automaton generatePerceptionTA();

/**
 * Generates a communication platform TA to model simple machine communication.
 *
 * @param machine name of the machine that should be communicated with
 * @return communication platform TA
 */
Automaton generateCommTA(::std::string machine);

/**
 * Generates constraints for the perception TA (see generatePerceptionTA()).
 *
 * @param perception_taautomaton that models the camera, icp and puck check
 * @return constraint activation mapping for the perception TA
 */
::std::vector<::std::unique_ptr<EncICInfo>>
generatePerceptionConstraints(const Automaton &perception_ta);

/**
 * Generates constraints for the communication TA (see generateCommTA()).
 *
 * @param comm_ta automaton that models the communication with machines
 * @param machine name of machine that is handled by the component
 *
 * @return constraint activation mapping for the communication TA
 */
::std::vector<::std::unique_ptr<EncICInfo>>
generateCommConstraints(const Automaton &comm_ta, ::std::string machine);

/**
 * Generates constraints for the calibration TA (see generateCommTA()).
 *
 * @param calib_ta automaton that models the axis calibration
 * @return constraint activation mapping for the calibration TA
 */
::std::vector<::std::unique_ptr<EncICInfo>>
generateCalibrationConstraints(const Automaton &calib_ta);

/**
 * Generates constraints for the communication TA (the TA is not generated but
 * parsed from an xml file).
 * The idea of the communication TA is to send a position update every few
 * seconds while the robot is moving.
 *
 * @return constraint activation mapping for the position TA
 */
::std::vector<::std::unique_ptr<EncICInfo>>
generatePositionConstraints(const Automaton &pos_ta);
} // end namespace rcllmodels
} // end namespace taptenc
