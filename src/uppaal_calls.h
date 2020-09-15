/** \file
 * Interface to call uppaal tools.
 *
 * Since there is no powerful C++ API the tools have to be invoked via system
 * calls.
 *
 * @author (2019) Tarik Viehmann
 */
#pragma once

#include "constants.h"
#include "timed-automata/timed_automata.h"
#include <chrono>
#include <string>
#include <vector>

namespace taptenc {
namespace uppaalcalls {
/** Unit of time measurement. */
typedef ::std::chrono::duration<long, std::milli> timedelta;
/** Default query string. */
constexpr char QUERY_STR[]{"E<> sys_direct.AqueryA"};
constexpr char TAPTENC_TEMP_XML[]{"taptenc_temp"};
/**
 * Deletes empty lines from a file.
 *
 * @param file_name name of the file to delete empty lines from.
 */
void deleteEmptyLines(const std::string &file_path);
/**
 *  Retrieves the content of an environment variable.
 *
 *  Used to get the variable VERIFYTA_DIR, which should hold the path to
 *  uppaals command line solver verifyta.
 *
 *  @param key environment variable name
 *  @return content of \a key
 */
std::string getEnvVar(std::string const &key);

/**
 * Call the verifyta solver and the tracer from the utap lib to solve a query
 * for a given xml system.
 *
 * @param file_name name of xml system file without .xml
 * @param query_str query string suitable for uppaal
 *
 * @return time measures for the two calls to verifyta and if the query is
 *         satisfied then also for the call to tracer
 */
::std::vector<timedelta> solve(::std::string file_name = TAPTENC_TEMP_XML,
                               ::std::string query_str = QUERY_STR);

/**
 * Call the verifyta solver and the tracer from the utap lib to solve a query
 * for a given automata system.
 *
 * @param sys automata system to solve the query for
 * @param file_name name of xml system file without .xml
 * @param query_str query string sutiable for uppaal
 *
 * @return time measures for the two calls to verifyta and if the query is
 *         satisfied then also for the call to tracer
 */
::std::vector<timedelta> solve(const AutomataSystem &sys,
                               ::std::string file_name = TAPTENC_TEMP_XML,
                               ::std::string query_str = QUERY_STR);
} // end namespace uppaalcalls
} // end namespace taptenc
