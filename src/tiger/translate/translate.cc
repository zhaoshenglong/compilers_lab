#include "tiger/translate/translate.h"

#include <cstdio>
#include <set>
#include <string>

#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/temp.h"
#include "tiger/semant/semant.h"
#include "tiger/semant/types.h"
#include "tiger/util/util.h"

extern EM::ErrorMsg errormsg;
namespace TR {

int static_link_offset = 8;


class Access {
 public:
  Level *level;
  F::Access *access;

  Access(Level *level, F::Access *access) : level(level), access(access) {}
  static Access *AllocLocal(Level *level, bool escape) { return nullptr; }
};

class AccessList {
 public:
  Access *head;
  AccessList *tail;

  AccessList(Access *head, AccessList *tail) : head(head), tail(tail) {}
};

class Level {
 public:
  F::Frame *frame;
  Level *parent;

  Level(F::Frame *frame, Level *parent) : frame(frame), parent(parent) {}
  AccessList *Formals(Level *level) { return nullptr; }

  static Level *NewLevel(Level *parent, TEMP::Label *name,
                         U::BoolList *formals);
};

class PatchList {
 public:
  TEMP::Label **head;
  PatchList *tail;

  PatchList(TEMP::Label **head, PatchList *tail) : head(head), tail(tail) {}
};

class Cx {
 public:
  PatchList *trues;
  PatchList *falses;
  T::Stm *stm;

  Cx(PatchList *trues, PatchList *falses, T::Stm *stm)
      : trues(trues), falses(falses), stm(stm) {}
};

class Exp {
 public:
  enum Kind { EX, NX, CX };

  Kind kind;

  Exp(Kind kind) : kind(kind) {}

  virtual T::Exp *UnEx() const = 0;
  virtual T::Stm *UnNx() const = 0;
  virtual Cx UnCx() const = 0;
};

class ExpAndTy {
 public:
  TR::Exp *exp;
  TY::Ty *ty;

  ExpAndTy(TR::Exp *exp, TY::Ty *ty) : exp(exp), ty(ty) {}
};

class ExExp : public Exp {
 public:
  T::Exp *exp;

  ExExp(T::Exp *exp) : Exp(EX), exp(exp) {}

  T::Exp *UnEx() const override {
    return exp;
  }
  T::Stm *UnNx() const override {
    return new T::ExpStm(exp);
  }
  Cx UnCx() const override {
    if (exp->kind ==  T::Exp::CONST) {
      TEMP::Label *l = TEMP::NewLabel();
      T::JumpStm *stm = new T::JumpStm(new T::NameExp(l), new TEMP::LabelList(l, NULL));
      if (((T::ConstExp *)exp)->consti == 0) {
        PatchList *falses = new PatchList(&stm->exp->name, NULL);
        return Cx(NULL, falses, new T::SeqStm(stm, new T::LabelStm(l)));
      } else if (((T::ConstExp *)exp)->consti == 1) {
        PatchList *trues = new PatchList(&stm->exp->name, NULL);
        return Cx(trues, NULL, new T::SeqStm(stm, new T::LabelStm(l)));
      }
    }
    T::CjumpStm *stm = new T::CjumpStm( 
                      T::RelOp::NE_OP, exp, new T::ConstExp(0), NULL, NULL);
    PatchList *trues = new PatchList(&stm->true_label, NULL),
              *falses = new PatchList(&stm->false_label, NULL);
    return Cx(trues, falses, stm);
  }
};

class NxExp : public Exp {
 public:
  T::Stm *stm;

  NxExp(T::Stm *stm) : Exp(NX), stm(stm) {}

  T::Exp *UnEx() const override {
    return new T::EseqExp(stm, new T::ConstExp(0));
  }
  T::Stm *UnNx() const override {
    return stm;
  }
  Cx UnCx() const override {
    printf("Nx encountered when unCx\n");
    assert(0);
  }
};

class CxExp : public Exp {
 public:
  Cx cx;

  CxExp(struct Cx cx) : Exp(CX), cx(cx) {}
  CxExp(PatchList *trues, PatchList *falses, T::Stm *stm)
      : Exp(CX), cx(trues, falses, stm) {}

  T::Exp *UnEx() const override {
    TEMP::Temp *r = TEMP::Temp::NewTemp();
    TEMP::Label *t = TEMP::NewLabel(),
                *f = TEMP::NewLabel(); 
    do_patch(cx.trues, t);
    do_patch(cx.falses, f);
    return new T::EseqExp(new T::MoveStm(new T::TempExp(r), new T::ConstExp(1)),
                  new T::EseqExp(cx.stm, new T::EseqExp(
                    new T::LabelStm(f),
                    new T::EseqExp(new T::MoveStm(
                      new T::TempExp(r), new T::ConstExp(0)), 
                      new T::EseqExp(new T::LabelStm(t), new T::TempExp(r))))));
  }
  T::Stm *UnNx() const override {
    /* True or False, jump to next stm or exp */
    TEMP::Label *t = TEMP::NewLabel(),
                *f = TEMP::NewLabel();
    do_patch(cx.trues, t);
    do_patch(cx.falses, f);
    return new T::SeqStm(cx.stm, new T::SeqStm(
              new T::LabelStm(t), new T::LabelStm(f)));
  }
  Cx UnCx() const override {
    return cx;
  }
};

void do_patch(PatchList *tList, TEMP::Label *label) {
  for (; tList; tList = tList->tail) *(tList->head) = label;
}

PatchList *join_patch(PatchList *first, PatchList *second) {
  if (!first) return second;
  for (; first->tail; first = first->tail)
    ;
  first->tail = second;
  return first;
}

Level *Outermost() {
  static Level *lv = nullptr;
  if (lv != nullptr) return lv;

  lv = new Level(nullptr, nullptr);
  return lv;
}

F::FragList *TranslateProgram(A::Exp *root) {
  // TODO: Put your codes here (lab5).
  return nullptr;
}

}  // namespace TR

namespace A {

TR::ExpAndTy SimpleVar::Translate(S::Table<E::EnvEntry> *venv,
                                  S::Table<TY::Ty> *tenv, TR::Level *level,
                                  TEMP::Label *label) const {
  TY::Ty *ty;
  T::Exp *exp;
  E::EnvEntry *entry = venv->Look(sym);
  if(!entry) {
    errormsg.Error(pos, "undefined variable %s", sym->Name().c_str());
    ty = TY::UndefinedTy::Instance();
    return TR::ExpAndTy(NULL, ty);
  }
  E::VarEntry *ve = (E::VarEntry *)entry;
  ty = ve->ty;
  
  TR::Level *lg = ve->access->level;
  TR::Level *lf = level;
  T::Exp *fp = new T::TempExp(F::FP());
  while (lf != lg)
  {
    fp = new T::MemExp(new T::BinopExp(T::BinOp::PLUS_OP, 
                       new T::ConstExp(TR::static_link_offset), fp));
    lf = lf->parent;
  }
  exp = ve->access->access->ToExp(fp);
  return TR::ExpAndTy(new TR::ExExp(exp), ty);
}

TR::ExpAndTy FieldVar::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const {
  TY::Ty *ty = NULL;
  T::Exp *exp;
  TR::ExpAndTy rec = var->Translate(venv, tenv, level, label);
  if(rec.ty->kind != TY::Ty::RECORD) {
    errormsg.Error(pos, "is not a record");
    return TR::ExpAndTy(NULL, TY::UndefinedTy::Instance());
  }
  TY::RecordTy *recTy = (TY::RecordTy *)rec.ty;
  TY::FieldList * fieldTy = recTy->fields;

  // Is the field pushed in order ?
  int off = 0;
  while (fieldTy)
  {
    if (!fieldTy->head->name->Name().compare(sym->Name())){
      ty = fieldTy->head->ty;
    }
    fieldTy = fieldTy->tail;
    off += F::wordsize;
  }
  if (!ty) {
    errormsg.Error(pos, "field %s doesn't exist", sym->Name().c_str());  
    return TR::ExpAndTy(NULL, TY::UndefinedTy::Instance());
  }
  
  exp = new T::MemExp(
          new T::BinopExp(
            T::BinOp::PLUS_OP, 
            new T::MemExp(rec.exp->UnEx()), 
            new T::ConstExp(off)));
  return TR::ExpAndTy(new TR::ExExp(exp), ty);
}

TR::ExpAndTy SubscriptVar::Translate(S::Table<E::EnvEntry> *venv,
                                     S::Table<TY::Ty> *tenv, TR::Level *level,
                                     TEMP::Label *label) const {
  TY::Ty *ty;
  T::Exp *exp;
  TR::ExpAndTy sub = subscript->Translate(venv, tenv, level, label);
  if (!sub.ty->IsSameType(TY::IntTy::Instance())) {
    errormsg.Error(pos, "array variable's subscript must be integer");
  }

  TR::ExpAndTy arr = var->Translate(venv, tenv, level, label);
  if(arr.ty->ActualTy()->kind != TY::Ty::ARRAY) {
    errormsg.Error(pos, "array type required");
  } else {
    TY::ArrayTy *arrTy = (TY::ArrayTy *)arr.ty;
    ty = arrTy->ty->ActualTy();
    exp = new T::MemExp(
            new T::BinopExp(
              T::BinOp::PLUS_OP, 
              new T::MemExp(arr.exp->UnEx()),
              new T::BinopExp(
                T::BinOp::MUL_OP, 
                new T::ConstExp(F::wordsize),
                sub.exp->UnEx())));
    return TR::ExpAndTy(new TR::ExExp(exp), ty);
  }
  return TR::ExpAndTy(NULL, NULL);
}

TR::ExpAndTy VarExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const {
  TR::ExpAndTy v = var->Translate(venv, tenv, level, label);
  return TR::ExpAndTy(v.exp, v.ty->ActualTy());
}

TR::ExpAndTy NilExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const {
  // NilExp should be ?
  return TR::ExpAndTy(NULL, TY::NilTy::Instance());
}

TR::ExpAndTy IntExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const {
  return TR::ExpAndTy(new TR::ExExp(new T::ConstExp(i)), TY::IntTy::Instance());
}

TR::ExpAndTy StringExp::Translate(S::Table<E::EnvEntry> *venv,
                                  S::Table<TY::Ty> *tenv, TR::Level *level,
                                  TEMP::Label *label) const {
  TY::Ty *ty = TY::StringTy::Instance();
  T::Exp *exp;

  TEMP::Label *l = TEMP::NewLabel();

  // where to put this fragment ?
  F::Frag *frag = new F::StringFrag(l, s);

  exp = new T::NameExp(l);
  return TR::ExpAndTy(new TR::ExExp(exp), ty);
}

TR::ExpAndTy CallExp::Translate(S::Table<E::EnvEntry> *venv,
                                S::Table<TY::Ty> *tenv, TR::Level *level,
                                TEMP::Label *label) const {
  TY::Ty *ty;
  T::Exp *exp;
  E::EnvEntry *entry = venv->Look(func);
  if (!entry){
    errormsg.Error(pos, "undefined function %s", func->Name().c_str());
    ty = TY::UndefinedTy::Instance();
    return TR::ExpAndTy(NULL, ty);
  }

  E::FunEntry *funcEntry = (E::FunEntry *)entry;
  
  TY::TyList *tyList = funcEntry->formals;
  A::ExpList *argList = args;
  while(tyList && argList) {
    TR::ExpAndTy t = argList->head->Translate(venv, tenv, level, label);
    if(!t.ty->IsSameType(tyList->head) && !t.ty->IsSameType(TY::UndefinedTy::Instance())){
      errormsg.Error(pos, "para type mismatch");
    }
    argList = argList->tail;
    tyList = tyList->tail;
  } 
  if(tyList) {
    // errormsg.Error(pos, "missing params in function %s", func->Name().c_str());
  }
  if(argList) {
    errormsg.Error(pos, "too many params in function %s", func->Name().c_str());
  }
  
  T::Exp *sl = new T::TempExp(F::FP());

  while (level != funcEntry->level->parent) 
  {
    sl = new T::MemExp(
          new T::BinopExp(
            T::BinOp::PLUS_OP,
            sl, 
            new T::ConstExp(TR::static_link_offset)));
    level = level->parent;
  }

  exp = new T::CallExp(
          new T::NameExp(funcEntry->label), 
          new T::ExpList(sl, T::make_actual_list(argList)));
  if(funcEntry->result) {
    ty = funcEntry->result;
  } else {
    ty = TY::VoidTy::Instance();
  }
  return TR::ExpAndTy(new TR::ExExp(exp), ty);
}

TR::ExpAndTy OpExp::Translate(S::Table<E::EnvEntry> *venv,
                              S::Table<TY::Ty> *tenv, TR::Level *level,
                              TEMP::Label *label) const {
  TR::ExpAndTy lexpTy = left->Translate(venv, tenv, level, label);
  TR::ExpAndTy rexpTy = right->Translate(venv, tenv, level, label);
  
  // Tree language binary operator is not corresponding to that in absyn
  switch (oper)
  {
  case T::BinOp::PLUS_OP:
  case T::BinOp::MINUS_OP:
  case T::BinOp::MUL_OP:
  case T::BinOp::DIV_OP:
    if (!lexpTy.ty->IsSameType(TY::IntTy::Instance()) 
        || !rexpTy.ty->IsSameType(TY::IntTy::Instance())) {
      if (!lexpTy.ty->IsSameType(TY::UndefinedTy::Instance()) 
          && !rexpTy.ty->IsSameType(TY::UndefinedTy::Instance())) {
        errormsg.Error(pos, "integer required");
        return TR::ExpAndTy(NULL, TY::IntTy::Instance());
      }
    } else {

    }
    break;
  case T::RelOp::EQ_OP:
  case T::RelOp::NE_OP:
  case T::RelOp::LT_OP:
  case T::RelOp::LE_OP:
  case T::RelOp::GT_OP:
  case T::RelOp::GE_OP:
    if (!lexpTy.ty->IsSameType(rexpTy.ty)) {
      errormsg.Error(pos, "same type required");
      return TR::ExpAndTy(NULL, TY::IntTy::Instance());
    } else {
      T::CjumpStm *stm = new T::CjumpStm(T::RelOp::GE_OP, lexpTy.exp->UnEx(), rexpTy.exp->UnEx(), NULL, NULL);
      TR::PatchList *trues = new TR::PatchList(&stm->true_label, NULL),
                    *falses = new TR::PatchList(&stm->false_label, NULL);
      return TR::ExpAndTy(
              new TR::CxExp(TR::Cx(trues, falses, stm)),
              TY::IntTy::Instance());
    }
    break;
  default:
    // should never go here;
    assert(0);
  }
}

TR::ExpAndTy RecordExp::Translate(S::Table<E::EnvEntry> *venv,
                                  S::Table<TY::Ty> *tenv, TR::Level *level,
                                  TEMP::Label *label) const {
  TY::Ty *ty = tenv->Look(typ);
  T::Exp *exp;
  if (!ty) {
    errormsg.Error(pos, "undefined type %s", typ->Name().c_str());
    return TR::ExpAndTy(NULL, TY::UndefinedTy::Instance());
  }

  T::Exp *exCall = F::externalCall("initArray", )

  return TR::ExpAndTy(nullptr, ty);
}

TR::ExpAndTy SeqExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const {
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy AssignExp::Translate(S::Table<E::EnvEntry> *venv,
                                  S::Table<TY::Ty> *tenv, TR::Level *level,
                                  TEMP::Label *label) const {
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy IfExp::Translate(S::Table<E::EnvEntry> *venv,
                              S::Table<TY::Ty> *tenv, TR::Level *level,
                              TEMP::Label *label) const {
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy WhileExp::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const {
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy ForExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const {
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy BreakExp::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const {
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy LetExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const {
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy ArrayExp::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const {
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy VoidExp::Translate(S::Table<E::EnvEntry> *venv,
                                S::Table<TY::Ty> *tenv, TR::Level *level,
                                TEMP::Label *label) const {
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::Exp *FunctionDec::Translate(S::Table<E::EnvEntry> *venv,
                                S::Table<TY::Ty> *tenv, TR::Level *level,
                                TEMP::Label *label) const {
  // TODO: Put your codes here (lab5).
  return nullptr;
}

TR::Exp *VarDec::Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                           TR::Level *level, TEMP::Label *label) const {
  // TODO: Put your codes here (lab5).
  return nullptr;
}

TR::Exp *TypeDec::Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                            TR::Level *level, TEMP::Label *label) const {
  // TODO: Put your codes here (lab5).
  return nullptr;
}

TY::Ty *NameTy::Translate(S::Table<TY::Ty> *tenv) const {
  // TODO: Put your codes here (lab5).
  return TY::VoidTy::Instance();
}

TY::Ty *RecordTy::Translate(S::Table<TY::Ty> *tenv) const {
  // TODO: Put your codes here (lab5).
  return TY::VoidTy::Instance();
}

TY::Ty *ArrayTy::Translate(S::Table<TY::Ty> *tenv) const {
  // TODO: Put your codes here (lab5).
  return TY::VoidTy::Instance();
}

}  // namespace A
