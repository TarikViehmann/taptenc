#pragma once
#include <string>

namespace taptenc {

struct bounds {
  ::std::string l_op;
  ::std::string r_op;
  int x;
  int y;
};
typedef struct bounds Bounds;

::std::string reverse_op(::std::string op);
::std::string inverse_op(::std::string op);
} // namespace taptenc
