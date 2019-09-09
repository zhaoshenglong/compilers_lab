#include "tiger/liveness/flowgraph.h"

namespace FG {

TEMP::TempList* Def(G::Node<AS::Instr>* n) {
  // your code here.
  return nullptr;
}

TEMP::TempList* Use(G::Node<AS::Instr>* n) {
  // your code here.
  return nullptr;
}

bool IsMove(G::Node<AS::Instr>* n) {
  // your code here.
  return true;
}

G::Graph<AS::Instr>* AssemFlowGraph(AS::InstrList* il, F::Frame* f) {
  // your code here.
  return nullptr;
}

}  // namespace FG
