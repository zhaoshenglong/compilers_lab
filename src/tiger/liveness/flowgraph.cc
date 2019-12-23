#include "tiger/liveness/flowgraph.h"
#include<map>

namespace FG {
// using InstrNodeT = G::Node<AS::Instr> *;

TEMP::TempList* Def(G::Node<AS::Instr>* n) {
  AS::Instr *instr = n->NodeInfo();
  if (instr->kind == AS::Instr::LABEL) {
    // Label Instr Has No Defs
    return NULL;
  } else if (instr->kind == AS::Instr::MOVE) {
    return static_cast<AS::MoveInstr *>(instr)->src;
  } else {
    return static_cast<AS::OperInstr *>(instr)->src;
  }
}

TEMP::TempList* Use(G::Node<AS::Instr>* n) {
  AS::Instr *instr = n->NodeInfo();
  if(instr->kind == AS::Instr::LABEL) {
    // Label Instr Has NO Use
    return NULL;;
  } else if (instr->kind == AS::Instr::MOVE) {
    return static_cast<AS::MoveInstr *>(instr)->dst;
  } else {
    return static_cast<AS::OperInstr *>(instr)->dst;
  }
}

bool IsMove(G::Node<AS::Instr>* n) {
  return n->NodeInfo()->kind == AS::Instr::MOVE;
}

G::Graph<AS::Instr>* AssemFlowGraph(AS::InstrList* il, F::Frame* f) {
  G::Graph<AS::Instr> *flowgraph = new G::Graph<AS::Instr>();
  assert(il && il->tail);

  // First Pass, Find All The Label Instrs
  std::map<TEMP::Label*, G::Node<AS::Instr> *> labelNodes;
  for (AS::InstrList *i = il; i; i = i->tail) {
    if (i->head->kind == AS::Instr::LABEL) {
      AS::LabelInstr *li = static_cast<AS::LabelInstr *>(i->head);
      labelNodes[li->label] = flowgraph->NewNode(li);
    }
  }
  
  // Second Pass, Add All edges to flowgraph
  AS::InstrList *cur = NULL, *next = NULL;
  G::Node<AS::Instr> *from, *to;
  for (cur = il, next = cur->tail; next; cur = next, next = next->tail){
    if (cur->head->kind == AS::Instr::LABEL){
      AS::LabelInstr *li = static_cast<AS::LabelInstr*>(cur->head);
      from = labelNodes[li->label];
    }
    from = flowgraph->NewNode(cur->head);
    if (next->head->kind == AS::Instr::LABEL) {
      AS::LabelInstr *li = static_cast<AS::LabelInstr*>(next->head);
      to = labelNodes[li->label];
    }
    to = flowgraph->NewNode(next->head);
    flowgraph->AddEdge(from, to);
    if (cur->head->kind == AS::Instr::OPER) {
      AS::OperInstr *opi = static_cast<AS::OperInstr *>(cur->head);
      if (opi->jumps) {
        TEMP::LabelList *labels = opi->jumps->labels;
        for (TEMP::LabelList *ll = labels; ll; ll = ll->tail) {
          assert(labelNodes[ll->head]);
          flowgraph->AddEdge(from, labelNodes[ll->head]);
        }
      }
    }
  }

  // Check Last Instr
  if (cur->head->kind == AS::Instr::OPER) {
    AS::OperInstr *opi = static_cast<AS::OperInstr *>(cur->head);
    if (opi->jumps) {
      TEMP::LabelList *labels = opi->jumps->labels;
      for (TEMP::LabelList *ll = labels; ll; ll = ll->tail) {
        assert(labelNodes[ll->head]);
        flowgraph->AddEdge(from, labelNodes[ll->head]);
      }
    }
  }
  
  return flowgraph;
}

}  // namespace FG
