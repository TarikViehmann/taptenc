#include "transformation.h"
#include "encoders.h"
#include "plan_ordered_tls.h"
#include "printer.h"
#include "uppaal_calls.h"
#include "utap_trace_parser.h"
#include "utap_xml_parser.h"
#include "vis_info.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <stdexcept>

using namespace taptenc;

DirectEncoder transformation::createDirectEncoding(
    AutomataSystem &direct_system, const std::vector<PlanAction> &plan,
    const std::vector<std::unique_ptr<EncICInfo>> &constraints,
    int plan_index) {
  DirectEncoder enc(direct_system, plan);
  for (const auto &gamma : constraints) {
    for (auto pa = direct_system.instances[plan_index].first.states.begin();
         pa != direct_system.instances[plan_index].first.states.end(); ++pa) {
      std::string pa_op = pa->id;
      if (pa->id != constants::START_PA && pa->id != constants::END_PA) {
        pa_op = Filter::getPrefix(pa->id, constants::PA_SEP);
      } else {
        continue;
      }
      auto plan_action_it =
          std::find_if(plan.begin(), plan.end(), [pa_op](const auto &act) {
            return act.name.toString() == pa_op;
          });
      auto pa_trigger = std::find_if(
          gamma->activations.begin(), gamma->activations.end(),
          [pa_op, plan_action_it](const ActionName &s) {
            return s.ground(plan_action_it->name.args).toString() ==
                   plan_action_it->name.toString();
          });
      auto is_active = gamma->activations.end() != pa_trigger;
      if (is_active) {
        switch (gamma->type) {
        case ICType::Future: {
          UnaryInfo *info = dynamic_cast<UnaryInfo *>(gamma.get());
          enc.encodeFuture(direct_system, pa->id, *info);
        } break;
        case ICType::Until: {
          BinaryInfo *info = dynamic_cast<BinaryInfo *>(gamma.get());
          enc.encodeUntil(direct_system, pa->id, *info);
        } break;
        case ICType::Since: {
          BinaryInfo *info = dynamic_cast<BinaryInfo *>(gamma.get());
          enc.encodeSince(direct_system, pa->id, *info);
        } break;
        case ICType::Past: {
          UnaryInfo *info = dynamic_cast<UnaryInfo *>(gamma.get());
          enc.encodePast(direct_system, pa->id, *info);
        } break;
        case ICType::NoOp: {
          UnaryInfo *info = dynamic_cast<UnaryInfo *>(gamma.get());
          enc.encodeNoOp(direct_system, info->specs.targets, pa->id);
        } break;
        case ICType::Invariant: {
          UnaryInfo *info = dynamic_cast<UnaryInfo *>(gamma.get());
          enc.encodeInvariant(direct_system, info->specs.targets, pa->id);
        } break;
        case ICType::UntilChain: {
          ChainInfo *info = dynamic_cast<ChainInfo *>(gamma.get());
          for (auto epa = pa + 1;
               epa != direct_system.instances[plan_index].first.states.end();
               ++epa) {
            std::string epa_op = Filter::getPrefix(epa->id, constants::PA_SEP);
            auto eplan_action_it = std::find_if(
                plan.begin(), plan.end(), [epa_op](const auto &act) {
                  return act.name.toString() == epa_op;
                });
            if (eplan_action_it == plan.end()) {
              break;
            }

            auto epa_trigger = std::find_if(
                info->activations_end.begin(), info->activations_end.end(),
                [epa_op, eplan_action_it](const auto &act) {
                  return act.ground(eplan_action_it->name.args).toString() ==
                         eplan_action_it->name.toString();
                });
            bool match_found = info->activations_end.end() != epa_trigger;
            if (match_found) {
              if (epa_trigger->id == pa_trigger->id &&
                  epa_trigger->args.size() != pa_trigger->args.size()) {
                // std::cout << "wrong argument length" << std::endl;
                // they only really match if they have the same number of args
                // this is not true in the general case, but for benchmarks
                // it is sufficient
                break;
              }
              bool missmatch = false;
              for (unsigned long i = 0; i < epa_trigger->args.size(); i++) {
                if (epa_trigger->args[i][0] == constants::VAR_PREFIX) {
                  for (unsigned long j = 0; j < pa_trigger->args.size(); j++) {
                    if (pa_trigger->args[j] == epa_trigger->args[i] &&
                        eplan_action_it->name.args[i] !=
                            plan_action_it->name.args[j]) {
                      // std::cout << "missmatch found: " << pa_trigger->args[j]
                      // << "
                      // "
                      //           << epa_trigger->args[i] << std::endl;
                      // std::cout << eplan_action_it->name.args[i] << " vs "
                      //           << plan_action_it->name.args[j] << std::endl;
                      // std::cout << pa_trigger->toString() << " vs "
                      //           << epa_trigger->toString() << std::endl;
                      missmatch = true;
                      break;
                    }
                  }
                }
              }
              if (!missmatch) {
                enc.encodeUntilChain(direct_system, *info, pa->id, epa->id);
                break;
              } else {
              }
            }
            bool is_tightest =
                gamma->activations.end() ==
                std::find_if(
                    gamma->activations.begin(), gamma->activations.end(),
                    [eplan_action_it](const ActionName &s) {
                      return s.ground(eplan_action_it->name.args).toString() ==
                             eplan_action_it->name.toString();
                    });
            if (!is_tightest) {
              break;
            }
          }
        } break;
        default:
          throw std::runtime_error("error: no support yet for type ");
        }
      }
    }
  }
  return enc;
}

timed_trace_t
transformation::transform_plan(const std::vector<PlanAction> &plan,
                               const std::vector<Automaton> &platform_models,
                               const Constraints &platform_constraints,
                               const update_t &clock_inits) {
  assert(platform_models.size() == platform_constraints.size());
  XMLPrinter printer;
  DirectEncoder merge_enc;
  AutomataSystem merged_system;
  Automaton product_ta = platform_models[0];
  AutomataSystem base_system;
  Automaton plan_ta = product_ta;
  for (long unsigned int j = 0; j < platform_models.size(); j++) {
    AutomataSystem base_system;
    base_system.instances.push_back(std::make_pair(platform_models[j], ""));
    // encode the j-th platform ta
    DirectEncoder curr_encoder = transformation::createDirectEncoding(
        base_system, plan, platform_constraints[j]);
    if (j > 0) {
      product_ta =
          PlanOrderedTLs::productTA(product_ta, platform_models[j], "product",
                                    encoderutils::SuccTransOpts::FROM_SOURCE);
      // merge the encoding of the j-th platform ta into the full encoding
      merge_enc = merge_enc.mergeEncodings(curr_encoder);
    } else {
      // init the full encoding with the instances of the first encoding
      merged_system.instances = base_system.instances;
      merge_enc = curr_encoder.copy();
      plan_ta = base_system.instances[curr_encoder.getPlanTAIndex()].first;
    }
    // extract all clocks from the transformed system
    SystemVisInfo tmp_vis_info;
    AutomataSystem direct_system =
        curr_encoder.createFinalSystem(base_system, tmp_vis_info);
    merged_system.globals.clocks.insert(direct_system.globals.clocks.begin(),
                                        direct_system.globals.clocks.end());
  }
  SystemVisInfo merged_system_vis_info;
  // finalize the encoding and obtain the visual information for printing
  AutomataSystem final_merged_system = merge_enc.createFinalSystem(
      merged_system, merged_system_vis_info, clock_inits);
  std::cout << "num_states: "
            << final_merged_system.instances[0].first.states.size()
            << std::endl;
  // print encoded ta to xml
  printer.print(final_merged_system, merged_system_vis_info, "merged.xml");
  // solve the encoded reachability problem
  auto uppaal_time = uppaalcalls::solve("merged");
  std::vector<std::string> times = {"intermediate format", "model checking",
                                    "tracer"};
  int i = 0;
  for (const auto &t : uppaal_time) {
    std::cout << times[i] << ": " << t.count() << " (ms)" << std::endl;
    i++;
  }
  UTAPTraceParser trace_parser = UTAPTraceParser(final_merged_system);
  // retrieve the solution trace
  trace_parser.parseTraceInfo("merged.trace");
  return trace_parser.getTimedTrace(product_ta, plan_ta);
}
