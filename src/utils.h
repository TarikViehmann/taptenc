#pragma once
#include <limits>
#include <string>
#include <utility>

namespace taptenc {

template <typename T, typename U>
::std::pair<T, U> operator+(const ::std::pair<T, U> &l,
                            const ::std::pair<T, U> &r) {
  return ::std::make_pair(l.first + r.first, l.second + r.second);
}
template <typename T, typename U>
::std::pair<T, U> operator-(const ::std::pair<T, U> &l,
                            const ::std::pair<T, U> &r) {
  return ::std::make_pair(l.first - r.first, l.second - r.second);
}

::std::pair<int, int> iMidPoint(const ::std::pair<int, int> &a,
                                const ::std::pair<int, int> &b);
float fDotProduct(const ::std::pair<float, float> &a,
                  const ::std::pair<float, float> &b);
::std::pair<float, float> iUnitNormalFromPoints(const ::std::pair<int, int> &a,
                                                const ::std::pair<int, int> &b);
::std::string addConstraint(::std::string old_con, ::std::string to_add);
::std::string addUpdate(::std::string old_con, ::std::string to_add);
::std::string addAction(::std::string old_action, ::std::string to_add);
::std::string trim(const ::std::string &str, const ::std::string &whitespace = " \t");
bool isPiecewiseContained(::std::string str, ::std::string container_str, ::std::string separator);

template <typename T> T safeAddition(T add1, T add2) {
  return (add1 < std::numeric_limits<T>::max() - add2)
             ? add1 + add2
             : ::std::numeric_limits<T>::max();
}
} // end namespace taptenc

namespace std {

template <> struct hash<::std::pair<::std::string, ::std::string>> {
  ::std::size_t
  operator()(const ::std::pair<::std::string, ::std::string> &k) const {
    using std::hash;
    using std::string;

    // Compute individual hash values for first,
    // second and third and combine them using XOR
    // and bit shifting:

    return ((hash<::std::string>()(k.first) ^
             (hash<::std::string>()(k.second) << 1)) >>
            1);
  }
};

} // end namespace std
