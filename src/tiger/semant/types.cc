#include "tiger/semant/types.h"

#include <map>
#include <string>

using TEnvType = S::Table<TY::Ty> *;

namespace {
std::map<TY::Ty::Kind, std::string> ty2str = {
    {TY::Ty::RECORD, "ty_record"}, {TY::Ty::NIL, "ty_nil"},
    {TY::Ty::INT, "ty_string"},    {TY::Ty::STRING, "ty_string"},
    {TY::Ty::ARRAY, "ty_array"},   {TY::Ty::NAME, "ty_name"},
    {TY::Ty::VOID, "ty_void"}};
}

namespace TY {

NilTy NilTy::nilty_;
IntTy IntTy::intty_;
StringTy StringTy::stringty_;
VoidTy VoidTy::voidty_;
UndefinedTy UndefinedTy::undefinedty_;

Ty *Ty::ActualTy() {
  Ty *ty = this;
  while (ty->kind == TY::Ty::NAME) {
    ty = static_cast<TY::NameTy *>(ty)->ty;
    if (this == ty) return nullptr;
  }
  return ty;
}

bool Ty::IsSameType(Ty *expected) {
  Ty *a = this->ActualTy();
  Ty *b = expected->ActualTy();

  if ((a->kind == RECORD && b->kind == NIL) ||
      (a->kind == NIL && b->kind == RECORD)) {
    return true;
  }

  return a == b;
}

};  // namespace TY
