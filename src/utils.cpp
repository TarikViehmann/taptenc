/** \file
 * Helper functions for standard types.
 *
 * \author (2019) Tarik Viehmann
 */
#include "utils.h"
#include "constants.h"
#include "constraints/constraints.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
using namespace taptenc;

std::pair<int, int> taptenc::iMidPoint(const ::std::pair<int, int> &a,
                                       const ::std::pair<int, int> &b) {
  std::pair<int, int> ret =
      std::make_pair((a.first + b.first) / 2, (a.second + b.second) / 2);
  return ret;
}

float taptenc::fDotProduct(const ::std::pair<float, float> &a,
                           const ::std::pair<float, float> &b) {
  return a.first * b.first + a.second * b.second;
}

std::pair<float, float>
taptenc::iUnitNormalFromPoints(const ::std::pair<int, int> &a,
                               const ::std::pair<int, int> &b) {
  std::pair<float, float> normal;
  std::pair<int, int> vec = a - b;
  normal.first = (float)-(vec.second);
  normal.second = (float)(vec.first);
  float normal_length = std::sqrt(fDotProduct(normal, normal));
  normal.first *= (1 / normal_length);
  normal.second *= (1 / normal_length);
  return normal;
}

::std::unique_ptr<ClockConstraint>
taptenc::addConstraint(const ClockConstraint &old_con,
                       const ClockConstraint &to_add) {
  if (old_con.type == CCType::TRUE) {
    return to_add.createCopy();
  }
  if (to_add.type == CCType::TRUE) {
    return old_con.createCopy();
  }
  return ConjunctionCC(old_con, to_add).createCopy();
}
update_t taptenc::addUpdate(const update_t &old_con, const update_t &to_add) {
  update_t res(old_con);
  res.insert(to_add.begin(), to_add.end());
  return {res};
}

std::string taptenc::trim(const ::std::string &str,
                          const ::std::string &whitespace) {
  const auto strBegin = str.find_first_not_of(whitespace);
  if (strBegin == std::string::npos)
    return ""; // no content

  const auto strEnd = str.find_last_not_of(whitespace);
  const auto strRange = strEnd - strBegin + 1;

  return str.substr(strBegin, strRange);
}

bool taptenc::isPiecewiseContained(::std::string str,
                                   ::std::string container_str,
                                   ::std::string separator) {
  size_t prev = 0;
  size_t found = str.find(separator);
  while (found != std::string::npos) {
    if (container_str.find(trim(str.substr(prev, found - prev))) ==
        std::string::npos) {
      return false;
    }
    prev = found + separator.size();
    found = str.find(separator, prev);
  }
  if (container_str.find(trim(str.substr(prev))) == std::string::npos) {
    return false;
  }
  return true;
}

void taptenc::replaceStringInPlace(::std::string &subject,
                                   const ::std::string &search,
                                   const ::std::string &replace) {
  size_t pos = 0;
  while ((pos = subject.find(search, pos)) != std::string::npos) {
    subject.replace(pos, search.length(), replace);
    pos += replace.length();
  }
}

::std::string taptenc::toLowerCase(::std::string s) {
  std::string res = s;
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return res;
}

::std::string taptenc::convertCharsToHTML(::std::string str) {
  replaceStringInPlace(str, "&", "&amp;");
  replaceStringInPlace(str, "<", "&lt;");
  replaceStringInPlace(str, ">", "&gt;");
  return str;
}

::std::vector<::std::string> taptenc::splitBySep(::std::string s, char sep) {
  std::vector<std::string> res;
  std::stringstream ss(s);
  while (ss.good()) {
    std::string substr;
    getline(ss, substr, sep);
    res.push_back(substr);
  }
  return res;
}
