#ifndef TIGER_CANON_CANON_H_
#define TIGER_CANON_CANON_H_

#include <cstdio>

#include "tiger/frame/temp.h"
#include "tiger/translate/tree.h"

/* Forward Declarations */
namespace T {
class StmList;
class Exp;
class Stm;
}  // namespace T

namespace C {

class StmListList {
 public:
  T::StmList* head;
  StmListList* tail;

  StmListList(T::StmList* head, StmListList* tail) : head(head), tail(tail) {}
};

class Block {
 public:
  StmListList* stmLists;
  TEMP::Label* label;
};

class ExpRefList {
 public:
  T::Exp** head;
  ExpRefList* tail;

  ExpRefList(T::Exp** head, ExpRefList* tail) : head(head), tail(tail) {}
};

class StmAndExp {
 public:
  T::Stm* s;
  T::Exp* e;

  StmAndExp(T::Stm* s, T::Exp* e) : s(s), e(e) {}
};

/* From an arbitrary Tree statement, produce a list of cleaned trees
satisfying the following properties:
    1.  No SEQ's or ESEQ's
    2.  The parent of every CALL is an EXP(..) or a MOVE(TEMP t,..)
*/
T::StmList* Linearize(T::Stm* stm);

/* basicBlocks : Tree.stm list -> (Tree.stm list list * Tree.label)
    From a list of cleaned trees, produce a list of
    basic blocks satisfying the following properties:
    1. and 2. as above;
    3.  Every block begins with a LABEL;
    4.  A LABEL appears only at the beginning of a block;
    5.  Any JUMP or CJUMP is the last stm in a block;
    6.  Every block ends with a JUMP or CJUMP;
    Also produce the "label" to which control will be passed
    upon exit.
*/
Block BasicBlocks(T::StmList* stmList);

/* traceSchedule : Tree.stm list list * Tree.label -> Tree.stm list
    From a list of basic blocks satisfying properties 1-6,
    along with an "exit" label,
    produce a list of stms such that:
    1. and 2. as above;
    7. Every CJUMP(_,t,f) is immediately followed by LABEL f.
    The blocks are reordered to satisfy property 7; also
    in this reordering as many JUMP(T.NAME(lab)) statements
    as possible are eliminated by falling through into T.LABEL(lab).
*/
T::StmList* TraceSchedule(Block b);

}  // namespace C
#endif