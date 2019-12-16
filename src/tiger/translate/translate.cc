#include "tiger/translate/translate.h"

#include <cstdio>
#include <set>
#include <string>

#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/temp.h"
#include "tiger/semant/semant.h"
#include "tiger/semant/types.h"
#include "tiger/util/util.h"

using VEnvType = S::Table<E::EnvEntry> *;
using TEnvType = S::Table<TY::Ty> *;

namespace
{
extern EM::ErrorMsg errormsg;
F::FragList* globalFragList = NULL;

void AddToGlobalFragList(F::Frag *frag) {
  if (!globalFragList) {
    globalFragList = new F::FragList(frag, NULL);
  } else {
    F::FragList *fl = globalFragList;
    for ( ; fl->tail; fl = fl->tail);
    fl->tail = new F::FragList(frag, NULL);
  }
}

static TY::TyList *make_formal_tylist(S::Table<TY::Ty> * tenv, A::FieldList *params) {
  if (params == nullptr) {
    return nullptr;
  }

  TY::Ty *ty = tenv->Look(params->head->typ);
  if (ty == nullptr) {
    errormsg.Error(params->head->pos, "undefined type %s",
                   params->head->typ->Name().c_str());
  }

  return new TY::TyList(ty->ActualTy(), make_formal_tylist(tenv, params->tail));
}

static U::BoolList *make_formal_bool(A::FieldList *params) {
  if (!params) {
    return NULL;
  } else {
    return new U::BoolList(true, make_formal_bool(params->tail));
  }
}

TR::Exp *INVALID_EXP() {
  return new TR::ExExp(new T::ConstExp(0));
}


} // namespace

namespace TR {

int static_link_offset = 8;

static T::ExpList *make_actual_list(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label, A::ExpList *formals) {
  if (!formals) {
    return NULL;
  }
  return new T::ExpList(formals->head->Translate(venv, tenv, level, label).exp->UnEx(), 
                        make_actual_list(venv, tenv, level, label, formals->tail));
}

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
  AccessList *Formals() { 
    F::AccessList *al = frame->getFormals();
    TR::AccessList *fm = NULL;
    TR::AccessList *fmPtr = fm;
    if (al) {
      TR::Access *acc = new TR::Access::Access(this, al->head);
      TR::AccessList *fm = new TR::AccessList(acc, NULL);
      TR::AccessList *fmPtr = fm;
      al=al->tail;
    } else {
      // must exist static link
      assert(0);
    }
    while (al){
      TR::Access *acc = new TR::Access(this, al->head);
      fmPtr->tail = new TR::AccessList(acc, NULL);
      fmPtr = fmPtr->tail;
      al = al->tail;  
    }
    return fm;
  }

  static Level *NewLevel(Level *parent, TEMP::Label *name,
                         U::BoolList *formals) {
    F::Frame *frame = F::newFrame(name, formals);
    Level *level = new TR::Level(frame, parent);
    return level;
  }
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
    // TODO: Rewrite this function
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
  TR::Level *outerlev = Outermost();
  TEMP::Label *outerLabel = TEMP::NamedLabel("tigermain");
  TR::ExpAndTy expTy = root->Translate(E::BaseVEnv(), E::BaseTEnv(), outerlev, outerLabel);
  T::Stm* stm = F::procEntryExit1(
                  outerlev->frame, 
                  new T::MoveStm(new T::TempExp(F::RV()), expTy.exp->UnEx()));
  F::ProcFrag *procFrag = F::NewProcFrag(stm, outerlev->frame);
  AddToGlobalFragList(procFrag);
  return globalFragList;
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
  // TODO: Check NilExp Translation
  return TR::ExpAndTy(new TR::ExExp(new T::ConstExp(0)), TY::NilTy::Instance());
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

  TEMP::Label *lab = TEMP::NewLabel();
  F::StringFrag *frag = new F::StringFrag(lab, s);

  AddToGlobalFragList(frag);
  return TR::ExpAndTy(
          new TR::ExExp(new T::NameExp(lab)), ty);
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
    errormsg.Error(pos, "missing params in function %s", func->Name().c_str());
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
          new T::ExpList(sl, make_actual_list(venv, tenv, level, label, argList)));
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

  TEMP::Temp *r = TEMP::Temp::NewTemp();
  int sz = 0, off = 0;

  T::SeqStm *seqPtr = NULL;
  T::SeqStm *seqStm = seqPtr;
  A::EFieldList *f = fields;

  bool initial = false;
  while (f)
  {
    if (!f) {
      break;
    } else {
      if (!initial) {
        seqStm = new T::SeqStm(
                    new T::MoveStm(
                      new T::MemExp(
                        new T::BinopExp(
                          T::BinOp::PLUS_OP, 
                          new T::TempExp(r), 
                          new T::ConstExp(off * F::wordsize))), 
                        f->head->exp->Translate(venv, tenv, level, label).exp->UnEx()), NULL);
        seqPtr = seqStm;
        initial = true;
      } else {
        T::SeqStm *stm = new T::SeqStm(
                          new T::MoveStm(
                            new T::MemExp(
                              new T::BinopExp(
                                T::BinOp::PLUS_OP, 
                                new T::TempExp(r), 
                                new T::ConstExp(off * F::wordsize))), 
                              f->head->exp->Translate(venv, tenv, level, label).exp->UnEx()), NULL);
        seqPtr->right = stm;
        seqPtr = stm;
      }
      off++;
      f = f->tail;
    }
  }
  sz = off;
  
  T::ExpList *exArgs = new T::ExpList(new T::ConstExp(sz), NULL);
  T::Exp *exCall = F::externalCall("allocRecord", exArgs);
  return TR::ExpAndTy(
          new TR::ExExp(
            new T::EseqExp(
              new T::SeqStm(
                new T::MoveStm(
                  new T::TempExp(r), 
                  exCall), seqStm), 
          new T::TempExp(r))), ty);
}

TR::ExpAndTy SeqExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const {
  TY::Ty *ty;
  T::SeqStm *seqPtr = NULL;
  T::SeqStm *seqStm = seqPtr;
  A::ExpList *expList = seq;
  bool initial = false;
  while (expList)
  {
    TR::ExpAndTy expTy = expList->head->Translate(venv, tenv, level, label);
    if(!expList->tail) {
      ty = expTy.ty;
      return TR::ExpAndTy(
              new TR::ExExp(
                new T::EseqExp(
                  seqStm, 
                  expTy.exp->UnEx())), 
              ty);
    } else {
      expList = expList->tail;
      if (!initial) {
        initial = true;
        seqPtr = new T::SeqStm(expTy.exp->UnNx(), NULL);
        seqStm = seqPtr;
      } else {
        T::SeqStm *stm = new T::SeqStm(expTy.exp->UnNx(), NULL);
        seqPtr->right = stm;
        seqPtr = stm;
      }
    }
  }
  // should not go here
  assert(0);
}

TR::ExpAndTy AssignExp::Translate(S::Table<E::EnvEntry> *venv,
                                  S::Table<TY::Ty> *tenv, TR::Level *level,
                                  TEMP::Label *label) const {
  T::Stm *stm;
  
  TR::ExpAndTy varExpTy = var->Translate(venv, tenv, level, label);
  TR::ExpAndTy expExpTy = exp->Translate(venv, tenv, level, label);

  if (varExpTy.ty->IsSameType(TY::UndefinedTy::Instance())) {
    return TR::ExpAndTy(NULL, TY::UndefinedTy::Instance());
  }
  if(!expExpTy.ty->IsSameType(varExpTy.ty)) {
    errormsg.Error(pos, "unmatched assign exp");
    return TR::ExpAndTy(NULL, TY::VoidTy::Instance());
  }
  if(varExpTy.ty->kind == A::Var::SIMPLE) {
    A::SimpleVar *sv = (A::SimpleVar*)var;
    E::VarEntry *ve = (E::VarEntry*)venv->Look(sv->sym);
    if(ve->readonly){
      errormsg.Error(pos, "loop variable can't be assigned");
      return TR::ExpAndTy(NULL, TY::VoidTy::Instance());
    }
  }
  stm = new T::MoveStm(varExpTy.exp->UnEx(), expExpTy.exp->UnEx());
  return TR::ExpAndTy(new TR::NxExp(stm), TY::VoidTy::Instance());
}

TR::ExpAndTy IfExp::Translate(S::Table<E::EnvEntry> *venv,
                              S::Table<TY::Ty> *tenv, TR::Level *level,
                              TEMP::Label *label) const {
  TR::ExpAndTy e1ExpTy = test->Translate(venv, tenv, level, label),
               e2ExpTy = then->Translate(venv, tenv, level, label);
  
  if (!e1ExpTy.ty || !e1ExpTy.ty->IsSameType(TY::IntTy::Instance())) {
    errormsg.Error(pos, "if test must be integer");
    return TR::ExpAndTy(NULL, TY::VoidTy::Instance());
  }
  
  if (!elsee) {
    if (!e2ExpTy.ty || !e2ExpTy.ty->IsSameType(TY::VoidTy::Instance())) {
      errormsg.Error(pos, "if-then exp's body must produce no value");
      return TR::ExpAndTy(NULL, TY::VoidTy::Instance());
    }
    TEMP::Label *t = TEMP::NewLabel(),
                *f = TEMP::NewLabel();
    TR::Cx cx = e1ExpTy.exp->UnCx();
    TR::do_patch(cx.trues, t);
    TR::do_patch(cx.falses, f);
    T::Stm *stm = new T::SeqStm(
                    cx.stm, 
                    new T::SeqStm(
                      new T::LabelStm(t), 
                      new T::SeqStm(
                        e2ExpTy.exp->UnNx(), 
                        new T::LabelStm(f))));
    return TR::ExpAndTy(new TR::NxExp(stm), TY::VoidTy::Instance());
  } else {
    TR::ExpAndTy e3ExpTY = elsee->Translate(venv, tenv, level, label);
    if(!e2ExpTy.ty->IsSameType(e3ExpTY.ty)){
      errormsg.Error(pos, "then exp and else exp type mismatch");
    }
    // TODO: Case e2 & e3 is not ExExp()
    TEMP::Label *t = TEMP::NewLabel(),
                *f = TEMP::NewLabel();
    TR::Cx cx = e1ExpTy.exp->UnCx();
    TR::do_patch(cx.trues, t);
    TR::do_patch(cx.falses, f);
    TEMP::Temp *r = TEMP::Temp::NewTemp();
    T::Exp *exp = new T::EseqExp(
                    new T::SeqStm(
                      cx.stm, 
                      new T::SeqStm(
                        new T::LabelStm(t), 
                        new T::SeqStm(
                          new T::MoveStm(
                            new T::TempExp(r), 
                            e2ExpTy.exp->UnEx()), 
                            new T::SeqStm(
                              new T::LabelStm(f), 
                              new T::SeqStm(
                                new T::MoveStm(
                                  new T::TempExp(r), 
                                  e3ExpTY.exp->UnEx()), 
                                  NULL))))),
                    new T::TempExp(r));
    return TR::ExpAndTy(new TR::ExExp(exp), e2ExpTy.ty); 
  }
}

TR::ExpAndTy WhileExp::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const {
  T::Stm *stm;
  TR::ExpAndTy testExpTy = test->Translate(venv, tenv, level, label);
  if (!testExpTy.ty || !testExpTy.ty->IsSameType(TY::IntTy::Instance())) {
    errormsg.Error(pos, "while test must be integer");
    return TR::ExpAndTy(NULL, TY::VoidTy::Instance());
  }
  TEMP::Label *doneLabel = TEMP::NewLabel();
  TR::ExpAndTy bodyExpTy = body->Translate(venv, tenv, level, doneLabel);
  if (!bodyExpTy.ty || !bodyExpTy.ty->IsSameType(TY::VoidTy::Instance())) {
    errormsg.Error(pos, "while body must produce no value");
    return TR::ExpAndTy(NULL, TY::VoidTy::Instance());
  }
  TEMP::Label *testLabel = TEMP::NewLabel();
  TEMP::Label *zLabel = TEMP::NewLabel();
  TR::Cx cx = testExpTy.exp->UnCx();
  TR::do_patch(cx.trues, zLabel);
  TR::do_patch(cx.falses, doneLabel);
  stm = new T::SeqStm(
          new T::LabelStm(testLabel),
            new T::SeqStm(
              cx.stm,
                new T::SeqStm(
                  new T::LabelStm(zLabel), 
                  new T::SeqStm(
                    bodyExpTy.exp->UnNx(), 
                    new T::SeqStm(
                      new T::JumpStm(
                        new T::NameExp(testLabel), 
                        new TEMP::LabelList(testLabel, NULL)),
                        new T::LabelStm(doneLabel))))));
  return TR::ExpAndTy(new TR::NxExp(stm), TY::VoidTy::Instance());
}

TR::ExpAndTy ForExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const {
  TR::ExpAndTy loExpTy = lo->Translate(venv, tenv, level, label);
  TR::ExpAndTy hiExpTy = hi->Translate(venv, tenv, level, label);
  if (!loExpTy.ty || !loExpTy.ty->IsSameType(TY::IntTy::Instance())) {
    errormsg.Error(pos, "for exp's range type is not integer");
    return TR::ExpAndTy(NULL, TY::VoidTy::Instance());
  }
  if (!hiExpTy.ty || !hiExpTy.ty->IsSameType(TY::IntTy::Instance())) {
    errormsg.Error(pos, "for exp's range type is not integer");
    return TR::ExpAndTy(NULL, TY::VoidTy::Instance());
  }

  venv->BeginScope();
  TR::Access* acc = TR::Access::AllocLocal(level, true);
  venv->Enter(var, new E::VarEntry(acc, TY::IntTy::Instance(), true));
  TR::ExpAndTy bodyExpTy = body->Translate(venv, tenv, level, label);
  if(!bodyExpTy.ty->IsSameType(TY::VoidTy::Instance())) {
    errormsg.Error(pos, "for expression must produce no value");
    return TR::ExpAndTy(NULL, TY::VoidTy::Instance());
  }
  venv->EndScope();
  TEMP::Label *zLabel = TEMP::NewLabel(),
              *doneLabel = TEMP::NewLabel(),
              *testLabel = TEMP::NewLabel();
  T::CjumpStm *conStm = new T::CjumpStm(
                      T::RelOp::LE_OP, 
                      acc->access->ToExp(new T::TempExp(F::FP())), 
                      hiExpTy.exp->UnEx(), 
                      zLabel, 
                      doneLabel);
  T::SeqStm *bodyStm = new T::SeqStm(
                        bodyExpTy.exp->UnNx(), 
                        new T::SeqStm(
                          new T::MoveStm(
                            acc->access->ToExp(new T::TempExp(F::FP())), 
                            new T::BinopExp(
                              T::BinOp::PLUS_OP, 
                              acc->access->ToExp(
                                new T::TempExp(F::FP())), 
                                new T::ConstExp(1))), NULL));
  return TR::ExpAndTy(
          new TR::NxExp(
            new T::SeqStm(
              new T::LabelStm(testLabel), 
              new T::SeqStm(
                conStm, 
                new T::SeqStm(
                  new T::LabelStm(zLabel), 
                  new T::SeqStm(bodyStm, 
                    new T::SeqStm(
                      new T::JumpStm(
                        new T::NameExp(testLabel), 
                        new TEMP::LabelList(testLabel, NULL)), 
                      new T::SeqStm(
                        new T::LabelStm(doneLabel), NULL))))))), TY::VoidTy::Instance());
}

TR::ExpAndTy BreakExp::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const {
  if (!label) {
    errormsg.Error(pos, "break is not in one loop");
    return TR::ExpAndTy(NULL, TY::VoidTy::Instance());
  }
  T::Stm * stm = new T::JumpStm(
                  new T::NameExp(label), 
                  new TEMP::LabelList(label, NULL));
  return TR::ExpAndTy(new TR::NxExp(stm), TY::VoidTy::Instance());
}

TR::ExpAndTy LetExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const {
  venv->BeginScope();
  tenv->BeginScope();

  T::SeqStm *decStm = NULL;
  T::SeqStm *stmPtr = decStm;
  TR::Exp *decExp;
  A::DecList *decList = decs;

  bool initial = false;

  while (decList)
  {
    decExp = decList->head->Translate(venv, tenv, level, label);
    
    if (!initial) {
      decStm = new T::SeqStm(decExp->UnNx(), NULL);
      stmPtr = decStm;
      initial = true;
    } else {
      T::SeqStm *stm = new T::SeqStm(decExp->UnNx(), NULL);
      stmPtr->right = stm;
      stmPtr = stm;
    }
    decList = decList->tail;
  }
  TR::ExpAndTy bodyExpTy = body->Translate(venv, tenv, level, label);
  tenv->EndScope();
  venv->EndScope();
  return TR::ExpAndTy(
          new TR::ExExp(
            new T::EseqExp(
              decStm, 
              bodyExpTy.exp->UnEx())), 
          bodyExpTy.ty);
}

TR::ExpAndTy ArrayExp::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const {
  TY::Ty *ty;
  T::Exp *exp;

  TY::Ty *arrTy = tenv->Look(typ);
  if(!arrTy) {
    errormsg.Error(pos, "undefiend type %s", typ->Name().c_str());
    return TR::ExpAndTy(NULL, TY::UndefinedTy::Instance());
  }
  if(arrTy->ActualTy()->kind != TY::Ty::ARRAY) {
    errormsg.Error(pos, "not array type");
    return TR::ExpAndTy(NULL, TY::VoidTy::Instance());
  }

  ty = (TY::ArrayTy *)arrTy;

  TR::ExpAndTy sizeExpTy = size->Translate(venv, tenv, level, label);
  if (!sizeExpTy.ty->IsSameType(TY::IntTy::Instance())) {
    errormsg.Error(pos, "integer required");
    return TR::ExpAndTy(NULL, TY::VoidTy::Instance());
  }
  TR::ExpAndTy initExpTy = init->Translate(venv, tenv, level, label);
  if(!initExpTy.ty->IsSameType(ty)){
    errormsg.Error(pos, "type mismatch");
    return TR::ExpAndTy(NULL, TY::VoidTy::Instance());
  }
  TEMP::Temp *r = TEMP::Temp::NewTemp();

  T::Exp *initArrExp = new T::EseqExp(
                        new T::MoveStm(
                          new T::TempExp(r), 
                          F::externalCall("initArray", 
                            new T::ExpList(
                              sizeExpTy.exp->UnEx(), 
                              new T::ExpList(
                                initExpTy.exp->UnEx(), NULL)))), 
                        new T::TempExp(r));

  return TR::ExpAndTy(new TR::ExExp(exp), ty);
}

TR::ExpAndTy VoidExp::Translate(S::Table<E::EnvEntry> *venv,
                                S::Table<TY::Ty> *tenv, TR::Level *level,
                                TEMP::Label *label) const {
  return TR::ExpAndTy(
          new TR::NxExp(
            new T::ExpStm(
              new T::ConstExp(0))), TY::VoidTy::Instance());
}

TR::Exp *FunctionDec::Translate(S::Table<E::EnvEntry> *venv,
                                S::Table<TY::Ty> *tenv, TR::Level *level,
                                TEMP::Label *label) const {
  A::FunDecList *funList = functions;
  while (funList) {
    A::FunDec *funDec = funList->head;
    TY::Ty *resultTy = TY::VoidTy::Instance();
    if (funDec->result) {
      resultTy = tenv->Look(funDec->result);
      if (!resultTy) {
        return INVALID_EXP();
      }
    }
    TY::TyList *formalTyList = make_formal_tylist(tenv, funDec->params);
    if (venv->Look(funDec->name)) {
      // TODO: Is this valid 
      errormsg.Error(pos, "two functions have the same name");
      return INVALID_EXP();
    }
    else {
      TEMP::Label *lab = TEMP::NewLabel();
      U::BoolList *formalBools = make_formal_bool(funDec->params);
      TR::Level *l = TR::Level::NewLevel(level, lab, formalBools);
      venv->Enter(funDec->name, new E::FunEntry(l, lab, formalTyList, resultTy));
    }
    funList = funList->tail;
  }

  // second pass through, check function body
  funList = functions;
  while (funList) {
    venv->BeginScope();
    A::FunDec *funDec = funList->head;
    A::FieldList *actuals = funDec->params;
    E::FunEntry *funEntry = (E::FunEntry *)venv->Look(funDec->name);
    TR::AccessList *formalsAcc = funEntry->level->Formals();
    TY::TyList *formalsTy = funEntry->formals;

    while (actuals) {
      // enter formal paremeters
      venv->Enter(actuals->head->name, new E::VarEntry(
                                        formalsAcc->head, 
                                        formalsTy->head, 
                                        false));
      formalsAcc = formalsAcc->tail;
      formalsTy = formalsTy->tail;
      actuals = actuals->tail;
    }
    TR::ExpAndTy bodyExpTy = funDec->body->Translate(venv, tenv, level, label);
    if(!bodyExpTy.ty->IsSameType(funEntry->result)) {
      if(funEntry->result->IsSameType(TY::VoidTy::Instance())) {
        errormsg.Error(pos, "procedure returns value");
        return new TR::ExExp(new T::ConstExp(0));
      } else {
        errormsg.Error(pos, "function return type mismatch");
        return new TR::ExExp(new T::ConstExp(0));
      }
    }
    // add fragments
    T::Stm *stm = new T::MoveStm(new T::TempExp(F::RV()), bodyExpTy.exp->UnEx());
    T::Stm * procStm = F::procEntryExit1(funEntry->level->frame, stm);
    F::ProcFrag *procFrag = F::NewProcFrag(procStm, funEntry->level->frame);
    AddToGlobalFragList(procFrag);

    venv->EndScope();
    funList = funList->tail;
  }
  return new TR::ExExp(new T::ConstExp(0));
}

TR::Exp *VarDec::Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                           TR::Level *level, TEMP::Label *label) const {
  TR::ExpAndTy initExpTy = init->Translate(venv, tenv, level, label);
  TR::Access *acc;

  if (typ){
    TY::Ty *varTy = tenv->Look(typ);
    if(!varTy) {
      errormsg.Error(pos, "undefined type of %s", typ->Name().c_str());
      return INVALID_EXP();
    }
    if(!varTy->IsSameType(initExpTy.ty) ){
      errormsg.Error(pos, "type mismatch");
      return INVALID_EXP();
    }

    acc = TR::Access::AllocLocal(level, true);
    venv->Enter(var, new E::VarEntry(acc, varTy, false));
  }
  else {
    if(initExpTy.ty->IsSameType(TY::NilTy::Instance()) && initExpTy.ty->ActualTy()->kind != TY::Ty::RECORD) {
      errormsg.Error(pos, "init should not be nil without type specified");
      return INVALID_EXP();
    } else {

      acc = TR::Access::AllocLocal(level, true);
      venv->Enter(var, new E::VarEntry(acc, initExpTy.ty, false));
    }
  }
  T::Stm *stm = new T::MoveStm(
                  acc->access->ToExp(new T::TempExp(F::FP())), 
                  initExpTy.exp->UnEx());
  return new TR::NxExp(stm);
}

TR::Exp *TypeDec::Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                            TR::Level *level, TEMP::Label *label) const {
  // Copy from lab4
  // CHANGE: tyList->head->ty->Semanlyse  ->  tyList->head->ty->TRanslate
  NameAndTyList *tyList = types;
  while (tyList)
  {
    if(tenv->Look(tyList->head->name)) {
      errormsg.Error(pos, "two types have the same name");
    } else {
      tenv->Enter(tyList->head->name, new TY::NameTy(tyList->head->name, nullptr));
    }
    tyList = tyList->tail;
  }

  tyList = types;
  while(tyList) {
    TY::Ty *ty = tenv->Look(tyList->head->name);
    ((TY::NameTy *)ty)->ty = tyList->head->ty->Translate(tenv);
    tyList = tyList->tail;
  }
  tyList = types;
  bool isOk = true;
  while(tyList && isOk) {
    TY::Ty *ty = tenv->Look(tyList->head->name);
    if(ty->kind == TY::Ty::NAME) {
      TY::Ty *tyTy = ((TY::NameTy *)ty)->ty;
      while (tyTy->kind == TY::Ty::NAME)
      {
        TY::NameTy *nameTy = (TY::NameTy *)tyTy;
        if(!nameTy->sym->Name().compare(tyList->head->name->Name())){
          errormsg.Error(pos, "illegal type cycle");
          isOk = false;
          break;
        }
        tyTy = nameTy->ty;
      }
    }
    
    tyList = tyList->tail;
  }
  return new TR::ExExp(new T::ConstExp(0));
}

TY::Ty *NameTy::Translate(S::Table<TY::Ty> *tenv) const {
  return this->SemAnalyze(tenv);
}

TY::Ty *RecordTy::Translate(S::Table<TY::Ty> *tenv) const {
  return this->SemAnalyze(tenv);
}

TY::Ty *ArrayTy::Translate(S::Table<TY::Ty> *tenv) const {
  return this->SemAnalyze(tenv);
}

}  // namespace A
