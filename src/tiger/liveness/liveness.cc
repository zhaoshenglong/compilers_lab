#include "tiger/liveness/liveness.h"
#include <list>
#include <set>
#include <algorithm>
#include <map>

using setTy = std::set<TEMP::Temp*>;
using setTy = std::set<TEMP::Temp*>;
using nodeTy = G::Node<AS::Instr>*;

namespace 
{
class LivenessEntry {
 public:
  setTy inset;
  setTy outset;
  LivenessEntry() {}
};


G::Node<TEMP::Temp> *newNode(G::Graph<TEMP::Temp> *, TEMP::Temp *t, std::map<TEMP::Temp*, G::Node<TEMP::Temp>* >*);
setTy templist2set(TEMP::TempList *tl);
} // namespace 

namespace LIVE {

// debug
void display(G::Node<AS::Instr> *node, std::set<TEMP::Temp *>  & out, TEMP::TempList *use, TEMP::TempList *def){
  TEMP::Map *bmap=F::tempMap();
  AS::Instr *instr=node->NodeInfo();
  if(instr->kind==AS::Instr::LABEL){
    ((AS::LabelInstr *)instr)->Print(stdout, bmap);
  }else if(instr->kind==AS::Instr::MOVE){
    ((AS::MoveInstr *)instr)->Print(stdout, bmap);
  }else{
    ((AS::OperInstr *)instr)->Print(stdout, bmap);
  }
  printf("  use: ");
  for(TEMP::TempList *ul=use; ul && ul->head; ul=ul->tail){
    printf("%s ", bmap->Look(ul->head)->c_str());
  }
  printf("  def: ");
  for(TEMP::TempList *dl=def; dl && dl->head; dl=dl->tail){
    printf("%s ", bmap->Look(dl->head)->c_str());
  }
  printf("  out: ");
  for(std::set<TEMP::Temp *>::iterator iter=out.begin(); iter!=out.end(); iter++){
    printf("%s ", bmap->Look(*iter)->c_str());
  }
  printf("\n");
}


LiveGraph Liveness(G::Graph<AS::Instr>* flowgraph) {
  MoveList *movelist = NULL;
  MoveList *ml = NULL;
  G::Graph<TEMP::Temp> *interferegraph = new G::Graph<TEMP::Temp>();
  LiveGraph liveness;
  liveness.graph = interferegraph;
  liveness.moves = movelist;
  
  // Init The IN/OUT Sets
  std::map<nodeTy, LivenessEntry*> calculation;
  G::NodeList<AS::Instr> *nodelist = flowgraph->Nodes();

  G::NodeList<AS::Instr> *nl = nodelist;
  while (nl) {
    calculation[nl->head] = new LivenessEntry();
    nl = nl->tail;
  }
  
  bool finished = false;
  while (!finished) {
    finished = true;
    for (nl = nodelist; nl; nl = nl->tail) {
      int old_insize = calculation[nl->head]->inset.size(),
          old_outsize = calculation[nl->head]->outset.size();
      TEMP::TempList *use = FG::Use(nl->head),
                     *def = FG::Def(nl->head);
      
      G::NodeList<AS::Instr>*succs = nl->head->Succ();

      // Whether it is valid?
      calculation[nl->head]->outset.clear();
      for (; succs; succs = succs->tail) {
        calculation[nl->head]->outset = U::set_union(
                                          calculation[nl->head]->outset, 
                                          calculation[succs->head]->inset);
      }
      calculation[nl->head]->inset = U::set_union(templist2set(use), 
            U::set_difference(calculation[nl->head]->outset, templist2set(def)));
      if (old_insize != calculation[nl->head]->inset.size() 
          || old_outsize != calculation[nl->head]->outset.size()) {
        finished = false;
      }
    }
  }
  
  printf("Flowgraph nodes count: %d", flowgraph->nodecount);
  for (nl = nodelist; nl; nl = nl->tail) {
    TEMP::TempList *def = FG::Def(nl->head);
    TEMP::TempList *use = FG::Use(nl->head);
    display(nl->head, calculation[nl->head]->outset, use ,def);
  }

  std::map<TEMP::Temp*, G::Node<TEMP::Temp>*> node_map;

  // Add edges to machine registers
  TEMP::TempList *registers = F::registers();
  for (TEMP::TempList *i = registers; i; i = i->tail) {
    for (TEMP::TempList *j = i->tail; j; j = j->tail) {
      G::Node<TEMP::Temp> *m = newNode(interferegraph, i->head, &node_map),
                          *n = newNode(interferegraph, j->head, &node_map);
      interferegraph->AddEdge(m, n); 
    }
  }
  
  // Produce Interference Graph
  for (nl = nodelist; nl; nl = nl->tail) {
    TEMP::TempList *def = FG::Def(nl->head);
    if (nl->head->NodeInfo()->kind == AS::Instr::MOVE) {
      TEMP::TempList *use = FG::Use(nl->head);
      setTy::iterator it = calculation[nl->head]->outset.begin();
      for(; it != calculation[nl->head]->outset.end(); it++) {
        if(*it != use->head){
          G::Node<TEMP::Temp> *m = newNode(interferegraph, def->head, &node_map),
                              *n = newNode(interferegraph, *it, &node_map);
          if(m != n) {
            interferegraph->AddEdge(m, n);
          }
        }
      }
      if (!movelist) {
        movelist = new MoveList(newNode(interferegraph, def->head, &node_map), 
                                newNode(interferegraph, use->head, &node_map), NULL);
        ml = movelist;
      } else {
        ml->tail = new MoveList(newNode(interferegraph, def->head, &node_map), 
                                newNode(interferegraph, use->head, &node_map), NULL);
        ml = ml->tail;
      }

  
    } else {
      while (def) {
        setTy::iterator it = calculation[nl->head]->outset.begin();
        for(; it != calculation[nl->head]->outset.end(); it++) {
          G::Node<TEMP::Temp> *m = newNode(interferegraph, def->head, &node_map),
                              *n = newNode(interferegraph, *it, &node_map);
          if(m != n) {
            interferegraph->AddEdge(m, n);
          }
        }
        def = def->tail;
      }
    }
  }

  G::NodeList<TEMP::Temp> *nodes = interferegraph->Nodes();
  for (; nodes; nodes = nodes->tail) {
    G::NodeList<TEMP::Temp> *succ = nodes->head->Adj();
    printf("=================== From node: %d ==================\n", nodes->head->NodeInfo()->Int());
    for (; succ; succ = succ->tail) {
      printf("%d ----- %d\n", nodes->head->NodeInfo()->Int(), succ->head->NodeInfo()->Int());
    }  
  }
  

  return liveness;
}

}  // namespace LIVE

namespace 
{
setTy templist2set(TEMP::TempList *tl) {
  setTy res;
  while (tl) {
    res.insert(tl->head);
    tl = tl->tail;
  }
  return res;
}
G::Node<TEMP::Temp> *newNode(G::Graph<TEMP::Temp> *g, TEMP::Temp *t, std::map<TEMP::Temp*, G::Node<TEMP::Temp>* >*m) {
  if (!(*m)[t]) {
    (*m)[t] = g->NewNode(t);
  }
  return (*m)[t];
}

} // namespace 
