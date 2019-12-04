#ifndef TIGER_UTIL_GRAPH_H_
#define TIGER_UTIL_GRAPH_H_

#include "tiger/util/table.h"

namespace G {

template <class T>
class Node;
template <class T>
class NodeList;

template <class T>
class Graph {
 public:
  /* Make a new graph */
  Graph() : nodecount(0), mynodes(nullptr), mylast(nullptr) {}

  /* Get the list of nodes belonging to graph */
  NodeList<T>* Nodes();

  /* Make a new node in graph "g", with associated "info" */
  Node<T>* NewNode(T* info);

  /* Make a new edge joining nodes "from" and "to", which must belong
  to the same graph */
  static void AddEdge(Node<T>* from, Node<T>* to);

  /* Delete the edge joining "from" and "to" */
  static void RmEdge(Node<T>* from, Node<T>* to);

  /* Show all the nodes and edges in the graph, using the function "showInfo"
    to print the name of each node */
  static void Show(FILE* out, NodeList<T>* p, void showInfo(T*));

  int nodecount;
  NodeList<T>* mynodes;
  NodeList<T>* mylast;
};

template <class T>
class Node {
  template <class NodeType>
  friend class Graph;

 public:
  /* Tell if there is an edge from this node to "to" */
  bool GoesTo(Node<T>* n);

  /* return length of predecessor list for node n */
  int InDegree();

  /* return length of successor list for node n */
  int OutDegree();

  /* Get all the successors and predecessors */
  NodeList<T>* Adj();

  /* Get all the successors of node */
  NodeList<T>* Succ();

  /* Get all the predecessors of node */
  NodeList<T>* Pred();

  /* Tell how many edges lead to or from node */
  int Degree();

  /* Get the "info" associated with node */
  T* NodeInfo();

  /* Get the "mykey_" associated with node */
  int Key();

 private:
  Graph<T>* mygraph_;
  int mykey_;
  NodeList<T>* succs_;
  NodeList<T>* preds_;
  T* info_;
  Node<T>()
      : mygraph_(nullptr),
        mykey_(0),
        succs_(nullptr),
        preds_(nullptr),
        info_(nullptr) {}
};

template <class T>
class NodeList {
 public:
  Node<T>* head;
  NodeList<T>* tail;

  /* Make a NodeList cell */
  NodeList<T>(Node<T>* head, NodeList<T>* tail) : head(head), tail(tail) {}

  /* Tell if "a" is in the list */
  bool InNodeList(Node<T>* a);

  /* put list b at the back of list a and return the concatenated list */
  static NodeList<T>* CatList(NodeList<T>* a, NodeList<T>* b);
  static NodeList<T>* DeleteNode(Node<T>* a, NodeList<T>* l);
};

/* generic creation of Node<T> */
template <class T>
Node<T>* Graph<T>::NewNode(T* info) {
  Node<T>* n = new Node<T>();
  NodeList<T>* p = new NodeList<T>(n, nullptr);
  n->mygraph_ = this;
  n->mykey_ = this->nodecount++;

  if (this->mylast == nullptr)
    this->mynodes = this->mylast = p;
  else
    this->mylast = this->mylast->tail = p;

  n->succs_ = nullptr;
  n->preds_ = nullptr;
  n->info_ = info;
  return n;
}

template <class T>
bool Node<T>::GoesTo(Node<T>* n) {
  return this->succs_->InNodeList(n);
}

template <class T>
void Graph<T>::AddEdge(Node<T>* from, Node<T>* to) {
  assert(from);
  assert(to);
  assert(from->mygraph_ == to->mygraph_);
  if (from->GoesTo(to)) return;
  to->preds_ = new NodeList<T>(from, to->preds_);
  from->succs_ = new NodeList<T>(to, from->succs_);
}

template <class T>
void Graph<T>::RmEdge(Node<T>* from, Node<T>* to) {
  assert(from && to);
  to->preds_ = NodeList<T>::DeleteNode(from, to->Pred());
  from->succs_ = NodeList<T>::DeleteNode(to, from->Succ());
}

template <class T>
int Node<T>::InDegree() {
  int deg = 0;
  NodeList<T>* p;
  for (p = this->preds_; p != nullptr; p = p->tail) deg++;
  return deg;
}

template <class T>
int Node<T>::OutDegree() {
  int deg = 0;
  NodeList<T>* p;
  for (p = this->succs_; p != nullptr; p = p->tail) deg++;
  return deg;
}

template <class T>
int Node<T>::Degree() {
  return InDegree() + OutDegree();
}

template <class T>
NodeList<T>* Node<T>::Adj() {
  return NodeList<T>::CatList(this->succs_, this->preds_);
}

template <class T>
NodeList<T>* Node<T>::Succ() {
  return this->succs_;
}

template <class T>
NodeList<T>* Node<T>::Pred() {
  return this->preds_;
}

template <class T>
T* Node<T>::NodeInfo() {
  return this->info_;
}

template <class T>
int Node<T>::Key() {
  return this->mykey_;
}

template <class T>
NodeList<T>* Graph<T>::Nodes() {
  return this->mynodes;
}

/* return true if a is in l list */
template <class T>
bool NodeList<T>::InNodeList(Node<T>* a) {
  NodeList<T>* p;
  for (p = this; p != nullptr; p = p->tail) {
    if (p->head == a) return true;
  }
  return false;
}

template <class T>
NodeList<T>* NodeList<T>::DeleteNode(Node<T>* a, NodeList<T>* l) {
  assert(a && l);
  if (a == l->head)
    return l->tail;
  else
    return new NodeList<T>(l->head, DeleteNode(a, l->tail));
}

template <class T>
NodeList<T>* NodeList<T>::CatList(NodeList<T>* a, NodeList<T>* b) {
  if (a == nullptr)
    return b;
  else
    return new NodeList<T>(a->head, CatList(a->tail, b));
}

/* The type of "tables" mapping graph-nodes to information */
template <typename T, typename ValueType>
using Table = TAB::Table<Node<T>, ValueType>;

/*
 * Print a human-readable dump for debugging.
 */
template <class T>
void Graph<T>::Show(FILE* out, NodeList<T>* p, void showInfo(T*)) {
  for (; p != nullptr; p = p->tail) {
    Node<T>* n = p->head;
    NodeList<T>* q;
    assert(n);
    if (showInfo) showInfo(n->NodeInfo());
    fprintf(out, " (%d): ", n->Key());
    for (q = n->Succ(); q != nullptr; q = q->tail)
      fprintf(out, "%d ", q->head->Key());
    fprintf(out, "\n");
  }
}

}  // namespace G

#endif
