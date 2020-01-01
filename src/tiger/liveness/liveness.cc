#include "tiger/liveness/liveness.h"
#include <list>
#include <algorithm>
#include <map>

using setTy = std::list<TEMP::Temp*>;
using setTy = std::list<TEMP::Temp*>;
using nodeTy = G::Node<AS::Instr>*;

namespace 
{
class LivenessEntry {
 public:
  setTy *inset;
  setTy *outset;
  LivenessEntry(setTy *inset, setTy *outset) : inset(inset), outset(outset) {}
  ~LivenessEntry() {delete inset, delete outset;};
};

setTy *set_difference(setTy *leftset, TEMP::TempList *tl);
setTy *set_union(setTy *lset, setTy *rset);
setTy *set_union(TEMP::TempList *tl, setTy *rightset);

} // namespace 

namespace LIVE {


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
    calculation[nl->head] = 
      new LivenessEntry(new setTy(), new setTy());
    nl = nl->tail;
  }
  
  bool finished = false;
  while (!finished) {
    for (nl = nodelist; nl; nl = nl->tail) {
      int old_insize = calculation[nl->head]->inset->size(),
          old_outsize = calculation[nl->head]->outset->size();
      TEMP::TempList *use = FG::Use(nl->head),
                     *def = FG::Def(nl->head);
      
      G::NodeList<AS::Instr>*succs = nl->head->Succ();

      // Whether it is valid?
      for (; succs; succs = succs->tail) {
        calculation[nl->head]->outset = set_union(
                                          calculation[nl->head]->outset, 
                                          calculation[succs->head]->inset);
      }
      calculation[nl->head]->inset = set_union(use, 
            set_difference(calculation[nl->head]->outset, def));
      if (old_insize != calculation[nl->head]->inset->size() 
          ||old_outsize != calculation[nl->head]->outset->size()) {
        finished = false;
      } else {
        finished = true;
      }
    }
  }

  // Produce Interference Graph
  for (nl = nodelist; nl; nl = nl->tail) {
    TEMP::TempList *def = FG::Def(nl->head);

    if (nl->head->NodeInfo()->kind == AS::Instr::MOVE) {
      while (def) {
        setTy::iterator it = calculation[nl->head]->outset->begin();
        for(; it != calculation[nl->head]->outset->end(); it++) {
          interferegraph->AddEdge(
                      interferegraph->NewNode(def->head), 
                      interferegraph->NewNode(*it));
          if (!movelist) {
            movelist = new MoveList(interferegraph->NewNode(def->head), interferegraph->NewNode(*it), NULL);
            ml = movelist;
          } else {
            ml->tail = new MoveList(interferegraph->NewNode(def->head), interferegraph->NewNode(*it), NULL);
          }
        }
        def = def->tail;
      }
    } else {
      while (def) {
        setTy::iterator it = calculation[nl->head]->outset->begin();
        for(; it != calculation[nl->head]->outset->end(); it++) {
          interferegraph->AddEdge(
                      interferegraph->NewNode(def->head), 
                      interferegraph->NewNode(*it));
        }
        def = def->tail;
      }
    }
  }

  return liveness;
}

}  // namespace LIVE

namespace 
{
bool _compare(TEMP::Temp *t1, TEMP::Temp *t2) {
  return t1->Int() < t2->Int();
}
setTy *_merge(setTy *leftset, setTy *rightset) {
  // Calculate left set - right set
  setTy::iterator leftit = leftset->begin(),
                  rightit = rightset->begin();
  setTy *res = new setTy();
  int lastTemp = 0;
  while (*leftit && *rightit) {
    if ((*leftit)->Int() < (*rightit)->Int()) {
      // No Duplicate
      if((*leftit)->Int() > lastTemp) {
        lastTemp = (*leftit)->Int();
        res->push_back((*leftit));
      }
      leftit++;
    } else {
      if ((*rightit)->Int() > lastTemp) {
        lastTemp = (*rightit)->Int();
        res->push_back((*rightit));
      }
      rightit++;
    }
  }
  while (*leftit) {
    if ((*leftit)->Int() > lastTemp) {
      lastTemp = (*leftit)->Int();
      res->push_back((*leftit));
    }
    leftit++;
  }
  while (*rightit) {
    if ((*rightit)->Int() > lastTemp) {
      lastTemp = (*rightit)->Int();
      res->push_back((*rightit));
    }
    rightit++;
  }
  return res;
}

setTy *_temp2list(TEMP::TempList *tl) {
  setTy *res = new setTy();
  while (tl) {
    res->push_back(tl->head);
    tl = tl->tail;
  }
  return res;
}

setTy *set_union(setTy *leftset, setTy *rightset) {
  std::sort(leftset->begin(), leftset->end(), _compare);
  std::sort(rightset->begin(), rightset->end(), _compare);
  return _merge(leftset, rightset);
}

setTy *set_union(TEMP::TempList *tl, setTy *rightset) {
  std::sort(rightset->begin(), rightset->end(), _compare);
  setTy *leftset = _temp2list(tl);
  std::sort(leftset->begin(), leftset->end(), _compare);
  return _merge(leftset, rightset);
}

setTy *set_difference(setTy *leftset, TEMP::TempList *tl) {
  while (tl) {
    setTy::iterator it;
    for ( it = leftset->begin(); *it; it++) {
      if ((*it)->Int() == tl->head->Int()) {
        leftset->erase(it);
      }
    }
    tl = tl->tail;
  }
}
} // namespace 
