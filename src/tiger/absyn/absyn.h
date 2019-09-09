#ifndef TIGER_ABSYN_ABSYN_H_
#define TIGER_ABSYN_ABSYN_H_

#include <cstdio>
#include <string>

#include "tiger/env/env.h"
#include "tiger/frame/temp.h"
#include "tiger/semant/types.h"
#include "tiger/symbol/symbol.h"
#include "tiger/translate/translate.h"

/* Forward Declarations */
namespace E {
class EnvEntry;
}  // namespace E

/* Forward Declarations */
namespace TR {
class Exp;
class Level;
class ExpAndTy;
}  // namespace TR

namespace A {

class Var;
class Exp;
class Dec;
class Ty;

class ExpList;
class FieldList;
class FunDecList;
class NameAndTyList;
class DecList;
class EFieldList;

enum Oper : unsigned int {
  PLUS_OP,
  MINUS_OP,
  TIMES_OP,
  DIVIDE_OP,
  EQ_OP,
  NEQ_OP,
  LT_OP,
  LE_OP,
  GT_OP,
  GE_OP
};

/*
 * Variables
 */

class Var {
 public:
  enum Kind { SIMPLE, FIELD, SUBSCRIPT };

  Kind kind;
  int pos;

  Var(Kind kind, int pos) : kind(kind), pos(pos) {}
  virtual void Print(FILE *out, int d) const = 0;

  // Hint: This function will be used in lab4
  virtual TY::Ty *SemAnalyze(S::Table<E::EnvEntry> *venv,
                             S::Table<TY::Ty> *tenv, int labelcount) const = 0;

  // Hint: This function will be used in lab5
  virtual TR::ExpAndTy Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const = 0;
};

class SimpleVar : public Var {
 public:
  S::Symbol *sym;

  SimpleVar(int pos, S::Symbol *sym) : Var(SIMPLE, pos), sym(sym) {}
  void Print(FILE *out, int d) const override;

  TY::Ty *SemAnalyze(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                     int labelcount) const override;

  TR::ExpAndTy Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                         TR::Level *level, TEMP::Label *label) const override;
};

class FieldVar : public Var {
 public:
  Var *var;
  S::Symbol *sym;

  FieldVar(int pos, Var *var, S::Symbol *sym)
      : Var(FIELD, pos), var(var), sym(sym) {}
  void Print(FILE *out, int d) const override;

  TY::Ty *SemAnalyze(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                     int labelcount) const override;

  TR::ExpAndTy Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                         TR::Level *level, TEMP::Label *label) const override;
};

class SubscriptVar : public Var {
 public:
  Var *var;
  Exp *subscript;

  SubscriptVar(int pos, Var *var, Exp *exp)
      : Var(SUBSCRIPT, pos), var(var), subscript(exp) {}
  void Print(FILE *out, int d) const override;

  TY::Ty *SemAnalyze(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                     int labelcount) const override;

  TR::ExpAndTy Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                         TR::Level *level, TEMP::Label *label) const override;
};

/*
 * Expressions
 */

class Exp {
 public:
  enum Kind {
    VAR,
    NIL,
    INT,
    STRING,
    CALL,
    OP,
    RECORD,
    SEQ,
    ASSIGN,
    IF,
    WHILE,
    FOR,
    BREAK,
    LET,
    ARRAY,
    VOID
  };

  Kind kind;
  int pos;

  Exp(Kind kind, int pos) : kind(kind), pos(pos) {}
  virtual void Print(FILE *out, int d) const = 0;

  // Hint: This function will be used in lab4
  virtual TY::Ty *SemAnalyze(S::Table<E::EnvEntry> *venv,
                             S::Table<TY::Ty> *tenv, int labelcount) const = 0;

  // Hint: This function will be used in lab5
  virtual TR::ExpAndTy Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const = 0;
};

class VarExp : public Exp {
 public:
  Var *var;

  VarExp(int pos, Var *var) : Exp(VAR, pos), var(var) {}
  void Print(FILE *out, int d) const override;

  TY::Ty *SemAnalyze(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                     int labelcount) const override;

  TR::ExpAndTy Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                         TR::Level *level, TEMP::Label *label) const override;
};

class NilExp : public Exp {
 public:
  NilExp(int pos) : Exp(NIL, pos) {}

  void Print(FILE *out, int d) const override;

  TY::Ty *SemAnalyze(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                     int labelcount) const override;

  TR::ExpAndTy Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                         TR::Level *level, TEMP::Label *label) const override;
};

class IntExp : public Exp {
 public:
  int i;

  IntExp(int pos, int i) : Exp(INT, pos), i(i) {}
  void Print(FILE *out, int d) const override;

  TY::Ty *SemAnalyze(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                     int labelcount) const override;

  TR::ExpAndTy Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                         TR::Level *level, TEMP::Label *label) const override;
};

class StringExp : public Exp {
 public:
  std::string s;

  StringExp(int pos, std::string *s) : Exp(STRING, pos), s(*s) {}
  void Print(FILE *out, int d) const override;

  TY::Ty *SemAnalyze(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                     int labelcount) const override;

  TR::ExpAndTy Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                         TR::Level *level, TEMP::Label *label) const override;
};

class CallExp : public Exp {
 public:
  S::Symbol *func;
  ExpList *args;

  CallExp(int pos, S::Symbol *func, ExpList *args)
      : Exp(CALL, pos), func(func), args(args) {}

  void Print(FILE *out, int d) const override;
  TY::Ty *SemAnalyze(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                     int labelcount) const override;

  TR::ExpAndTy Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                         TR::Level *level, TEMP::Label *label) const override;
};

class OpExp : public Exp {
 public:
  Oper oper;
  Exp *left, *right;

  OpExp(int pos, Oper oper, Exp *left, Exp *right)
      : Exp(OP, pos), oper(oper), left(left), right(right) {}

  void Print(FILE *out, int d) const override;
  TY::Ty *SemAnalyze(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                     int labelcount) const override;

  TR::ExpAndTy Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                         TR::Level *level, TEMP::Label *label) const override;
};

class RecordExp : public Exp {
 public:
  S::Symbol *typ;
  EFieldList *fields;

  RecordExp(int pos, S::Symbol *typ, EFieldList *fields)
      : Exp(RECORD, pos), typ(typ), fields(fields) {}
  void Print(FILE *out, int d) const override;

  TY::Ty *SemAnalyze(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                     int labelcount) const override;

  TR::ExpAndTy Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                         TR::Level *level, TEMP::Label *label) const override;
};

class SeqExp : public Exp {
 public:
  ExpList *seq;

  SeqExp(int pos, ExpList *seq) : Exp(SEQ, pos), seq(seq) {}
  void Print(FILE *out, int d) const override;

  TY::Ty *SemAnalyze(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                     int labelcount) const override;

  TR::ExpAndTy Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                         TR::Level *level, TEMP::Label *label) const override;
};

class AssignExp : public Exp {
 public:
  Var *var;
  Exp *exp;

  AssignExp(int pos, Var *var, Exp *exp)
      : Exp(ASSIGN, pos), var(var), exp(exp) {}

  void Print(FILE *out, int d) const override;
  TY::Ty *SemAnalyze(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                     int labelcount) const override;

  TR::ExpAndTy Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                         TR::Level *level, TEMP::Label *label) const override;
};

class IfExp : public Exp {
 public:
  Exp *test, *then, *elsee;

  IfExp(int pos, Exp *test, Exp *then, Exp *elsee)
      : Exp(IF, pos), test(test), then(then), elsee(elsee) {}

  void Print(FILE *out, int d) const override;

  TY::Ty *SemAnalyze(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                     int labelcount) const override;

  TR::ExpAndTy Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                         TR::Level *level, TEMP::Label *label) const override;
};

class WhileExp : public Exp {
 public:
  Exp *test, *body;

  WhileExp(int pos, Exp *test, Exp *body)
      : Exp(WHILE, pos), test(test), body(body) {}

  void Print(FILE *out, int d) const override;

  TY::Ty *SemAnalyze(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                     int labelcount) const override;

  TR::ExpAndTy Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                         TR::Level *level, TEMP::Label *label) const override;
};

class ForExp : public Exp {
 public:
  S::Symbol *var;
  Exp *lo, *hi, *body;
  bool escape;

  ForExp(int pos, S::Symbol *var, Exp *lo, Exp *hi, Exp *body)
      : Exp(FOR, pos), var(var), lo(lo), hi(hi), body(body), escape(true) {}

  void Print(FILE *out, int d) const override;

  TY::Ty *SemAnalyze(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                     int labelcount) const override;

  TR::ExpAndTy Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                         TR::Level *level, TEMP::Label *label) const override;
};

class BreakExp : public Exp {
 public:
  BreakExp(int pos) : Exp(BREAK, pos) {}
  void Print(FILE *out, int d) const override;

  TY::Ty *SemAnalyze(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                     int labelcount) const override;

  TR::ExpAndTy Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                         TR::Level *level, TEMP::Label *label) const override;
};

class LetExp : public Exp {
 public:
  DecList *decs;
  Exp *body;

  LetExp(int pos, DecList *decs, Exp *body)
      : Exp(LET, pos), decs(decs), body(body) {}
  void Print(FILE *out, int d) const override;

  TY::Ty *SemAnalyze(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                     int labelcount) const override;

  TR::ExpAndTy Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                         TR::Level *level, TEMP::Label *label) const override;
};

class ArrayExp : public Exp {
 public:
  S::Symbol *typ;
  Exp *size, *init;

  ArrayExp(int pos, S::Symbol *typ, Exp *size, Exp *init)
      : Exp(ARRAY, pos), typ(typ), size(size), init(init) {}
  void Print(FILE *out, int d) const override;

  TY::Ty *SemAnalyze(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                     int labelcount) const override;

  TR::ExpAndTy Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                         TR::Level *level, TEMP::Label *label) const override;
};

class VoidExp : public Exp {
 public:
  VoidExp(int pos) : Exp(VOID, pos) {}
  void Print(FILE *out, int d) const override;

  TY::Ty *SemAnalyze(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                     int labelcount) const override;

  TR::ExpAndTy Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                         TR::Level *level, TEMP::Label *label) const override;
};

/*
 * Declarations
 */

class Dec {
 public:
  enum Kind { FUNCTION, VAR, TYPE };

  Kind kind;
  int pos;

  Dec(Kind kind, int pos) : kind(kind), pos(pos) {}
  virtual void Print(FILE *out, int d) const = 0;

  // Hint: This function will be used in lab4
  virtual void SemAnalyze(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                          int labelcount) const = 0;

  // Hint: This function will be used in lab5
  virtual TR::Exp *Translate(S::Table<E::EnvEntry> *venv,
                             S::Table<TY::Ty> *tenv, TR::Level *level,
                             TEMP::Label *label) const = 0;
};

class FunctionDec : public Dec {
 public:
  FunDecList *functions;

  FunctionDec(int pos, FunDecList *functions)
      : Dec(FUNCTION, pos), functions(functions) {}
  void Print(FILE *out, int d) const override;

  void SemAnalyze(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                  int labelcount) const override;

  TR::Exp *Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                     TR::Level *level, TEMP::Label *label) const override;
};

class VarDec : public Dec {
 public:
  S::Symbol *var;
  S::Symbol *typ;
  Exp *init;
  bool escape;

  VarDec(int pos, S::Symbol *var, S::Symbol *typ, Exp *init)
      : Dec(VAR, pos), var(var), typ(typ), init(init), escape(true) {}
  void Print(FILE *out, int d) const override;

  void SemAnalyze(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                  int labelcount) const override;

  TR::Exp *Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                     TR::Level *level, TEMP::Label *label) const override;
};

class TypeDec : public Dec {
 public:
  NameAndTyList *types;

  TypeDec(int pos, NameAndTyList *types) : Dec(TYPE, pos), types(types) {}
  void Print(FILE *out, int d) const override;

  void SemAnalyze(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                  int labelcount) const override;

  TR::Exp *Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                     TR::Level *level, TEMP::Label *label) const override;
};

/*
 * Types
 */

class Ty {
 public:
  enum Kind { NAME, RECORD, ARRAY };

  Kind kind;
  int pos;

  Ty(Kind kind, int pos) : kind(kind), pos(pos) {}
  virtual void Print(FILE *out, int d) const = 0;

  virtual TY::Ty *SemAnalyze(S::Table<TY::Ty> *tenv) const = 0;

  virtual TY::Ty *Translate(S::Table<TY::Ty> *tenv) const = 0;
};

class NameTy : public Ty {
 public:
  S::Symbol *name;

  NameTy(int pos, S::Symbol *name) : Ty(NAME, pos), name(name) {}
  void Print(FILE *out, int d) const override;

  TY::Ty *SemAnalyze(S::Table<TY::Ty> *tenv) const override;

  TY::Ty *Translate(S::Table<TY::Ty> *tenv) const override;
};

class RecordTy : public Ty {
 public:
  FieldList *record;

  RecordTy(int pos, FieldList *record) : Ty(RECORD, pos), record(record) {}
  void Print(FILE *out, int d) const override;

  TY::Ty *SemAnalyze(S::Table<TY::Ty> *tenv) const override;

  TY::Ty *Translate(S::Table<TY::Ty> *tenv) const override;
};

class ArrayTy : public Ty {
 public:
  S::Symbol *array;

  ArrayTy(int pos, S::Symbol *array) : Ty(ARRAY, pos), array(array) {}
  void Print(FILE *out, int d) const override;

  TY::Ty *SemAnalyze(S::Table<TY::Ty> *tenv) const override;

  TY::Ty *Translate(S::Table<TY::Ty> *tenv) const override;
};

/*
 * Linked lists and nodes of lists
 */

class Field {
 public:
  int pos;
  S::Symbol *name, *typ;
  bool escape;

  Field(int pos, S::Symbol *name, S::Symbol *typ)
      : pos(pos), name(name), typ(typ), escape(true) {}

  void Print(FILE *out, int d) const;
};

class FieldList {
 public:
  Field *head;
  FieldList *tail;

  FieldList(Field *head, FieldList *tail) : head(head), tail(tail) {}

  static void Print(FILE *out, FieldList *v, int d);
  ;
};

class ExpList {
 public:
  Exp *head;
  ExpList *tail;

  ExpList(Exp *head, ExpList *tail) : head(head), tail(tail) { assert(head); }

  static void Print(FILE *out, ExpList *v, int d);
};

class FunDec {
 public:
  int pos;
  S::Symbol *name;
  FieldList *params;
  S::Symbol *result;
  Exp *body;

  FunDec(int pos, S::Symbol *name, FieldList *params, S::Symbol *result,
         Exp *body)
      : pos(pos), name(name), params(params), result(result), body(body) {}

  void Print(FILE *out, int d) const;
};

class FunDecList {
 public:
  FunDec *head;
  FunDecList *tail;

  FunDecList(FunDec *head, FunDecList *tail) : head(head), tail(tail) {}

  static void Print(FILE *out, FunDecList *v, int d);
};

class DecList {
 public:
  Dec *head;
  DecList *tail;

  DecList(Dec *head, DecList *tail) : head(head), tail(tail) {}

  static void Print(FILE *out, DecList *v, int d);
};

class NameAndTy {
 public:
  S::Symbol *name;
  Ty *ty;

  NameAndTy(S::Symbol *name, Ty *ty) : name(name), ty(ty) {}

  void Print(FILE *out, int d) const;
};

class NameAndTyList {
 public:
  NameAndTy *head;
  NameAndTyList *tail;

  NameAndTyList(NameAndTy *head, NameAndTyList *tail)
      : head(head), tail(tail) {}

  static void Print(FILE *out, NameAndTyList *v, int d);
};

class EField {
 public:
  S::Symbol *name;
  Exp *exp;

  EField(S::Symbol *name, Exp *exp) : name(name), exp(exp) {}

  static void Print(FILE *out, EField *v, int d);
};

class EFieldList {
 public:
  EField *head;
  EFieldList *tail;

  EFieldList(EField *head, EFieldList *tail) : head(head), tail(tail) {}

  static void Print(FILE *out, EFieldList *v, int d);
};

};  // namespace A

#endif  // TIGER_ABSYN_ABSYN_H_
