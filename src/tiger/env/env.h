#ifndef TIGER_ENV_ENV_H_
#define TIGER_ENV_ENV_H_

#include "tiger/frame/temp.h"
#include "tiger/semant/types.h"
#include "tiger/symbol/symbol.h"
#include "tiger/translate/translate.h"

/* Forward Declarations */
namespace TR {
class Access;
class Level;
}  // namespace TR

namespace E {
class EnvEntry {
 public:
  enum Kind { VAR, FUN };

  Kind kind;
  bool readonly;

  EnvEntry(Kind kind, bool readonly = true) : kind(kind), readonly(readonly) {}
};

class VarEntry : public EnvEntry {
 public:
  TR::Access *access;
  TY::Ty *ty;

  // For lab4(semantic analysis) only
  VarEntry(TY::Ty *ty, bool readonly = false)
      : EnvEntry(VAR, readonly), ty(ty), access(nullptr){};

  // For lab5(translate IR tree)
  VarEntry(TR::Access *access, TY::Ty *ty, bool readonly = false)
      : EnvEntry(VAR, readonly), ty(ty), access(access){};
};

class FunEntry : public EnvEntry {
 public:
  TR::Level *level;
  TEMP::Label *label;
  TY::TyList *formals;
  TY::Ty *result;

  // For lab4(semantic analysis) only
  FunEntry(TY::TyList *formals, TY::Ty *result)
      : EnvEntry(FUN),
        formals(formals),
        result(result),
        level(nullptr),
        label(nullptr) {}

  // For lab5(translate IR tree)
  FunEntry(TR::Level *level, TEMP::Label *label, TY::TyList *formals,
           TY::Ty *result)
      : EnvEntry(FUN),
        formals(formals),
        result(result),
        level(level),
        label(label) {}
};

S::Table<TY::Ty> *BaseTEnv();
S::Table<EnvEntry> *BaseVEnv();

}  // namespace E

#endif  // TIGER_ENV_ENV_H_
