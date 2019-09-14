#include "tiger/env/env.h"

namespace E {

S::Table<TY::Ty>* BaseTEnv() {
  S::Table<TY::Ty>* tenv = new S::Table<TY::Ty>();
  tenv->Enter(S::Symbol::UniqueSymbol("int"), TY::IntTy::Instance());
  tenv->Enter(S::Symbol::UniqueSymbol("string"), TY::StringTy::Instance());
  return tenv;
}

S::Table<EnvEntry>* BaseVEnv() {
  S::Table<EnvEntry>* venv = new S::Table<EnvEntry>();

  TY::Ty* result = nullptr;
  TY::TyList* formals = nullptr;

  TEMP::Label* label = nullptr;
  TR::Level* level = TR::Outermost();

  venv->Enter(S::Symbol::UniqueSymbol("flush"),
              new E::FunEntry(level, label, nullptr, TY::VoidTy::Instance()));

  result = TY::IntTy::Instance();
  formals = new TY::TyList(TY::IntTy::Instance(), nullptr);

  venv->Enter(S::Symbol::UniqueSymbol("exit"),
              new E::FunEntry(level, label, formals, TY::VoidTy::Instance()));

  // venv->Enter(S::Symbol::UniqueSymbol("not"),new
  // E::FunEntry(level,label,formals,result));

  result = TY::StringTy::Instance();

  venv->Enter(S::Symbol::UniqueSymbol("chr"),
              new E::FunEntry(level, label, formals, result));

  venv->Enter(S::Symbol::UniqueSymbol("getchar"),
              new E::FunEntry(level, label, nullptr, result));

  formals = new TY::TyList(TY::StringTy::Instance(), nullptr);

  venv->Enter(S::Symbol::UniqueSymbol("print"),
              new E::FunEntry(level, label, formals, TY::VoidTy::Instance()));
  venv->Enter(S::Symbol::UniqueSymbol("printi"),
              new E::FunEntry(level, label,
                              new TY::TyList(TY::IntTy::Instance(), nullptr),
                              TY::VoidTy::Instance()));

  result = TY::IntTy::Instance();
  venv->Enter(S::Symbol::UniqueSymbol("ord"),
              new E::FunEntry(level, label, formals, result));

  venv->Enter(S::Symbol::UniqueSymbol("size"),
              new E::FunEntry(level, label, formals, result));

  result = TY::StringTy::Instance();
  formals = new TY::TyList(TY::StringTy::Instance(),
                           new TY::TyList(TY::StringTy::Instance(), nullptr));
  venv->Enter(S::Symbol::UniqueSymbol("concat"),
              new E::FunEntry(level, label, formals, result));

  formals = new TY::TyList(
      TY::StringTy::Instance(),
      new TY::TyList(TY::IntTy::Instance(),
                     new TY::TyList(TY::IntTy::Instance(), nullptr)));
  venv->Enter(S::Symbol::UniqueSymbol("substring"),
              new E::FunEntry(level, label, formals, result));
  return venv;
}

}  // namespace E
