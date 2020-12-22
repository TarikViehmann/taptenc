#pragma once
namespace taptenc {
namespace constants {
constexpr char PA_SEP{'W'};
constexpr char TL_SEP{'X'};
constexpr char CONSTRAINT_SEP{'Y'};
constexpr char BASE_SEP{'Z'};
constexpr char COMPONENT_SEP{'C'};
constexpr char ACTION_SEP{'B'};
constexpr char SYNC_SEP{'S'};
constexpr char ACTIVATION_SEP{'F'};
// for finding constraint activations
constexpr char OPEN_BRACKET{'I'};
constexpr char VAR_SEP{'J'};
constexpr char CLOSED_BRACKET{'K'};
// encode variables for ungrounded actions
constexpr char VAR_PREFIX{'L'};
constexpr char START_PA[]{"AstartA"};
constexpr char END_PA[]{"AendA"};
constexpr char QUERY[]{"AqueryA"};
constexpr char PLAN_TA_NAME[]{"AplanA"};
constexpr char INITIAL_STATE[]{"AinitA"};
constexpr char REL_PLAN_CLOCK[]{"ArelclockA"};
constexpr char GLOBAL_CLOCK[]{"AglobalclockA"};
constexpr char CC_CONJUNCTION[]{"&amp;&amp;"};
constexpr char UPDATE_CONJUNCTION[]{","};
} // end namespace constants
} // end namespace taptenc
