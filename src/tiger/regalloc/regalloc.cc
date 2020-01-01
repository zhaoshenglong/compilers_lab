#include "tiger/regalloc/regalloc.h"
#include "tiger/liveness/liveness.h"
#include <list>
#include <set>
#include <map>

namespace 
{
  class Edge{
   public:
    TEMP::Temp *src;
    TEMP::Temp *dst;
    Edge(TEMP::Temp *src, TEMP::Temp *dst) : src(src), dst(dst) {};
  };
  
  static TEMP::TempList *L(TEMP::Temp *r1, TEMP::TempList *r2) {
    return new TEMP::TempList(r1, r2);
  }
  static void insertAfter(AS::InstrList *origin, AS::Instr *cur, AS::InstrList *il);
  static void insertBefore(AS::InstrList *origin, AS::Instr *cur, AS::InstrList *il);

  static int K;
  static std::set<TEMP::Temp *> precolored;
  static std::set<TEMP::Temp *> initial;
  static std::set<TEMP::Temp *> simplifyWorklist;
  static std::set<TEMP::Temp *> freezeWorklist;
  static std::set<TEMP::Temp *> spillWorklist;
  static std::set<TEMP::Temp *> spilledNodes;
  static std::set<TEMP::Temp *> coalescedNodes;
  static std::set<TEMP::Temp *> coloredNodes;
  static std::list<TEMP::Temp *> selectStack;
  static std::set<TEMP::Temp *> selectStackSet;

  static std::set<AS::Instr *> coalescedMoves;
  static std::set<AS::Instr *> constrainedMoves;
  static std::set<AS::Instr *> frozenMoves;
  static std::set<AS::Instr *> worklistMoves;
  static std::set<AS::Instr *> activeMoves;

  static std::map<TEMP::Temp *, int> degree;
  static std::map<TEMP::Temp *, TEMP::Temp *> alias;
  static std::map<TEMP::Temp *, std::set<AS::Instr *> > moveList;
  static std::set<Edge *> adjSet;
  static std::map<TEMP::Temp *, std::set<TEMP::Temp *> > adjList;
  static std::map<TEMP::Temp *, TEMP::Temp *> color;

  static std::set<TEMP::Temp *> getColors();
  void initWorklist();
  void mainProcedure(RA::Result *res, F::Frame *f, AS::InstrList *il);
  void build(G::Graph<AS::Instr> *flowgraph, AS::InstrList *il);
  void addEdge(TEMP::Temp *src, TEMP::Temp *dst);
  void makeWorklist();
  void simplify();
  void coalesce();
  void freeze();
  void selectSpill();
  void assignColors();


  void rewriteProgram(LIVE::LiveGraph liveness, F::Frame *f, AS::InstrList * il);
  void assignColors();
  void coalescse();


} // namespace 

namespace RA {

Result RegAlloc(F::Frame* f, AS::InstrList* il) {
  Result res;
  initWorklist();
  mainProcedure(&res, f, il);
  return res;
}

}  // namespace RA

namespace 
{
  void initWorklist() {
    precolored.clear();
    initial.clear();
    simplifyWorklist.clear();
    freezeWorklist.clear();
    spillWorklist.clear();
    spilledNodes.clear();
    coalescedNodes.clear();
    coloredNodes.clear();
    selectStack.clear();
    selectStackSet.clear();

    coalescedMoves.clear();
    constrainedMoves.clear();
    frozenMoves.clear();
    worklistMoves.clear();
    activeMoves.clear();

    degree.clear();
    alias.clear();
    moveList.clear();
    adjSet.clear();
    adjList.clear();
    color.clear();
  }

  void mainProcedure(RA::Result *res, F::Frame* f, AS::InstrList* il) {
    G::Graph<AS::Instr> *flowgraph = FG::AssemFlowGraph(il, f);
    LIVE::LiveGraph livegraph = LIVE::Liveness(flowgraph);
    build(livegraph, il);
    makeWorklist();
    while (!simplifyWorklist.empty() && !worklistMoves.empty()
    &&!freezeWorklist.empty() && !spillWorklist.empty())
    {
      if (!simplifyWorklist.empty()) simplify();
      else if (!worklistMoves.empty()) coalesce();
      else if (!freezeWorklist.empty()) freeze();
      else if (!spillWorklist.empty()) selectSpill();
    }
    
    assignColors();
    if (!spilledNodes.empty()){
      rewriteProgram(flowgraph, f, il);
      mainProcedure(res, f, il);
    }
  }

  void build(LIVE::LiveGraph livegraph, AS::InstrList *il) {
    LIVE::MoveList *movelist = livegraph.moves;
    G::NodeList<TEMP::Temp> *nodes = livegraph.graph->Nodes();
    AS::InstrList *ilp = il;
    while (ilp) {
      // FG::Def()
      worklistMoves.insert(ilp->head);
      ilp = ilp->tail;
    }

    while (nodes) {
      G::Node<TEMP::Temp> *node = nodes->head;

      G::NodeList<TEMP::Temp> *succ = nodes->head->Succ();
      for (; succ; succ = succ->tail) {
        addEdge(nodes->head->NodeInfo(), succ->head->NodeInfo());
      }
      initial.insert(node->NodeInfo());
    }
  }

  void addEdge(TEMP::Temp *u, TEMP::Temp *v) {
    Edge *e;
    e->src = u;
    e->dst = v;
    if (u != v && adjSet.find(e) == adjSet.end()) {
      adjSet.insert(new Edge(u, v));
      adjSet.insert(new Edge(v, u));
      if (precolored.find(u) == precolored.end()) {
        adjList[u].insert(v);
        degree[u] = degree[u] + 1;
      }
      if (precolored.find(v) == precolored.end()) {
        adjList[v].insert(u);
        degree[v] = degree[v] + 1;
      }
    }
  }

  void makeWorklist() {
    std::set<TEMP::Temp *>::iterator it = initial.begin();
    for (; it != initial.end(); it++)
    {
      initial.erase(it);
      if (degree[*it] >= K) {
        spillWorklist.insert(*it);
      } else if (moveRelated(*it)) {
        freezeWorklist.insert(*it);
      } else {
        simplifyWorklist.insert(*it);
      }
    }
  }

  std::set<TEMP::Temp *> adjacent(TEMP::Temp *n) {
    return U::set_difference(adjList[n], 
                    U::set_union(selectStackSet, coalescedNodes));
  }

  std::set<AS::Instr *> nodeMoves(TEMP::Temp *n) {
    return U::set_intersect(moveList[n], 
                    U::set_union(activeMoves, worklistMoves));
  }

  bool moveRelated(TEMP::Temp *n) {
    return !nodeMoves(n).empty();
  }

  void simplify() {
    if (!simplifyWorklist.empty() ) {
      std::set<TEMP::Temp *>::iterator n = simplifyWorklist.begin();
      simplifyWorklist.erase(n);
      selectStack.push_back(*n);
      selectStackSet.insert(*n);

      std::set<TEMP::Temp *> adj = adjacent(*n);
      std::set<TEMP::Temp *>::iterator it = adj.begin();
      for (; it != adj.end(); it++) {
        decrementDegree(*it);
      }
    }
  }

  void decrementDegree(TEMP::Temp *m) {
    degree[m] = degree[m] - 1;
    if (degree[m] == K) {
      std::set<TEMP::Temp *>mset;
      mset.insert(m);
      enableMoves(U::set_union(adjacent(m), mset));
      spillWorklist.erase(spillWorklist.find(m));
      if (moveRelated(m)) {
        freezeWorklist.insert(m);
      } else {
        simplifyWorklist.insert(m);
      }
    }
  }

  void enableMoves(std::set<TEMP::Temp *> nodes) {
    std::set<TEMP::Temp *>::iterator it = nodes.begin();
    for (; it != nodes.end(); it++) {
      std::set<AS::Instr *> nodemoves = nodeMoves(*it);
      std::set<AS::Instr *>::iterator nit = nodemoves.begin();
      for (; nit != nodemoves.end(); nit++) {
        if (activeMoves.find(*nit) != activeMoves.end()) {
          activeMoves.erase(*nit);
          worklistMoves.insert(*nit);
        }
      }
    }
  }

  void coalesce() {
    if (!worklistMoves.empty()) {
      std::set<AS::Instr *>::iterator m = worklistMoves.begin();
      AS::MoveInstr *mm = static_cast<AS::MoveInstr *>(*m);
      TEMP::Temp *x = mm->src->head;
      TEMP::Temp *y = mm->dst->head;
      x = getAlias(x);
      y = getAlias(y);

      TEMP::Temp *t;
      if (precolored.find(y) != precolored.end()) {
        // make sure precolored y is in the first place
        t = x;
        x = y;
        y = t; 
      }
      worklistMoves.erase(worklistMoves.find(*m));

      Edge *e;
      e->src = x; e->dst = y;
      bool test = true;
      std::set<TEMP::Temp*> adj = adjacent(y);
      std::set<TEMP::Temp*>::iterator it = adj.begin();
      for (; it != adj.end(); it++) {
        if (!ok(*it, x)) {
          test = false;
          break;
        }
      }
      
      if (x == y) {
        coalescedMoves.insert(*m);
        addWorklist(x);
      } else if(precolored.find(y) != precolored.end() 
                || adjSet.find(e) != adjSet.end()) {
        constrainedMoves.insert(*m);
        addWorklist(x);
        addWorklist(y);
      } else if (precolored.find(x) != precolored.end() && test
                || precolored.find(x) == precolored.end() 
                && conservative(U::set_union(adjacent(x), adjacent(y)))) {
        coalescedMoves.insert(*m);
        combine(x, y);
        addWorklist(x);
      } else {
        activeMoves.insert(*m);
      }
    }
  }

  void addWorklist(TEMP::Temp *u) {
    if(precolored.find(u) == precolored.end() && !moveRelated(u) && degree[u] < K) {
      freezeWorklist.erase(freezeWorklist.find(u));
      simplifyWorklist.insert(u);
    }
  }

  bool ok(TEMP::Temp *t, TEMP::Temp *r) {
    Edge *e;
    e->src = t;
    e->dst = r;
    return degree[t] < K 
          || precolored.find(t) != precolored.end() 
          || adjSet.find(e) != adjSet.end();
  }

  bool conservative(std::set<TEMP::Temp *> nodes) {
    int k = 0;
    std::set<TEMP::Temp *>::iterator it = nodes.begin();
    for (; it != nodes.begin(); it++) {
      if (degree[*it] >= K) 
        k++;
    }
    return k < K;
  }

  TEMP::Temp* getAlias(TEMP::Temp *n) {
    if (coalescedNodes.find(n) != coalescedNodes.end()){
      return getAlias(alias[n]);
    } else return n;
  }

  void combine(TEMP::Temp *u, TEMP::Temp *v ) {
    if (freezeWorklist.find(v) != freezeWorklist.end()) {
      freezeWorklist.erase(freezeWorklist.find(v));
    } else {
      spillWorklist.erase(spillWorklist.find(v));
    }
    coalescedNodes.insert(v);

    alias[v] = u;
    moveList[u] = U::set_union(moveList[u], moveList[v]);
    std::set<TEMP::Temp *>vset;
    vset.insert(v);
    enableMoves(vset);

    std::set<TEMP::Temp *> adj = adjacent(v);
    std::set<TEMP::Temp *>::iterator it = adj.begin();
    for (; it != adj.end(); it++) {
      addEdge(*it, u);
      decrementDegree(*it);
    }
    if (degree[u] >= K && freezeWorklist.find(u) != freezeWorklist.end()) {
      freezeWorklist.erase(freezeWorklist.find(u));
      spillWorklist.insert(u);
    }
  }

  void freeze() {
    if (!freezeWorklist.empty()) {
      std::set<TEMP::Temp *>::iterator it = freezeWorklist.begin();
      freezeWorklist.erase(it);
      simplifyWorklist.insert(*it);
      freezeMoves(*it);
    }
  }

  void freezeMoves(TEMP::Temp *u) {
    std::set<AS::Instr *> moves = nodeMoves(u);
    std::set<AS::Instr *>::iterator it = moves.begin();
    for ( ; it != moves.end(); it++) {
      AS::MoveInstr *m = static_cast<AS::MoveInstr *>(*it);
      TEMP::Temp *x = m->src->head, *y = m->dst->head;
      TEMP::Temp *v;
      if (getAlias(y) == getAlias(u)) {
        v = getAlias(x);
      } else {
        v = getAlias(y);
      }
      activeMoves.erase(activeMoves.find(m));
      frozenMoves.insert(m);
      if (nodeMoves(v).empty() && degree[v] < K) {
        freezeWorklist.erase(freezeWorklist.find(v));
        simplifyWorklist.insert(v);
      }
    }
  }
  
  void slectSpill() {
    std::set<TEMP::Temp *>::iterator it = spillWorklist.begin();
    spillWorklist.erase(it);
    simplifyWorklist.insert(*it);
    freezeMoves(*it);
  }

  void assignColors() {
    while (!selectStack.empty()) {
      TEMP::Temp * n = selectStack.back();
      selectStack.pop_back();
      std::set<TEMP::Temp *> okColors = getColors();

      std::set<TEMP::Temp *> adj = adjList[n];
      std::set<TEMP::Temp *>::iterator it = adj.begin();

      for ( it; it != adj.end(); it++) {
        std::set<TEMP::Temp *> tmp = U::set_union(coloredNodes, precolored);
        if (tmp.find(getAlias(*it)) != tmp.end()) {
          okColors.erase(okColors.find(color[getAlias(*it)]));
        }
      }
      if(okColors.empty) {
        spilledNodes.insert(n);
      } else {
        coloredNodes.insert(n);
        std::set<TEMP::Temp *>::iterator c = okColors.begin();
        color[n] = *c;
      }
    }
    std::set<TEMP::Temp *>::iterator it = coalescedNodes.begin(); 
    for (; it != coalescedNodes.end(); it++) {
      color[*it] = color[getAlias(*it)];
    }
  }

  void rewriteProgram(G::Graph<AS::Instr> *flowgraph, F::Frame *f, AS::InstrList *il) {
    std::set<TEMP::Temp *>::iterator it = spilledNodes.begin();
    std::map<TEMP::Temp *, int> allocations;
    for ( ; it != spilledNodes.end(); it++) {
      f->allocLocal(true);
      allocations[*it] = f->getSize();
    }

    const char *store_template = "\tmovq\t`s0, (%s - %d)(%%rsp)";
    const char *load_template = "\tmovq\t(%s - %d)(%%rsp), `d0";

    G::NodeList<AS::Instr> *nl = flowgraph->Nodes();
    std::set<TEMP::Temp *> newTemps;
    while (nl) {
      TEMP::TempList *def = FG::Def(nl->head);
      TEMP::TempList *use = FG::Use(nl->head);
      
      TEMP::TempList *dp = def, *up = use;
      while (dp) {
        if (allocations[dp->head]) {
          // insert store instr;
          char instr[128];
          sprintf(instr, store_template, f->getFramesizeStr()->c_str(), allocations[dp->head]);
          AS::MoveInstr *ms = new AS::MoveInstr(instr, NULL, L(TEMP::Temp::NewTemp(), NULL));
          insertAfter(il, nl->head->NodeInfo(), new AS::InstrList(ms, NULL));
          newTemps.insert(dp->head);
        }
        dp = dp->tail;
      }

      while (up) {
        if (allocations[up->head]) {
          // insert load instr;
          char instr[128];
          sprintf(instr, load_template, f->getFramesizeStr()->c_str(), allocations[up->head]);
          AS::MoveInstr *ms = new AS::MoveInstr(instr, L(TEMP::Temp::NewTemp(), NULL), NULL);
          insertBefore(il, nl->head->NodeInfo(), new AS::InstrList(ms, NULL));
          newTemps.insert(up->head);
        }
        up = up->tail;
      }        
      nl = nl->tail;
    }

    spilledNodes.clear();
    initial = U::set_union(coloredNodes, U::set_union(coalescedNodes, newTemps));
    coloredNodes.clear();
    coalescedNodes.clear();
  }

  static void insertAfter(AS::InstrList *origin, AS::Instr *cur, AS::InstrList *il) {
    while (origin && origin->head != cur) {
      origin = origin->tail;
    }
    il->tail = origin->tail;
    origin->tail = il;
  }

  static void insertBefore(AS::InstrList *origin, AS::Instr *cur, AS::InstrList *il) {
    while (origin->tail && origin->tail->head != cur) {
      origin = origin->tail;
    }
    il->tail = origin->tail;
    origin->tail = il;
  }

  static std::set<TEMP::Temp *> getColors() {
    std::set<TEMP::Temp *> colors;
    TEMP::TempList *registers = F::registers();
    for (TEMP::TempList *tl = registers; tl; tl = tl->tail) {
      colors.insert(tl->head);
    }
    return colors;
  }
} // namespace 
