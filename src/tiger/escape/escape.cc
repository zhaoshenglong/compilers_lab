#include "tiger/escape/escape.h"

namespace ESC {

class EscapeEntry {
 public:
  int depth;
  bool* escape;

  EscapeEntry(int depth, bool* escape) : depth(depth), escape(escape) {}
};

// Escape Analysis For ForExp(i), VarDec(variable), Field(params)
void FindEscape(A::Exp* exp) {
  S::Table<ESC::EscapeEntry> *escEnv = new S::Table<ESC::EscapeEntry>();

  if(exp) exp->EscapeAnalysis(escEnv, 0);
}

}  // namespace ESC

namespace A
{
  using EscEnvType = S::Table<ESC::EscapeEntry> *;

  void SimpleVar::EscapeAnalysis(EscEnvType escEnv, int depth) {
    ESC::EscapeEntry *entry = escEnv->Look(sym);
    if (!entry) {
      // ... Escape does not deal with this
      return;
    }
    if(depth > entry->depth) {
      *(entry->escape) = true;
    }
  }
  void FieldVar::EscapeAnalysis(EscEnvType escEnv, int depth) {
    var->EscapeAnalysis(escEnv, depth);
  }
  void SubscriptVar::EscapeAnalysis(EscEnvType escEnv, int depth) {
    var->EscapeAnalysis(escEnv, depth);
    subscript->EscapeAnalysis(escEnv, depth);
  }
  
  void VarExp::EscapeAnalysis(EscEnvType escEnv, int depth) {
    var->EscapeAnalysis(escEnv, depth);
  }
  void NilExp::EscapeAnalysis(EscEnvType escEnv, int depth) {
    // Do Nothing
  }
  void IntExp::EscapeAnalysis(EscEnvType escEnv, int depth) {
    // Do Nothing
  }
  void StringExp::EscapeAnalysis(EscEnvType escEnv, int depth) {
    // Do Nothing
  }
  void CallExp::EscapeAnalysis(EscEnvType escEnv, int depth) {
    // Do Nothing
  }
  void OpExp::EscapeAnalysis(EscEnvType escEnv, int depth) {
    left->EscapeAnalysis(escEnv, depth);
    right->EscapeAnalysis(escEnv, depth);
  }
  void RecordExp::EscapeAnalysis(EscEnvType escEnv, int depth) {
    EFieldList* fieldList = fields;
    while (fieldList) {
      fieldList->head->exp->EscapeAnalysis(escEnv, depth);
      fieldList = fieldList->tail;
    }
  }
  void SeqExp::EscapeAnalysis(EscEnvType escEnv, int depth) {
    ExpList *expList = seq;
    while (expList) {
      expList->head->EscapeAnalysis(escEnv, depth);
      expList = expList->tail;
    }
  }
  void AssignExp::EscapeAnalysis(EscEnvType escEnv, int depth) {
    var->EscapeAnalysis(escEnv, depth);
    exp->EscapeAnalysis(escEnv, depth);
  }
  void IfExp::EscapeAnalysis(EscEnvType escEnv, int depth) {
    test->EscapeAnalysis(escEnv, depth);
    then->EscapeAnalysis(escEnv, depth);
    if (elsee) elsee->EscapeAnalysis(escEnv, depth);
  }
  void WhileExp::EscapeAnalysis(EscEnvType escEnv, int depth) {
    test->EscapeAnalysis(escEnv, depth);
    body->EscapeAnalysis(escEnv, depth);
  }
  void ForExp::EscapeAnalysis(EscEnvType escEnv, int depth) {
    // Note: Enter iterator varible into ENV
    escEnv->Enter(var, new ESC::EscapeEntry(depth, &escape));
    lo->EscapeAnalysis(escEnv, depth);
    hi->EscapeAnalysis(escEnv, depth);
    body->EscapeAnalysis(escEnv, depth);
  }
  void BreakExp::EscapeAnalysis(EscEnvType escEnv, int depth) {
    // Do Nothing
  }
  void LetExp::EscapeAnalysis(EscEnvType escEnv, int depth) {
    DecList *decList = decs;
    while (decList) {
      decList->head->EscapeAnalysis(escEnv, depth);
      decList = decList->tail;
    }
    body->EscapeAnalysis(escEnv, depth);
  }
  void ArrayExp::EscapeAnalysis(EscEnvType escEnv, int depth) {
    size->EscapeAnalysis(escEnv, depth);
    init->EscapeAnalysis(escEnv, depth);
  }
  void VoidExp::EscapeAnalysis(S::Table<ESC::EscapeEntry> *escEnv, int depth) {
    // Do Nothing
  }

  void FunctionDec::EscapeAnalysis(S::Table<ESC::EscapeEntry> *escEnv, int depth) {
    FunDecList *funList = functions;
    ++depth;
    while (funList) {
      // Order Could Not Be Reversed
      FieldList *fieldList = funList->head->params;
      while (fieldList) {
        escEnv->Enter(fieldList->head->name, new ESC::EscapeEntry(depth, &fieldList->head->escape));
      }
      funList->head->body->EscapeAnalysis(escEnv, depth);
      funList = funList->tail;
    }
    
  }
  void VarDec::EscapeAnalysis(EscEnvType escEnv, int depth) {
    escEnv->Enter(var, new ESC::EscapeEntry(depth, &escape));
    init->EscapeAnalysis(escEnv, depth);
  }
  void TypeDec::EscapeAnalysis(EscEnvType escEnv, int depth) {
    // Do Nothing
  }
} // namespace A
