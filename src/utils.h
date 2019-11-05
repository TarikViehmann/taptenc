/** \file
 * Helper functions for standard types.
 *
 * \author (2019) Tarik Viehmann
 */

#pragma once

#include "constraints/constraints.h"
#include "timed-automata/timed_automata.h"
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace taptenc {
/**
 * Piecewise addition for pairs.
 */
template <typename T, typename U>
::std::pair<T, U> operator+(const ::std::pair<T, U> &l,
                            const ::std::pair<T, U> &r) {
  return ::std::make_pair(l.first + r.first, l.second + r.second);
}
/**
 * Piecewise substraction for pairs.
 */
template <typename T, typename U>
::std::pair<T, U> operator-(const ::std::pair<T, U> &l,
                            const ::std::pair<T, U> &r) {
  return ::std::make_pair(l.first - r.first, l.second - r.second);
}

/**
 * Calculates the middle points between two points.
 *
 * @param a first point
 * @param b second point
 * @return mid point between \a a and \a b
 */
::std::pair<int, int> iMidPoint(const ::std::pair<int, int> &a,
                                const ::std::pair<int, int> &b);
/**
 * Calculates the dot product between two points.
 *
 * @param a first point
 * @param b second point
 * @return dot product between \a a and \a b
 */
float fDotProduct(const ::std::pair<float, float> &a,
                  const ::std::pair<float, float> &b);

/**
 * Calculates the unit normal from two points.
 *
 * @param a first point
 * @param b second point
 * @return unit normal of a from \a a to \a b
 */
::std::pair<float, float> iUnitNormalFromPoints(const ::std::pair<int, int> &a,
                                                const ::std::pair<int, int> &b);

/**
 * Concatenates two clock constraints.
 *
 * @param old_con constraint1
 * @param to_add constraint2
 * @return conjunction of constraint1 and constraint2
 */
::std::unique_ptr<ClockConstraint> addConstraint(const ClockConstraint &old_con,
                                                 const ClockConstraint &to_add);

/**
 * Concatenates two clock updates.
 *
 * @param old_con update1
 * @param to_add update2
 * @return concatenation of update1 and update2
 */
update_t addUpdate(const update_t &old_con, const update_t &to_add);

/**
 * Removes leading and trailing whitespaces and tabs.
 *
 * @param str string to trim
 * @param whitespace whitespace characters
 * @return \a str without leading and training \a whitespace chars
 */
::std::string trim(const ::std::string &str,
                   const ::std::string &whitespace = " \t");

/**
 * Checks if all parts of one string are contained in another.
 *
 * Trims the separated parts before checking using trim().
 *
 * @param str string that should be contained in \a container_str
 * @param container_str string that should piecewise contain \a str
 * @param separator substring that separates parts of \a str
 * @return true iff all parts of \a str separated by \a separator are contained
 *         in \a container_str
 *
 */
bool isPiecewiseContained(::std::string str, ::std::string container_str,
                          ::std::string separator);

/**
 * Converts a string to lower case.
 *
 * @param s string to convert
 * @return lower case version of \a s
 */
::std::string toLowerCase(::std::string s);

/**
 * Replaces all occurances of a string by another string.
 *
 * @param subject string to replace substrings of
 * @param search substring to replace
 * @param replace replacement for occurances of \a search in \a subject
 */
void replaceStringInPlace(::std::string &subject, const ::std::string &search,
                          const ::std::string &replace);

/**
 * Converts occurances of &, < and > to their html encodings.
 *
 * @param str string to convert
 * @return \a str with &,< and > replaced by their html encodings
 */
::std::string convertCharsToHTML(::std::string str);

/**
 * Split a string into separate ones according to a separator char.
 *
 * @param s string to split
 * @param sep separator to split along
 * @return strings between the separators
 */
::std::vector<::std::string> splitBySep(::std::string s, char sep);

/**
 * Addition robust to overflows.
 *
 * @tparam T type that has std::numeric_limits::max()
 * @param add1 first summand
 * @param add2 second summand
 * @return add1+add2 or std::numeric_limits::max() if this would overflow
 *
 */
template <typename T> T safeAddition(T add1, T add2) {
  return (add1 < std::numeric_limits<T>::max() - add2)
             ? add1 + add2
             : ::std::numeric_limits<T>::max();
}
} // end namespace taptenc

namespace std {

/**
 * Hash function helper struct for pairs of strings.
 */
template <> struct hash<::std::pair<::std::string, ::std::string>> {
  /**
   * Hash funtion for pairs of strings.
   */
  ::std::size_t
  operator()(const ::std::pair<::std::string, ::std::string> &k) const {
    using std::hash;
    using std::string;

    // Compute individual hash values for first and second and combine them
    // using XORcand bit shifting:

    return ((hash<::std::string>()(k.first) ^
             (hash<::std::string>()(k.second) << 1)) >>
            1);
  }
};

} // end namespace std
