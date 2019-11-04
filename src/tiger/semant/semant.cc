#include "tiger/semant/semant.h"
#include "tiger/errormsg/errormsg.h"

extern EM::ErrorMsg errormsg;

using VEnvType = S::Table<E::EnvEntry> *;
using TEnvType = S::Table<TY::Ty> *;

namespace {
static TY::TyList *make_formal_tylist(TEnvType tenv, A::FieldList *params) {
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

static TY::FieldList *make_fieldlist(TEnvType tenv, A::FieldList *fields) {
  if (fields == nullptr) {
    return nullptr;
  }

  TY::Ty *ty = tenv->Look(fields->head->typ);
  if (ty == nullptr) {
    errormsg.Error(fields->head->pos, "undefined type %s",
                   fields->head->typ->Name().c_str());
  }
  return new TY::FieldList(new TY::Field(fields->head->name, ty),
                           make_fieldlist(tenv, fields->tail));
}

}  // namespace

namespace A {

TY::Ty *SimpleVar::SemAnalyze(VEnvType venv, TEnvType tenv,
                              int labelcount) const {
  // TODO: Put your codes here (lab4).
  E::EnvEntry *envEntry = venv->Look(sym);
  if(!envEntry) {
    errormsg.Error(pos, "undefined variable %s", sym->Name().c_str());
    return TY::VoidTy::Instance();
  }
  return ((E::VarEntry *)envEntry)->ty;
}

TY::Ty *FieldVar::SemAnalyze(VEnvType venv, TEnvType tenv,
                             int labelcount) const {
  // TODO: Put your codes here (lab4).
  TY::Ty *ty = var->SemAnalyze(venv, tenv, labelcount)->ActualTy();
  if(ty->kind != TY::Ty::RECORD) {
    errormsg.Error(pos, "not a record type");
    return TY::VoidTy::Instance();
  }
  TY::RecordTy *recTy = (TY::RecordTy *)ty;
  TY::FieldList * fieldTy = recTy->fields;

  while (fieldTy)
  {
    if (!fieldTy->head->name->Name().compare(sym->Name()))
    {
      return fieldTy->head->ty;
    }
    fieldTy = fieldTy->tail;
  }
  errormsg.Error(pos, "field %s doesn't exist", sym->Name().c_str());  
  return TY::VoidTy::Instance();
}

TY::Ty *SubscriptVar::SemAnalyze(VEnvType venv, TEnvType tenv,
                                 int labelcount) const {
  // TODO: Put your codes here (lab4).
  TY::Ty *subscriptTy = subscript->SemAnalyze(venv, tenv, labelcount);
  if (!subscriptTy->IsSameType(TY::IntTy::Instance())) {
    errormsg.Error(pos, "array variable's subscript must be integer");
  }

  TY::Ty *arrayTy = var->SemAnalyze(venv, tenv, labelcount);
  if(arrayTy->kind != TY::Ty::ARRAY) {
    errormsg.Error(pos, "array type required");
  }
  return arrayTy->ActualTy();
}

TY::Ty *VarExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  return var->SemAnalyze(venv, tenv, labelcount)->ActualTy();
}

TY::Ty *NilExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  return TY::NilTy::Instance();
}

TY::Ty *IntExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  return TY::IntTy::Instance();
}

TY::Ty *StringExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                              int labelcount) const {
  // TODO: Put your codes here (lab4).
  return TY::StringTy::Instance();
}

TY::Ty *CallExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                            int labelcount) const {
  // TODO: Put your codes here (lab4).

  E::EnvEntry *envEntry = venv->Look(func);
  if (!envEntry){
    errormsg.Error(pos, "undefined function %s", func->Name().c_str());
    return TY::VoidTy::Instance();
  }
  E::FunEntry *funcEntry = (E::FunEntry *)envEntry;
  TY::TyList *tyList = funcEntry->formals;
  A::ExpList *expList = args;
  while(tyList && expList) {
    TY::Ty *ty = expList->head->SemAnalyze(venv, tenv, labelcount);
    if(!ty->IsSameType(tyList->head)){
      errormsg.Error(pos, "para type mismatch");
    }
    expList = expList->tail;
    tyList = tyList->tail;
  } 
  if(tyList) {
    errormsg.Error(pos, "missing params in function %s", func->Name().c_str());
  }
  if(expList) {
    errormsg.Error(pos, "too many params in function %s", func->Name().c_str());
  }
  if(funcEntry->result) {
    return funcEntry->result;
  } else return TY::VoidTy::Instance();
}

TY::Ty *OpExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  TY::Ty *leftTy = left->SemAnalyze(venv, tenv, labelcount);
  TY::Ty *rightTy = right->SemAnalyze(venv, tenv, labelcount);

  switch (oper)
  {
  case PLUS_OP:
  case MINUS_OP:
  case TIMES_OP:
  case DIVIDE_OP:
    if (!leftTy->IsSameType(TY::IntTy::Instance()) || !rightTy->IsSameType(TY::IntTy::Instance()))
      errormsg.Error(pos, "integer required");
    break;
  case EQ_OP:
  case NEQ_OP:
  case LT_OP:
  case LE_OP:
  case GT_OP:
  case GE_OP:
    if (!leftTy->IsSameType(rightTy))
      errormsg.Error(pos, "same type required");
    break;
  default:
    break;
  }
 
  return TY::IntTy::Instance();
}

TY::Ty *RecordExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                              int labelcount) const {
  // TODO: Put your codes here (lab4).
  TY::Ty *ty = tenv->Look(typ);
  if (!ty) {
    errormsg.Error(pos, "undefined type %s", typ->Name().c_str());
  } else {
    // TY::RecordTy *recordTy = (TY::RecordTy *)ty;
    // TY::FieldList *recFields = recordTy->fields;
    // A::EFieldList *fieldsPt = fields;
    // while (fieldsPt && recFields)
    // {
    //   if (!fieldsPt->head->exp->SemAnalyze(venv, tenv, labelcount)
    //         ->IsSameType(recFields->head->ty))
    //   {
    //     errormsg.Error(pos, "mismatched type in record expression");
    //   }
    //   fieldsPt = fieldsPt->tail;
    //   recFields = recFields->tail;
    // }
  }
  return ty;
}

TY::Ty *SeqExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  A::ExpList *expList = seq;
  TY::Ty *expListTy;
  while (expList)
  {
    expListTy = expList->head->SemAnalyze(venv, tenv, labelcount);
    if(!expList->tail) {
      return expListTy;
    } else expList = expList->tail;
  }
}

TY::Ty *AssignExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                              int labelcount) const {
  // TODO: Put your codes here (lab4).
  TY::Ty *varTy = var->SemAnalyze(venv, tenv, labelcount);
  TY::Ty *expTy = exp->SemAnalyze(venv, tenv, labelcount);

  if(!expTy->IsSameType(varTy)) {
    errormsg.Error(pos, "unmatched assign exp");
  }
  if(var->kind == A::Var::SIMPLE) {
    A::SimpleVar *sv = (A::SimpleVar*)var;
    E::VarEntry *ve = (E::VarEntry*)venv->Look(sv->sym);
    if(ve->readonly){
      errormsg.Error(pos, "loop variable can't be assigned");
    }
  } 
  return TY::VoidTy::Instance();
}

TY::Ty *IfExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  TY::Ty *testTy, *thenTy, *elseTy;
  testTy = test->SemAnalyze(venv, tenv, labelcount);
  if (!testTy || !testTy->IsSameType(TY::IntTy::Instance())) {
    errormsg.Error(pos, "if test must be integer");
  }
  thenTy = then->SemAnalyze(venv, tenv, labelcount);
  
  if (!elsee) {
    if (!thenTy || !thenTy->IsSameType(TY::VoidTy::Instance())) {
      errormsg.Error(pos, "if-then exp's body must produce no value");
    }  
    return TY::VoidTy::Instance();
  } else {
    elseTy = elsee->SemAnalyze(venv, tenv, labelcount);
    if(!thenTy->IsSameType(elseTy)){
      errormsg.Error(pos, "then exp and else exp type mismatch");
    }
    return thenTy;
  }
}

TY::Ty *WhileExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                             int labelcount) const {
  // TODO: Put your codes here (lab4).
  TY::Ty *testTy = test->SemAnalyze(venv, tenv, labelcount);
  if (!testTy || !testTy->IsSameType(TY::IntTy::Instance())) {
    errormsg.Error(pos, "while test must be integer");
  }
  TY::Ty *bodyTy = body->SemAnalyze(venv, tenv, labelcount);
  if (!bodyTy || !bodyTy->IsSameType(TY::VoidTy::Instance())) {
    errormsg.Error(pos, "while body must produce no value");
  }
  return TY::VoidTy::Instance();
}

TY::Ty *ForExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  
  TY::Ty *loTy = lo->SemAnalyze(venv, tenv, labelcount);
  TY::Ty *hiTy = hi->SemAnalyze(venv, tenv, labelcount);
  if (!loTy || !loTy->IsSameType(TY::IntTy::Instance())) {
    errormsg.Error(pos, "for exp's range type is not integer");
  }
  if (!hiTy || !hiTy->IsSameType(TY::IntTy::Instance())) {
    errormsg.Error(pos, "for exp's range type is not integer");
  }

  venv->BeginScope();
  venv->Enter(var, new E::VarEntry(TY::IntTy::Instance(), true));
  TY::Ty *expTy = body->SemAnalyze(venv, tenv, labelcount);
  if(!expTy || !expTy->IsSameType(TY::VoidTy::Instance())) {
    errormsg.Error(pos, "for expression must produce no value");
  }
  venv->EndScope();
  return TY::VoidTy::Instance();
}

TY::Ty *BreakExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                             int labelcount) const {
  // TODO: Put your codes here (lab4).
  return TY::VoidTy::Instance();
}

TY::Ty *LetExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  // tenv only begin from let exp
  TY::Ty *letTy;
  venv->BeginScope();
  tenv->BeginScope();

  A::DecList *decList = decs;
  while (decList)
  {
    decList->head->SemAnalyze(venv, tenv, labelcount);
    decList = decList->tail;
  }
  
  letTy = body->SemAnalyze(venv, tenv, labelcount);
  tenv->EndScope();
  venv->EndScope();
  return letTy;
}

TY::Ty *ArrayExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                             int labelcount) const {
  // TODO: Put your codes here (lab4).
  TY::Ty *ty = tenv->Look(typ);
  if(!ty) {
    errormsg.Error(pos, "undefiend type %s", typ->Name().c_str());
  }

  ty = ty->ActualTy();
  if(ty->kind != TY::Ty::ARRAY) {
    errormsg.Error(pos, "not array type");
  }
  TY::ArrayTy *expected = (TY::ArrayTy *)ty;

  ty = size->SemAnalyze(venv, tenv, labelcount);
  if (!ty->IsSameType(TY::IntTy::Instance())) {
    errormsg.Error(pos, "integer required");
  }
  
  ty = init->SemAnalyze(venv, tenv, labelcount);
  if(!ty->IsSameType(expected->ty)){
    errormsg.Error(pos, "type mismatch");
  }
  return expected;
}

TY::Ty *VoidExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                            int labelcount) const {
  // TODO: Put your codes here (lab4).
  return TY::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(VEnvType venv, TEnvType tenv,
                             int labelcount) const {
  // TODO: Put your codes here (lab4).
  A::FunDecList *funList = functions;
  while (funList)
  {
    A::FunDec *funDec = funList->head;
    TY::Ty *resultTy = TY::VoidTy::Instance();
    if (funDec->result) {
      resultTy = tenv->Look(funDec->result);
    }
    TY::TyList *formalTyList = make_formal_tylist(tenv, funDec->params);
    if (venv->Look(funDec->name)) {
      errormsg.Error(pos, "two functions have the same name");
    }
    else venv->Enter(funDec->name, new E::FunEntry(formalTyList, resultTy));
    funList = funList->tail;
  }

  // second pass through, check function body
  funList = functions;
  while (funList)
  {
    venv->BeginScope();
    A::FunDec *funDec = funList->head;
    A::FieldList *actuals = funDec->params;
    E::FunEntry *funEntry = (E::FunEntry *)venv->Look(funDec->name);
    TY::TyList *formals = funEntry->formals;
    while (formals && actuals)
    {
      venv->Enter(actuals->head->name, new E::VarEntry(formals->head));
      formals = formals->tail;
      actuals = actuals->tail;
    }
    TY::Ty *resTy = funDec->body->SemAnalyze(venv, tenv, labelcount);
    if(!resTy->IsSameType(funEntry->result)) {
      if(funEntry->result->IsSameType(TY::VoidTy::Instance())) {
        errormsg.Error(pos, "procedure returns value");
      } else {
        errormsg.Error(pos, "function return type mismatch");
      }
    }
    venv->EndScope();
    funList = funList->tail;
  }
  
  
}

void VarDec::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
  TY::Ty *expTy = init->SemAnalyze(venv, tenv, labelcount);
  if(venv->Look(var)) {
    errormsg.Error(pos, "two variables have the same name");
    return;
  }
  if (typ){
    TY::Ty *varTy = tenv->Look(typ);
    if(!varTy) {
      errormsg.Error(pos, "undefined type of %s", typ->Name().c_str());
    }
    if(!varTy->IsSameType(expTy) ){
      errormsg.Error(pos, "type mismatch");
    }
    venv->Enter(var, new E::VarEntry(varTy));
  }
  else {
    if(expTy->IsSameType(TY::NilTy::Instance()) && expTy->ActualTy()->kind != TY::Ty::RECORD) {
      errormsg.Error(pos, "init should not be nil without type specified");
    } else {
      venv->Enter(var, new E::VarEntry(expTy));
    }
  }
}

void TypeDec::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const {
  // TODO: Put your codes here (lab4).
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
    ((TY::NameTy *)ty)->ty = tyList->head->ty->SemAnalyze(tenv);
    tyList = tyList->tail;
  }
  // check recursive name type dec
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
}

TY::Ty *NameTy::SemAnalyze(TEnvType tenv) const {
  // TODO: Put your codes here (lab4).
  TY::Ty *ty = tenv->Look(name);
  if (!ty) {
    errormsg.Error(pos, "undefined type %s", name->Name().c_str());
  }
  return ty;
}

TY::Ty *RecordTy::SemAnalyze(TEnvType tenv) const {
  // TODO: Put your codes here (lab4).
  TY::FieldList *tyFields = make_fieldlist(tenv, record);
  return new TY::RecordTy(tyFields);
}

TY::Ty *ArrayTy::SemAnalyze(TEnvType tenv) const {
  // TODO: Put your codes here (lab4).
  TY::Ty *ty = tenv->Look(array);
  if(!ty) {
    errormsg.Error(pos, "undefined type %s", array->Name().c_str());
  }
  return new TY::ArrayTy(ty);
}
}  // namespace A

namespace SEM {
void SemAnalyze(A::Exp *root) {
  if (root) root->SemAnalyze(E::BaseVEnv(), E::BaseTEnv(), 0);
}

}  // namespace SEM
