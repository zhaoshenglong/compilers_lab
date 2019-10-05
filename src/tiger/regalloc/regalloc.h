#ifndef TIGER_REGALLOC_REGALLOC_H_
#define TIGER_REGALLOC_REGALLOC_H_

#include "tiger/codegen/assem.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"

namespace RA {

class Result {
 public:
  TEMP::Map* coloring;
  AS::InstrList* il;
};

Result RegAlloc(F::Frame* f, AS::InstrList* il);

}  // namespace RA

#endif