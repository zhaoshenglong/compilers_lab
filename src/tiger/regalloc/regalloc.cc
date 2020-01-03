#include "tiger/regalloc/regalloc.h"
#include "tiger/liveness/liveness.h"
#include <list>
#include <vector>
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

  struct edge_comp {
    bool operator() ( Edge* lhs,  Edge* rhs) const {
      return lhs->src->Int() < rhs->src->Int() 
            || ( lhs->src->Int() == rhs->src->Int() 
                && lhs->dst->Int() < rhs->dst->Int() );
    }
  };

  static TEMP::TempList *L(TEMP::Temp *r1, TEMP::TempList *r2) {
    return new TEMP::TempList(r1, r2);
  }

  static int K = 16;
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
  static std::set<Edge *, edge_comp> adjSet;
  static std::map<TEMP::Temp *, std::set<TEMP::Temp *> > adjList;
  static std::map<TEMP::Temp *, TEMP::Temp *> color;

  static std::set<TEMP::Temp *> getColors();
  void initWorklist();
  void mainProcedure(RA::Result *res, F::Frame *f, AS::InstrList *il);
  void build(LIVE::LiveGraph *livegraph, AS::InstrList *il);
  void addEdge(TEMP::Temp *src, TEMP::Temp *dst);
  void makeWorklist();
  std::set<TEMP::Temp *> adjacent(TEMP::Temp *n);
  std::set<AS::Instr  *> nodeMoves(TEMP::Temp *n);
  bool moveRelated(TEMP::Temp *);
  
  void simplify();
  void decrementDegree(TEMP::Temp *);
  void enableMoves(std::set<TEMP::Temp *>);
  
  void coalesce();
  void addWorklist(TEMP::Temp *n);
  bool ok(TEMP::Temp *, TEMP::Temp *);
  bool conservative(std::set<TEMP::Temp *>);
  TEMP::Temp *getAlias(TEMP::Temp *);
  void combine(TEMP::Temp *u, TEMP::Temp *v);
   
  void freeze();
  void freezeMoves(TEMP::Temp *n);

  void selectSpill();
  void assignColors();

  void rewriteProgram(G::Graph<AS::Instr> *flowgraph, F::Frame *f, AS::InstrList * il);

  static void insertAfter(AS::InstrList *il, AS::InstrList *nl);
  static void insertBefore(AS::InstrList *il, AS::InstrList *nl);
  TEMP::TempList *getDefs(AS::Instr *);
  TEMP::TempList *getUses(AS::Instr *);
  void print(AS::InstrList *);
} // namespace 

namespace RA {

Result RegAlloc(F::Frame* f, AS::InstrList* il) {

  Result res;
  initWorklist();
  mainProcedure(&res, f, il);

  TEMP::Map *coloring = TEMP::Map::Empty();
  TEMP::Map *registers = F::tempMap();
  std::map<TEMP::Temp *, TEMP::Temp *>::iterator it = color.begin();   

  for (; it != color.end(); it++) {
    coloring->Enter(it->first, registers->Look(it->second));
    printf("color[%d] -> %s\n", it->first->Int(), registers->Look(it->second)->c_str());
  }
  
  
  while (il->head->kind == AS::Instr::MOVE &&
        !coloring->Look(static_cast<AS::MoveInstr*>(il->head)->src->head)
        ->compare(*coloring->Look(static_cast<AS::MoveInstr*>(il->head)->dst->head))
       ) {
    il = il->tail;
  }
  AS::InstrList *cur = il, *next = NULL;
  while (cur && (next = cur->tail) && next) {
    if(next->head->kind == AS::Instr::MOVE && 
      !coloring->Look(static_cast<AS::MoveInstr*>(next->head)->src->head)
        ->compare(*coloring->Look(static_cast<AS::MoveInstr*>(next->head)->dst->head))) {
      cur->tail = next->tail;
    } else {
      cur = cur->tail;
    }
  }
  
  res.coloring = coloring;
  res.il = il;
  
  return res;
}

}  // namespace RA

namespace 
{
  void initWorklist() {
    precolored = getColors();
    for (std::set<TEMP::Temp *>::iterator it = precolored.begin(); it != precolored.end(); it++) {
      printf("t%d -> %s\n", (*it)->Int(), F::tempMap()->Look((*it))->c_str());
    }
    
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
    G::NodeList<AS::Instr> *n = flowgraph->Nodes();
    
    print(il);

    LIVE::LiveGraph livegraph = LIVE::Liveness(flowgraph);
    build(&livegraph, il);
    makeWorklist();
    while (!simplifyWorklist.empty() || !worklistMoves.empty()
    || !freezeWorklist.empty() || !spillWorklist.empty()) {
      if (!simplifyWorklist.empty()) simplify();
      else if (!worklistMoves.empty()) coalesce();
      else if (!freezeWorklist.empty()) freeze();
      else if (!spillWorklist.empty()) selectSpill();
    }
    assignColors();
    if (!spilledNodes.empty()){
      rewriteProgram(flowgraph, f, il);
      initWorklist();
      mainProcedure(res, f, il);
    }
  }

  void build(LIVE::LiveGraph *livegraph, AS::InstrList *il) {
    printf("build\n");
    LIVE::MoveList *movelist = livegraph->moves;
    G::NodeList<TEMP::Temp> *nodes = livegraph->graph->Nodes();
    AS::InstrList *ilp = il;

    while (ilp) {
      if(ilp->head->kind == AS::Instr::MOVE) {
        worklistMoves.insert(ilp->head);
        AS::MoveInstr *mi = static_cast<AS::MoveInstr*>(ilp->head);
        TEMP::TempList *stl = mi->src,
                       *dtl = mi->dst;
        while (stl) {
          moveList[stl->head].insert(ilp->head);
          stl = stl->tail;
        }
        while (dtl) {
          moveList[dtl->head].insert(ilp->head);
          dtl = dtl->tail;
        }
      }
      ilp = ilp->tail;
    }
    nodes = livegraph->graph->Nodes();
    
    while (nodes) {
      G::NodeList<TEMP::Temp> *adj = nodes->head->Adj();
      for (; adj; adj = adj->tail) {
        addEdge(nodes->head->NodeInfo(), adj->head->NodeInfo());
      }
      nodes = nodes->tail;
    }

    nodes = livegraph->graph->Nodes();
    while (nodes) {
      if (precolored.find(nodes->head->NodeInfo()) != precolored.end()) {
        color[nodes->head->NodeInfo()] = *precolored.find(nodes->head->NodeInfo());
      } else {
        initial.insert(nodes->head->NodeInfo());
      }
      nodes = nodes->tail;
    }
  }

  void addEdge(TEMP::Temp *u, TEMP::Temp *v) {
    Edge e = Edge(u, v);
    assert(u);
    assert(v);
    if (u != v && adjSet.find(&e) == adjSet.end()) {
      printf("Add   : %d ---- %d\n", u->Int(), v->Int());
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
    printf("makeWorklist\n");
    std::set<TEMP::Temp *>::iterator it = initial.begin();
    for (; it != initial.end(); it++) {
      if (degree[*it] >= K) {
        spillWorklist.insert(*it);
      } else if (moveRelated(*it)) {
        freezeWorklist.insert(*it);
      } else {
        simplifyWorklist.insert(*it);
      }
    }
    initial.clear();
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
    printf("Simplify\n");
    if (!simplifyWorklist.empty() ) {
      std::set<TEMP::Temp *>::iterator n = simplifyWorklist.begin();
      TEMP::Temp *t = *n;
      simplifyWorklist.erase(n);
      selectStack.push_back(t);
      selectStackSet.insert(t);
      printf("push back %d\n", (t)->Int());

      std::set<TEMP::Temp *> adj = adjacent(t);
      std::set<TEMP::Temp *>::iterator it = adj.begin();
      for (; it != adj.end(); it++) {
        decrementDegree(*it);
      }
    }
  }

  void decrementDegree(TEMP::Temp *m) {
    printf("DecrementDegree\n");
    degree[m] = degree[m] - 1;
    if (degree[m] == K - 1) {
      std::set<TEMP::Temp *>mset;
      mset.insert(m);
      enableMoves(U::set_union(adjacent(m), mset));
      spillWorklist.erase(m);
      if (moveRelated(m)) {
        freezeWorklist.insert(m);
      } else {
        simplifyWorklist.insert(m);
      }
    }
  }

  void enableMoves(std::set<TEMP::Temp *> nodes) {
    printf("enableMoves\n");
    std::set<TEMP::Temp *>::iterator it = nodes.begin();
    for (; it != nodes.end(); it++) {
      std::set<AS::Instr *> nodemoves = nodeMoves(*it);
      std::set<AS::Instr *>::iterator nit = nodemoves.begin();
      for (; nit != nodemoves.end(); nit++) {
        if (activeMoves.find(*nit) != activeMoves.end()) {
          worklistMoves.insert(*nit);
          activeMoves.erase(*nit);
        }
      }
    }
  }

  void coalesce() {
    printf("coalesce\n");
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
      worklistMoves.erase(*m);

      bool test = true;
      std::set<TEMP::Temp*> adj = adjacent(y);
      std::set<TEMP::Temp*>::iterator it = adj.begin();
      for (; it != adj.end(); it++) {
        if (!ok(*it, x)) {
          test = false;
          break;
        }
      }
      
      Edge e = Edge(x, y);
      if (x == y) {
        coalescedMoves.insert(*m);
        addWorklist(x);
      } else if(precolored.find(y) != precolored.end() 
                || adjSet.find(&e) != adjSet.end()) {
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
    printf("addWorklist\n");
    if(precolored.find(u) == precolored.end() && !moveRelated(u) && degree[u] < K) {
      freezeWorklist.erase(u);
      simplifyWorklist.insert(u);
    }
  }

  bool ok(TEMP::Temp *t, TEMP::Temp *r) {
    printf("ok\n");
    Edge e = Edge(t, r);
    return degree[t] < K 
          || precolored.find(t) != precolored.end() 
          || adjSet.find(&e) != adjSet.end();
  }

  bool conservative(std::set<TEMP::Temp *> nodes) {
    printf("conservative\n");
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
    printf("combine\n");
    if (freezeWorklist.find(v) != freezeWorklist.end()) {
      freezeWorklist.erase(v);
    } else {
      printf("combine spill");
      if (spillWorklist.find(v) != spillWorklist.end()) {
        spillWorklist.erase(v);
      }
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
      freezeWorklist.erase(u);
      spillWorklist.insert(u);
    }
  }

  void freeze() {
    printf("freeze\n");
    if (!freezeWorklist.empty()) {
      std::set<TEMP::Temp *>::iterator it = freezeWorklist.begin();
      freezeWorklist.erase(it);
      simplifyWorklist.insert(*it);
      freezeMoves(*it);
    }
  }

  void freezeMoves(TEMP::Temp *u) {
    printf("freezeMoves\n");
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
      printf("Erase activemoves\n");
      activeMoves.erase(m);
      frozenMoves.insert(m);
      printf("ERASED activmoves\n");
      if (nodeMoves(v).empty() && degree[v] < K) {
        freezeWorklist.erase(v);
        printf("erase freezeworklist\n");
        simplifyWorklist.insert(v);
      }
    }
  }
  
  void selectSpill() {
    printf("SelectSpill\n");
    std::set<TEMP::Temp *>::iterator it = spillWorklist.begin();
    TEMP::Temp *victim = *it;
    int max_degree = degree[*it];
    for (; it != spillWorklist.end(); it++) {
      if(spilledNodes.find(*it) == spilledNodes.end() &&
         precolored.find(*it) == precolored.end()) {
        std::map<TEMP::Temp *, int>::iterator i = degree.find(*it);
        if (i->second > max_degree) {
          victim = *it;
          max_degree = i->second;
        }
      }
    }
    printf("========== Select spill: %d, const: %d ============\n", victim->Int(), max_degree);
    spillWorklist.erase(victim);
    simplifyWorklist.insert(victim);
    freezeMoves(victim);
  }

  void assignColors() {
    printf("selectStack empty: %d, %d\n", selectStackSet.empty(), selectStack.empty());
    while (!selectStack.empty()) {
      TEMP::Temp *n = selectStack.back();
      selectStack.pop_back();
      printf("pop stack\n");
      if (selectStackSet.find(n) != selectStackSet.end()) {
        selectStackSet.erase(n);
      } else {
        printf("set is null!\n");
      }
      std::set<TEMP::Temp *> okColors = getColors();

      std::set<TEMP::Temp *> adj = adjList[n];
      std::set<TEMP::Temp *>::iterator it = adj.begin();

      for (; it != adj.end(); it++) {
        std::set<TEMP::Temp *> tmp = U::set_union(coloredNodes, precolored);
        if (tmp.find(getAlias(*it)) != tmp.end()) {
          okColors.erase(color[getAlias(*it)]);
        }
      }
      if(okColors.empty()) {
        printf("Insert into spilledNodes]n");
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
      printf("Allocate for spillednodes: %d\n", (*it)->Int());
    }
    printf("Rewrite program\n");
    const char *store_template = "\tmovq\t`s0, (%s - %d)(%%rsp)";
    const char *load_template = "\tmovq\t(%s - %d)(%%rsp), `d0";

    AS::InstrList *i = il,
                  *prev = NULL;
    while (i) {
      TEMP::TempList *def = getDefs(i->head);
      TEMP::TempList *use = getUses(i->head);
      
      while (def) {
        if (allocations[def->head]) {
          TEMP::Temp *t = TEMP::Temp::NewTemp();
          printf("Found def:, def->head: %d, off: %d\n", def->head->Int(), allocations[def->head]);
          // insert store instr;
          char instr[128];
          sprintf(instr, store_template, f->getFramesizeStr()->c_str(), allocations[def->head]);
          AS::OperInstr *ms = new AS::OperInstr(instr, NULL, L(t, L(F::SP(), NULL)), NULL);
          insertAfter(i, new AS::InstrList(ms, NULL));
          def->head = t;

          printf("insert asfter");
          i->head->Print(stdout, TEMP::Map::LayerMap(F::tempMap(), TEMP::Map::Name()));
          i = i->tail;
        }
        def = def->tail;
      }

      while (use) {
        if (allocations[use->head]) {
          // insert load instr;
          TEMP::Temp *t = TEMP::Temp::NewTemp();
          char instr[128];
          sprintf(instr, load_template, f->getFramesizeStr()->c_str(), allocations[use->head]);
          AS::OperInstr *ms = new AS::OperInstr(instr, L(t, L(F::SP(), NULL)), NULL, NULL);
          assert(prev);
          insertAfter(prev, new AS::InstrList(ms, NULL));
          use->head = t;
          printf("insert asfter");
          prev->head->Print(stdout, TEMP::Map::LayerMap(F::tempMap(), TEMP::Map::Name()));
        }
        use = use->tail;
      }        
      prev = i;
      i = i->tail;
    }
    print(il);
  }

  static void insertAfter(AS::InstrList *il,  AS::InstrList *nl) {
    nl->tail = il->tail;
    il->tail = nl;
  }

  static std::set<TEMP::Temp *> getColors() {
    static std::set<TEMP::Temp *>* colors = NULL;
    if (!colors) {
      colors = new std::set<TEMP::Temp *>();
      TEMP::TempList *registers = F::registers();
      for (TEMP::TempList *tl = registers; tl; tl = tl->tail) {
        colors->insert(tl->head);
      }
    }
    return *colors;
  }

  TEMP::TempList *getDefs(AS::Instr *i) {
    if (i->kind == AS::Instr::LABEL) {
      // Label Instr Has No Defs
      return NULL;
    } else if (i->kind == AS::Instr::MOVE) {
      return static_cast<AS::MoveInstr *>(i)->dst;
    } else {
      return static_cast<AS::OperInstr *>(i)->dst;
    }
  }
  TEMP::TempList *getUses(AS::Instr *i) {
    if(i->kind == AS::Instr::LABEL) {
      // Label Instr Has NO Use
      return NULL;;
    } else if (i->kind == AS::Instr::MOVE) {
      return static_cast<AS::MoveInstr *>(i)->src;
    } else {
      return static_cast<AS::OperInstr *>(i)->src;
    }
  }

  void print(AS::InstrList *il) {
    printf("Printing\n");
    for (; il; il = il->tail) {
      il->head->Print(stdout, TEMP::Map::LayerMap(F::tempMap(), TEMP::Map::Name()));
    }
  }
} // namespace 
