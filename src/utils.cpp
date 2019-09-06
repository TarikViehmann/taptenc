#include "utils.h"
#include <cmath>
#include <string>
#include <utility>
using namespace taptenc;

std::pair<int, int> taptenc::iMidPoint(const std::pair<int, int> &a,
                                       const std::pair<int, int> &b) {
  std::pair<int, int> ret =
      std::make_pair((a.first + b.first) / 2, (a.second + b.second) / 2);
  return ret;
}

float taptenc::fDotProduct(const std::pair<float, float> &a,
                           const std::pair<float, float> &b) {
  return a.first * b.first + a.second * b.second;
}

std::pair<float, float>
taptenc::iUnitNormalFromPoints(const std::pair<int, int> &a,
                               const std::pair<int, int> &b) {
  std::pair<float, float> normal;
  std::pair<int, int> vec = a - b;
  normal.first = (float)-(vec.second);
  normal.second = (float)(vec.first);
  float normal_length = std::sqrt(fDotProduct(normal, normal));
  normal.first *= (1 / normal_length);
  normal.second *= (1 / normal_length);
  return normal;
}

std::string taptenc::addConstraint(std::string old_con, std::string to_add) {
  if (old_con.length() == 0) {
    return to_add;
  }
  if (to_add.length() == 0) {
    return old_con;
  }
  return old_con + "&amp;&amp; " + to_add;
}

std::string taptenc::addUpdate(std::string old_con, std::string to_add) {
  if (old_con.length() == 0) {
    return to_add;
  }
  if (to_add.length() == 0) {
    return old_con;
  }
  return old_con + ", " + to_add;
}
