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

  // Override default set comparator, used in adjSet
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

  static std::set<LIVE::MoveList *> coalescedMoves;
  static std::set<LIVE::MoveList *> constrainedMoves;
  static std::set<LIVE::MoveList *> frozenMoves;
  static std::set<LIVE::MoveList *> worklistMoves;
  static std::set<LIVE::MoveList *> activeMoves;

  static std::map<TEMP::Temp *, int> degree;
  static std::map<TEMP::Temp *, TEMP::Temp *> alias;
  static std::map<TEMP::Temp *, std::set<LIVE::MoveList *> > moveList;
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
  std::set<LIVE::MoveList *> nodeMoves(TEMP::Temp *n);
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

  bool movesDuplicate(std::set<LIVE::MoveList *>, LIVE::MoveList *);
  static void insertAfter(AS::InstrList *il, AS::InstrList *nl);
  static void insertBefore(AS::InstrList *il, AS::InstrList *nl);
  TEMP::TempList *getDefs(AS::Instr *);
  TEMP::TempList *getUses(AS::Instr *);
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
    LIVE::MoveList *movelist = livegraph->moves;
    G::NodeList<TEMP::Temp> *nodes = livegraph->graph->Nodes();

    // add movelist
    while (movelist) {
      TEMP::Temp *src = movelist->src->NodeInfo();
      TEMP::Temp *dst = movelist->dst->NodeInfo();
      if (!movesDuplicate(worklistMoves, movelist)) {
        worklistMoves.insert(movelist);
      }
      if (!movesDuplicate(moveList[src], movelist)) {
        moveList[src].insert(movelist);
      }
      if (!movesDuplicate(moveList[dst], movelist)) {
        moveList[dst].insert(movelist);
      }
    }
    
    // add edges
    nodes = livegraph->graph->Nodes();
    while (nodes) {
      G::NodeList<TEMP::Temp> *adj = nodes->head->Adj();
      for (; adj; adj = adj->tail) {
        addEdge(nodes->head->NodeInfo(), adj->head->NodeInfo());
      }
      nodes = nodes->tail;
    }

    // add initials
    nodes = livegraph->graph->Nodes();
    while (nodes) {
      if (precolored.find(nodes->head->NodeInfo()) != precolored.end()) {
        color[nodes->head->NodeInfo()] = *precolored.find(nodes->head->NodeInfo());
      } else {
        initial.insert(nodes->head->NodeInfo());
      }
      alias[nodes->head->NodeInfo()] = nodes->head->NodeInfo();
      nodes = nodes->tail;
    }
  }

  void addEdge(TEMP::Temp *u, TEMP::Temp *v) {
    Edge e = Edge(u, v);
    assert(u);
    assert(v);
    if (u != v && adjSet.find(&e) == adjSet.end()) {
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

  std::set<LIVE::MoveList *> nodeMoves(TEMP::Temp *n) {
    return U::set_intersect(moveList[n], 
                    U::set_union(activeMoves, worklistMoves));
  }

  bool moveRelated(TEMP::Temp *n) {
    return !nodeMoves(n).empty();
  }

  void simplify() {
    if (!simplifyWorklist.empty() ) {
      std::set<TEMP::Temp *>::iterator n = simplifyWorklist.begin();
      TEMP::Temp *t = *n;
      simplifyWorklist.erase(n);
      selectStack.push_back(t);
      selectStackSet.insert(t);

      std::set<TEMP::Temp *> adj = adjacent(t);
      std::set<TEMP::Temp *>::iterator it = adj.begin();
      for (; it != adj.end(); it++) {
        decrementDegree(*it);
      }
    }
  }

  void decrementDegree(TEMP::Temp *m) {
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
    std::set<TEMP::Temp *>::iterator it = nodes.begin();
    for (; it != nodes.end(); it++) {
      std::set<LIVE::MoveList *> nodemoves = nodeMoves(*it);
      std::set<LIVE::MoveList *>::iterator nit = nodemoves.begin();
      for (; nit != nodemoves.end(); nit++) {
        if (activeMoves.find(*nit) != activeMoves.end()) {
          worklistMoves.insert(*nit);
          activeMoves.erase(*nit);
        }
      }
    }
  }

  void coalesce() {
    if (!worklistMoves.empty()) {
      std::set<LIVE::MoveList *>::iterator m = worklistMoves.begin();
      TEMP::Temp *x = (*m)->src->NodeInfo();
      TEMP::Temp *y = (*m)->dst->NodeInfo();
      x = getAlias(x);
      y = getAlias(y);

      TEMP::Temp *t;
      if (precolored.find(y) != precolored.end()) {
        // make sure precolored is in the first place if exists.
        t = x;
        x = y;
        y = t;
      }
      worklistMoves.erase(*m);

      // George algorithm
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
        // degree not change
        coalescedMoves.insert(*m);
        addWorklist(x);
      } else if(precolored.find(y) != precolored.end() 
                || adjSet.find(&e) != adjSet.end()) {
        // degree not change, simply remove MOVE[x->y]
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
        // The move is not prepared for combination
        activeMoves.insert(*m);
      }
    }
  }

  // Whenever coalesce two nodes, we need to chekc if that can be add to 
  // simplyfy worklist.
  void addWorklist(TEMP::Temp *u) {
    if(precolored.find(u) == precolored.end() && !moveRelated(u) && degree[u] < K) {
      freezeWorklist.erase(u);
      simplifyWorklist.insert(u);
    }
  }

  // George algorithm, precolored chcking is not necessary I think
  bool ok(TEMP::Temp *t, TEMP::Temp *r) {
    Edge e = Edge(t, r);
    return degree[t] < K 
          || precolored.find(t) != precolored.end() 
          || adjSet.find(&e) != adjSet.end();
  }

  // Brigg algorithm
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
      freezeWorklist.erase(v);
    } else {
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
      // connevt u & *it for each v->it
      addEdge(*it, u);
      // decremet degree, since v is removed
      decrementDegree(*it);
    }
    if (degree[u] >= K && freezeWorklist.find(u) != freezeWorklist.end()) {
      freezeWorklist.erase(u);
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
    std::set<LIVE::MoveList *> moves = nodeMoves(u);
    std::set<LIVE::MoveList *>::iterator it = moves.begin();
    for ( ; it != moves.end(); it++) {
      TEMP::Temp *x = (*it)->src->NodeInfo(), *y = (*it)->dst->NodeInfo();
      TEMP::Temp *v;
      if (getAlias(y) == getAlias(u)) {
        v = getAlias(x);
      } else {
        v = getAlias(y);
      }
      activeMoves.erase(*it);
      frozenMoves.insert(*it);
      if (nodeMoves(v).empty() && degree[v] < K) {
        freezeWorklist.erase(v);
        simplifyWorklist.insert(v);
      }
    }
  }
  
  void selectSpill() {
    // select the highest degree node
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
    spillWorklist.erase(victim);
    simplifyWorklist.insert(victim);
    freezeMoves(victim);
  }

  void assignColors() {
    while (!selectStack.empty()) {
      TEMP::Temp *n = selectStack.back();
      selectStack.pop_back();
      if (selectStackSet.find(n) != selectStackSet.end()) {
        selectStackSet.erase(n);
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

    AS::InstrList *i = il,
                  *prev = NULL;
    while (i) {
      TEMP::TempList *def = getDefs(i->head);
      TEMP::TempList *use = getUses(i->head);
      
      while (def) {
        if (allocations[def->head]) {
          TEMP::Temp *t = TEMP::Temp::NewTemp();
          // insert store instr;
          char instr[128];
          sprintf(instr, store_template, f->getFramesizeStr()->c_str(), allocations[def->head]);
          AS::OperInstr *ms = new AS::OperInstr(instr, NULL, L(t, L(F::SP(), NULL)), NULL);
          insertAfter(i, new AS::InstrList(ms, NULL));
          def->head = t;
          // i->head->Print(stdout, TEMP::Map::LayerMap(F::tempMap(), TEMP::Map::Name()));
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
          AS::OperInstr *ms = new AS::OperInstr(instr, L(t, NULL),  L(F::SP(), NULL), NULL);
          assert(prev);
          insertAfter(prev, new AS::InstrList(ms, NULL));
          use->head = t;
          // prev->head->Print(stdout, TEMP::Map::LayerMap(F::tempMap(), TEMP::Map::Name()));
        }
        use = use->tail;
      }        
      prev = i;
      i = i->tail;
    }
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

  bool movesDuplicate(std::set<LIVE::MoveList *> moves, LIVE::MoveList * t) {
    std::set<LIVE::MoveList *>::iterator it = moves.begin();

    bool found = false;
    for (; it != moves.end(); it++) {
      if((*it)->dst == t->dst && (*it)->src == t->src
          || ((*it)->dst == t->src && (*it)->src == t->dst)) {
        return true;
      }
    }
    return false;
  }
} // namespace 
