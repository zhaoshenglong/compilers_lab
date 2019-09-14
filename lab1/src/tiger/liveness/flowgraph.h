#ifndef TIGER_LIVENESS_FLOWGRAPH_H_
#define TIGER_LIVENESS_FLOWGRAPH_H_

#include "tiger/codegen/assem.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/util/graph.h"

namespace FG {

TEMP::TempList* Def(G::Node<AS::Instr>* n);
TEMP::TempList* Use(G::Node<AS::Instr>* n);

bool IsMove(G::Node<AS::Instr>* n);

G::Graph<AS::Instr>* AssemFlowGraph(AS::InstrList* il, F::Frame* f);

}  // namespace FG

#endif