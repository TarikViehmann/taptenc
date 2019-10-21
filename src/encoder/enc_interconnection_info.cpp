/** \file
 * Datastructures to represent interconnection constraints.
 *
 * \author (2019) Tarik Viehmann
 */
#include "enc_interconnection_info.h"

using namespace taptenc;
bool EncICInfo::isFutureInfo() const {
  return type == ICType::Future || type == ICType::Until;
}
bool EncICInfo::isPastInfo() const {
  return type == ICType::Past || type == ICType::Since;
}

UnaryInfo BinaryInfo::toUnary() const { return UnaryInfo(name, type, specs); }
