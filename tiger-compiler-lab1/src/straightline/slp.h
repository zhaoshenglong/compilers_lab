#ifndef STRAIGHTLINE_SLP_H_
#define STRAIGHTLINE_SLP_H_

#include <algorithm>
#include <cassert>
#include <string>

namespace A {

class Stm;
class Exp;
class ExpList;
enum StmKind { COMPOUND_SMT = 0, ASSIGN_SMT, PRINT_SMT };
enum BinOp { PLUS = 0, MINUS, TIMES, DIV };
enum ExpKind { ID_EXP = 0, NUM_EXP, OP_EXP, ESEQ_EXP };
enum ExpListKind { PAIR_EXP_LIST = 0, LAST_EXP_LIST };

/* some data structures used by interp */
class Table;
class IntAndTable;

class Stm {
 public:
  Stm(StmKind kind) : kind(kind) {}
  virtual int MaxArgs() const = 0;
  virtual Table *Interp(Table *) const = 0;
  
 protected:
  StmKind kind;
};

class CompoundStm : public Stm {
 public:
  CompoundStm(Stm *stm1, Stm *stm2)
      : Stm(COMPOUND_SMT), stm1(stm1), stm2(stm2) {}
  int MaxArgs() const override;
  Table *Interp(Table *) const override;

 private:
  Stm *stm1, *stm2;
};

class AssignStm : public Stm {
 public:
  AssignStm(std::string id, Exp *exp) : Stm(ASSIGN_SMT), id(id), exp(exp) {}
  int MaxArgs() const override;
  Table *Interp(Table *) const override;

 private:
  std::string id;
  Exp *exp;
};

class PrintStm : public Stm {
 public:
  PrintStm(ExpList *exps) : Stm(PRINT_SMT), exps(exps) {}
  int MaxArgs() const override;
  Table *Interp(Table *) const override;

 private:
  ExpList *exps;
};

class Exp {
 public:
  Exp(ExpKind kind) : kind(kind) {}
  // TODO: you'll have to add some definitions here (lab1).
  // Hints: You may add interfaces like `int MaxArgs()`,
  //        and ` IntAndTable *Interp(Table *)`
    virtual int MaxArgs() const = 0;
    virtual IntAndTable *Interp(Table *) const = 0;
 protected:
  ExpKind kind;
};

class IdExp : public Exp {
 public:
  IdExp(std::string id) : Exp(ID_EXP), id(id) {}
  // TODO: you'll have to add some definitions here (lab1).
    int MaxArgs() const override;
    IntAndTable *Interp(Table *) const override;

 private:
  std::string id;
};

class NumExp : public Exp {
 public:
  NumExp(int num) : Exp(NUM_EXP), num(num) {}
  // TODO: you'll have to add some definitions here.
    int MaxArgs() const override;
    IntAndTable *Interp(Table *) const override;
 private:
  int num;
};

class OpExp : public Exp {
 public:
  OpExp(Exp *left, BinOp oper, Exp *right)
      : Exp(OP_EXP), left(left), oper(oper), right(right) {}
  // TODO: you'll have to add some definitions here (lab1).
    int MaxArgs() const override;
    IntAndTable *Interp(Table *) const override;
 private:
  Exp *left;
  BinOp oper;
  Exp *right;
};

class EseqExp : public Exp {
 public:
  EseqExp(Stm *stm, Exp *exp) : Exp(ESEQ_EXP), stm(stm), exp(exp) {}
  // TODO: you'll have to add some definitions here (lab1).
    int MaxArgs() const override;
    IntAndTable *Interp(Table *) const override;
 private:
  Stm *stm;
  Exp *exp;
};

class ExpList {
 public:
  ExpList(ExpListKind kind) : kind(kind) {}
 // TODO: you'll have to add some definitions here (lab1).
 // Hints: You may add interfaces like `int MaxArgs()`, `int NumExps()`,
 //        and ` IntAndTable *Interp(Table *)`
    virtual int MaxArgs() const = 0;
    virtual IntAndTable *Interp(Table *) const = 0;
    virtual int NumExps() const = 0;
 protected:
  ExpListKind kind;
};

class PairExpList : public ExpList {
 public:
  PairExpList(Exp *head, ExpList *tail)
      : ExpList(PAIR_EXP_LIST), head(head), tail(tail) {}
  // TODO: you'll have to add some definitions here (lab1).
    int MaxArgs() const override;
    IntAndTable *Interp(Table *) const override;
    int NumExps() const override;
 private:
  Exp *head;
  ExpList *tail;
};

class LastExpList : public ExpList {
 public:
  LastExpList(Exp *last) : ExpList(LAST_EXP_LIST), last(last) {}
  // TODO: you'll have to add some definitions here (lab1).
  int MaxArgs() const override;
  IntAndTable *Interp(Table *) const override;
  int NumExps() const override;
 private:
  Exp *last;
};

class Table {
 public:
  Table(std::string id, int value, const Table *tail)
      : id(id), value(value), tail(tail) {}
  int Lookup(std::string key) const;
  Table *Update(std::string key, int value) const;

 private:
  std::string id;
  int value;
  const Table *tail;
};

class IntAndTable {
 public:
  int i;
  Table *t;

  IntAndTable(int i, Table *t) : i(i), t(t) {}
};

}  // namespace A

#endif  // STRAIGHTLINE_SLP_H_
