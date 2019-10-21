/** \file
 * Datastructures to represent interconnection constraints.
 *
 * \author (2019) Tarik Viehmann
 */
#pragma once

#include "timed_automata.h"
#include <string>
#include <vector>

namespace taptenc {
/**
 * Different types of supported interconnection constraints.
 *
 * Constraints activate upon reaching a plan action \a pa.
 */
enum ICType {
  /** be in a set of target states within some time bounds */
  Future,
  /** upon reaching \a pa be in a set of target states */
  NoOp,
  /** have been in a set of targets satates within some time bounds */
  Past,
  /** during \a pa always be in a set of target states */
  Invariant,
  /**
   * be in one set of target states first, then within some time bounds reach
   * another set of target states
   */
  Until,
  /** Chained Until constraints (be in one set of target states, reach another,
   * reach yet another, ...., until you reach a certain plan action
   */
  UntilChain,
  /**
   * have been in one set of target states first within some time bounds;
   * eversince that, have been in another set of target states until reaching
   * \a pa
   */
  Since
};

/**
 * Target states together with associated time bounds.
 */
struct targetSpecs {
  Bounds bounds;
  ::std::vector<State> targets;
  targetSpecs(const Bounds arg_bounds, const ::std::vector<State> &arg_targets)
      : bounds(arg_bounds), targets(arg_targets) {}
};
typedef targetSpecs TargetSpecs;

/**
 * Interconnection information (IC) base structure.
 */
struct encICInfo {
  /** Name of the information */
  ::std::string name;
  /** Type of the information */
  ICType type;
  encICInfo(::std::string arg_name, ICType arg_type)
      : name(arg_name), type(arg_type){};
  /**
   * Check if \a type is a type concerned with future manipulation of states.
   */
  bool isFutureInfo() const;
  /**
   * Check if \a type is a type concerned with past manipulation of states.
   */
  bool isPastInfo() const;
  virtual ~encICInfo() = default;
};

typedef struct encICInfo EncICInfo;

/**
 * IC info suitable for constraints that specify at most one set of target
 * states.
 *
 * Suitable for #Invariant, #NoOp, #Future and #Past.
 */
struct unaryInfo : EncICInfo {
  TargetSpecs specs;
  unaryInfo(::std::string arg_name, ICType arg_type,
            const TargetSpecs &arg_specs)
      : EncICInfo(arg_name, arg_type), specs(arg_specs) {}
};
typedef struct unaryInfo UnaryInfo;

/**
 * IC info suitable for constraints that specify two set of target states and
 * one set of bounds.
 *
 * Suitabe for #Future and #Since.
 */
struct binaryInfo : EncICInfo {
  TargetSpecs specs;
  ::std::vector<State> pre_targets;
  binaryInfo(::std::string arg_name, ICType arg_type,
             const TargetSpecs &arg_specs,
             const ::std::vector<State> arg_pre_targets)
      : EncICInfo(arg_name, arg_type), specs(arg_specs),
        pre_targets(arg_pre_targets) {}
  /**
   * Converts the binaryInfo to unaryInfo by discarding \a pre_targets.
   * @return unaryInfo with \a specs from this binaryInfo
   */
  UnaryInfo toUnary() const;
};
typedef struct binaryInfo BinaryInfo;

/**
 * IC info suitable for chained constraints specifying a varying number of
 * target sates and bounds.
 *
 * Suitable for #UntilChain.
 */
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
