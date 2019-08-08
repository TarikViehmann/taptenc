#include "constraints.h"
#include <iostream>
using namespace taptenc;

std::string taptenc::reverse_op(std::string op) {
  if (op == "&lt;")
    return "&gt;";
  if (op == "&gt;")
    return "&lt;";
  if (op == "&lt;=")
    return "&gt;=";
  if (op == "&gt;=")
    return "&lt;=";
  std::cout << "reverse_op: unknown op:" << op << std::endl;
  return "reverse_op: error";
}

std::string taptenc::inverse_op(std::string op) {
  if (op == "&lt;")
    return "&gt;=";
  if (op == "&gt;")
    return "&lt;=";
  if (op == "&lt;=")
    return "&gt;";
  if (op == "&gt;=")
    return "&lt;";
  std::cout << "inverse_op: unknown op:" << op << std::endl;
  return "inverse_op: error";
}
