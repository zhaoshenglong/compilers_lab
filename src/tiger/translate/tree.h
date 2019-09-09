#ifndef TIGER_TRANSLATE_TREE_H_
#define TIGER_TRANSLATE_TREE_H_

#include <cstdio>

#include "tiger/canon/canon.h"
#include "tiger/frame/temp.h"

/* Forward Declarations */
namespace C {
class Block;
class StmListList;
class ExpRefList;
class StmAndExp;
}  // namespace C

namespace T {

class Stm;
class Exp;
class NameExp;

class ExpList;
class StmList;

enum BinOp {
  PLUS_OP,
  MINUS_OP,
  MUL_OP,
  DIV_OP,
  AND_OP,
  OR_OP,
  LSHIFT_OP,
  RSHIFT_OP,
  ARSHIFT_OP,
  XOR_OP
};

enum RelOp {
  EQ_OP,
  NE_OP,
  LT_OP,
  GT_OP,
  LE_OP,
  GE_OP,
  ULT_OP,
  ULE_OP,
  UGT_OP,
  UGE_OP
};

/*
 * Statements
 */

class Stm {
 public:
  enum Kind { SEQ, LABEL, JUMP, CJUMP, MOVE, EXP };

  Kind kind;

  Stm(Kind kind) : kind(kind) {}
  virtual void Print(FILE* out, int d) const = 0;

  /*Lab6 only*/
  virtual Stm* Canon(Stm*) = 0;
};

class SeqStm : public Stm {
 public:
  Stm *left, *right;

  SeqStm(Stm* left, Stm* right) : Stm(SEQ), left(left), right(right) {
    assert(left);
  }
  void Print(FILE* out, int d) const override;

  /*Lab6 only*/
  Stm* Canon(Stm*) override;
};

class LabelStm : public Stm {
 public:
  TEMP::Label* label;

  LabelStm(TEMP::Label* label) : Stm(LABEL), label(label) {}
  void Print(FILE* out, int d) const override;

  /*Lab6 only*/
  Stm* Canon(Stm*) override;
};

class JumpStm : public Stm {
 public:
  NameExp* exp;
  TEMP::LabelList* jumps;

  JumpStm(NameExp* exp, TEMP::LabelList* jumps)
      : Stm(JUMP), exp(exp), jumps(jumps) {}
  void Print(FILE* out, int d) const override;

  /*Lab6 only*/
  Stm* Canon(Stm*) override;
};

class CjumpStm : public Stm {
 public:
  RelOp op;
  Exp *left, *right;
  TEMP::Label *true_label, *false_label;

  CjumpStm(RelOp op, Exp* left, Exp* right, TEMP::Label* true_label,
           TEMP::Label* false_label)
      : Stm(CJUMP),
        op(op),
        left(left),
        right(right),
        true_label(true_label),
        false_label(false_label) {}
  void Print(FILE* out, int d) const override;

  /*Lab6 only*/
  Stm* Canon(Stm*) override;
};

class MoveStm : public Stm {
 public:
  Exp *dst, *src;

  MoveStm(Exp* dst, Exp* src) : Stm(MOVE), dst(dst), src(src) {}
  void Print(FILE* out, int d) const override;

  /*Lab6 only*/
  Stm* Canon(Stm*) override;
};

class ExpStm : public Stm {
 public:
  Exp* exp;

  ExpStm(Exp* exp) : Stm(EXP), exp(exp) {}
  void Print(FILE* out, int d) const override;

  /*Lab6 only*/
  Stm* Canon(Stm*) override;
};

/*
 *Expressions
 */

class Exp {
 public:
  enum Kind { BINOP, MEM, TEMP, ESEQ, NAME, CONST, CALL };

  Kind kind;

  Exp(Kind kind) : kind(kind) {}
  virtual void Print(FILE* out, int d) const = 0;

  /*Lab6 only*/
  virtual C::StmAndExp Canon(Exp*) = 0;
};

class BinopExp : public Exp {
 public:
  BinOp op;
  Exp *left, *right;

  BinopExp(BinOp op, Exp* left, Exp* right)
      : Exp(BINOP), op(op), left(left), right(right) {}
  void Print(FILE* out, int d) const override;

  /*Lab6 only*/
  C::StmAndExp Canon(Exp*) override;
};

class MemExp : public Exp {
 public:
  Exp* exp;

  MemExp(Exp* exp) : Exp(MEM), exp(exp) {}
  void Print(FILE* out, int d) const override;

  /*Lab6 only*/
  C::StmAndExp Canon(Exp*) override;
};

class TempExp : public Exp {
 public:
  TEMP::Temp* temp;

  TempExp(TEMP::Temp* temp) : Exp(TEMP), temp(temp) {}
  void Print(FILE* out, int d) const override;

  /*Lab6 only*/
  C::StmAndExp Canon(Exp*) override;
};

class EseqExp : public Exp {
 public:
  Stm* stm;
  Exp* exp;

  EseqExp(Stm* stm, Exp* exp) : Exp(ESEQ), stm(stm), exp(exp) {}
  void Print(FILE* out, int d) const override;

  /*Lab6 only*/
  C::StmAndExp Canon(Exp*) override;
};

class NameExp : public Exp {
 public:
  TEMP::Label* name;

  NameExp(TEMP::Label* name) : Exp(NAME), name(name) {}
  void Print(FILE* out, int d) const override;

  /*Lab6 only*/
  C::StmAndExp Canon(Exp*) override;
};

class ConstExp : public Exp {
 public:
  int consti;

  ConstExp(int consti) : Exp(CONST), consti(consti) {}
  void Print(FILE* out, int d) const override;

  /*Lab6 only*/
  C::StmAndExp Canon(Exp*) override;
};

class CallExp : public Exp {
 public:
  Exp* fun;
  ExpList* args;

  CallExp(Exp* fun, ExpList* args) : Exp(CALL), fun(fun), args(args) {}
  void Print(FILE* out, int d) const override;

  /*Lab6 only*/
  C::StmAndExp Canon(Exp*) override;
};

class ExpList {
 public:
  Exp* head;
  ExpList* tail;

  ExpList(Exp* head, ExpList* tail) : head(head), tail(tail) {}
};

class StmList {
 public:
  Stm* head;
  StmList* tail;

  StmList(Stm* head, StmList* tail) : head(head), tail(tail) {}
  void Print(FILE* out) const;
};

RelOp notRel(RelOp);  /* a op b    ==     not(a notRel(op) b)  */
RelOp commute(RelOp); /* a op b    ==    b commute(op) a       */

}  // namespace T

#endif  // TIGER_TRANSLATE_TREE_H_
