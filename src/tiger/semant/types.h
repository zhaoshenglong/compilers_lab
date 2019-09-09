#ifndef TIGER_SEMANT_TYPES_H_
#define TIGER_SEMANT_TYPES_H_

#include "tiger/symbol/symbol.h"

namespace TY {

class TyList;
class Field;
class FieldList;

class Ty {
 public:
  enum Kind { RECORD, NIL, INT, STRING, ARRAY, NAME, VOID };

  Kind kind;

  Ty *ActualTy();
  bool IsSameType(Ty *);

 protected:
  Ty(Kind kind) : kind(kind) {}
};

class NilTy : public Ty {
 public:
  static NilTy *Instance() { return &nilty_; }

 private:
  static NilTy nilty_;

  NilTy() : Ty(NIL) {}
};

class IntTy : public Ty {
 public:
  static IntTy *Instance() { return &intty_; }

 private:
  static IntTy intty_;

  IntTy() : Ty(INT) {}
};

class StringTy : public Ty {
 public:
  static StringTy *Instance() { return &stringty_; }

 private:
  static StringTy stringty_;

  StringTy() : Ty(STRING) {}
};

class VoidTy : public Ty {
 public:
  static VoidTy *Instance() { return &voidty_; }

 private:
  static VoidTy voidty_;

  VoidTy() : Ty(VOID) {}
};

class RecordTy : public Ty {
 public:
  FieldList *fields;

  RecordTy(FieldList *fields) : Ty(RECORD), fields(fields) {}
};

class ArrayTy : public Ty {
 public:
  Ty *ty;

  ArrayTy(Ty *ty) : Ty(ARRAY), ty(ty) {}
};

class NameTy : public Ty {
 public:
  S::Symbol *sym;
  Ty *ty;

  NameTy(S::Symbol *sym, Ty *ty) : Ty(NAME), sym(sym), ty(ty) {}
};

class TyList {
 public:
  Ty *head;
  TyList *tail;

  TyList(Ty *head, TyList *tail) : head(head), tail(tail) {}
};

class Field {
 public:
  S::Symbol *name;
  Ty *ty;

  Field(S::Symbol *name, Ty *ty) : name(name), ty(ty) {}
};

class FieldList {
 public:
  Field *head;
  FieldList *tail;

  FieldList(Field *head, FieldList *tail) : head(head), tail(tail) {}
};

}  // namespace TY

#endif  // TIGER_SEMANT_TYPES_H_
