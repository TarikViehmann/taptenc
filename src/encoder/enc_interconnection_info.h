#pragma once

#include "timed_automata.h"
#include <string>
#include <vector>

namespace taptenc {
enum ICType { Future, NoOp, Past, Invariant, Until, UntilChain, Since };
struct targetSpecs {
  Bounds bounds;
  ::std::vector<State> targets;
  targetSpecs(const Bounds arg_bounds, const ::std::vector<State> &arg_targets)
      : bounds(arg_bounds), targets(arg_targets) {}
};
typedef targetSpecs TargetSpecs;
struct encICInfo {
  ::std::string name;
  ICType type;
  encICInfo(::std::string arg_name, ICType arg_type)
      : name(arg_name), type(arg_type){};
  bool isFutureInfo() const;
  bool isPastInfo() const;
  virtual ~encICInfo() = default;
};

typedef struct encICInfo EncICInfo;

struct unaryInfo : EncICInfo {
  TargetSpecs specs;
  unaryInfo(::std::string arg_name, ICType arg_type,
            const TargetSpecs &arg_specs)
      : EncICInfo(arg_name, arg_type), specs(arg_specs) {}
};
typedef struct unaryInfo UnaryInfo;
struct binaryInfo : EncICInfo {
  TargetSpecs specs;
  ::std::vector<State> pre_targets;
  binaryInfo(::std::string arg_name, ICType arg_type,
             const TargetSpecs &arg_specs,
             const ::std::vector<State> arg_pre_targets)
      : EncICInfo(arg_name, arg_type), specs(arg_specs),
        pre_targets(arg_pre_targets) {}
  UnaryInfo toUnary() const;
};
typedef struct binaryInfo BinaryInfo;
struct chainInfo : EncICInfo {
  ::std::vector<TargetSpecs> specs_list;
  ::std::string end_pa;
  chainInfo(::std::string arg_name, ICType arg_type,
            const ::std::vector<TargetSpecs> &arg_specs_list,
            ::std::string arg_end_pa)
      : EncICInfo(arg_name, arg_type), specs_list(arg_specs_list),
        end_pa(arg_end_pa) {}
};
typedef struct chainInfo ChainInfo;
} // end namespace taptenc
