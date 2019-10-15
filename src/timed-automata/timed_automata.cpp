#include "timed_automata.h"
using namespace taptenc;

bool transition::operator<(const transition &r) const {
  return source_id + dest_id + guard + update + sync <
         r.source_id + r.dest_id + r.guard + r.update + r.sync;
}
bool state::operator<(const state &r) const { return id + inv < r.id + r.inv; }
