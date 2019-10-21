/** \file
 * XML parser utilizing the utap lib parser
 *
 * \author (2019) Tarik Viehmann
 */
#include "../timed-automata/timed_automata.h"
#include <string>
namespace taptenc {
namespace utapxmlparser {
/**
 * Read an XML system.
 * @param filename name of xml file
 * @return AutomataSystem containing the info from the xml system.
 */
AutomataSystem readXMLSystem(::std::string filename);
} // end namespace utapxmlparser
} // end namespace taptenc
