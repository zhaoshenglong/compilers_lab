#ifndef TIGER_REGALLOC_COLOR_H_
#define TIGER_REGALLOC_COLOR_H_

#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/util/graph.h"

namespace COL {

class Result {
 public:
  TEMP::Map* coloring;
  TEMP::TempList* spills;
};

Result Color(G::Graph<TEMP::Temp>* ig, TEMP::Map* initial, TEMP::TempList* regs,
             LIVE::MoveList* moves);

}  // namespace COL

#endif