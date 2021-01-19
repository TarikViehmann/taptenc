/** \file
 * Utility functions to parse plans generated from temporal fast downward.
 *
 * \author: (2021) Tarik Viehmann
 */

#pragma once

#include <string>
#include <vector>

namespace taptenc {

namespace tfdplanparser {
std::vector<PlanAction> readSequentialPlan(::std::string file);
}
} // namespace taptenc
