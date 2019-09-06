#pragma once
#include <limits>
#include <string>

namespace taptenc {

struct bounds {
  ::std::string l_op;
  ::std::string r_op;
  int lower_bound;
  int upper_bound;
  bounds(int l, int u) {
    l_op = "&lt;=";
    r_op = u == ::std::numeric_limits<int>::max() ? "&lt;" : "&lt;=";
    lower_bound = l;
    upper_bound = u;
  }
  bounds(int l, int u, ::std::string arg_l_op, ::std::string arg_r_op) {
    l_op = arg_l_op;
    r_op = arg_r_op;
    lower_bound = l;
    upper_bound = u;
  }
};
typedef struct bounds Bounds;

struct planAction {
  ::std::string name;
  Bounds duration;
  planAction(::std::string arg_name, const Bounds &arg_duration)
      : name(arg_name), duration(arg_duration) {}
};
typedef struct planAction PlanAction;

::std::string reverse_op(::std::string op);
::std::string inverse_op(::std::string op);
} // namespace taptenc
