/*
 * transformation.h
 * Copyright (C) 2020 Tarik Viehmann <viehmann@kbsg.rwth-aachen.de>
 *
 * Distributed under terms of the MIT license.
 */

#pragma once

#include <vector>
#include <memory>
#include "encoders.h"
#include "timed_automata.h"
#include "enc_interconnection_info.h"
#include "constraints.h"
#include "utap_trace_parser.h"

namespace taptenc {
namespace transformation {
typedef std::vector<std::vector<std::unique_ptr<EncICInfo>>> Constraints;
/**
 * Apply the direct encoding to a given automata system consisting of all platform TAs and the plan TA.
 *
 * @param direct_system the system containing the platform TAs and the plan TAs
 * @param plan the plan used to create the plan TA from the \a direct_system
 * @param constraints Constraints connecting platform models with plan actions
 * @param plan_index index of the plan TA inside \a direct_system
 * @return Encoder holding the direct encoding construction
 */
DirectEncoder createDirectEncoding(
    AutomataSystem &direct_system, const std::vector<PlanAction> &plan,
    const std::vector<std::unique_ptr<EncICInfo>> &constraints, int plan_index = 1);
/**
 * Transform a plan according to a platform models and constraints
 *
 * @param plan Plan to transform
 * @param platform_models platform models realizing platform specific behavior
 * @param platform_constraints Constraints connecting platform models with plan actions
 * @return timed trace reflecting the resulting temporal plan
 */
timed_trace_t transform_plan(const std::vector<PlanAction> &plan, const std::vector<Automaton> &platform_models, const Constraints &platform_constraints);

} // end namespace transformation
} // end namespace taptenc
