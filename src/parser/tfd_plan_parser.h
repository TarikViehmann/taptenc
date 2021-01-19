/** \file
 * Utility functions to parse plans generated from temporal fast downward.
 *
 * \author: (2021) Tarik Viehmann
 */

#pragma once

#include "constraints.h"
#include <string>
#include <vector>

namespace taptenc {

namespace tfdplanparser {
std::vector<PlanAction> readSequentialPlan(::std::string file);
std::vector<PlanAction> insertWaitActions(const std::vector<PlanAction> &input);
} // namespace tfdplanparser
} // namespace taptenc
