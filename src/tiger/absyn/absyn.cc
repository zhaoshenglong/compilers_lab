#include "tiger/absyn/absyn.h"

namespace {

inline void indent(FILE *out, int d) {
  for (int i = 0; i <= d; i++) fprintf(out, " ");
}

std::string str_oper[12] = {"PLUS",  "MINUS",    "TIMES",    "DIVIDE",
                            "EQUAL", "NOTEQUAL", "LESSTHAN", "LESSEQ",
                            "GREAT", "GREATEQ"};

inline void pr_oper(FILE *out, A::Oper d) {
  fprintf(out, "%s", str_oper[d].c_str());
}

}  // namespace

namespace A {

void SimpleVar::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "simpleVar(%s)", sym->Name().c_str());
}

void FieldVar::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "%s\n", "fieldVar(");
  var->Print(out, d + 1);
  fprintf(out, "%s\n", ",");
  indent(out, d + 1);
  fprintf(out, "%s)", sym->Name().c_str());
}

void SubscriptVar::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "%s\n", "subscriptVar(");
  var->Print(out, d + 1);
  fprintf(out, "%s\n", ",");
  subscript->Print(out, d + 1);
  fprintf(out, "%s", ")");
}

void VarExp::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "varExp(\n");
  var->Print(out, d + 1);
  fprintf(out, "%s", ")");
}

void NilExp::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "nilExp()");
}

void IntExp::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "intExp(%d)", i);
}

void StringExp::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "stringExp(%s)", s.c_str());
}

void CallExp::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "callExp(%s,\n", func->Name().c_str());
  ExpList::Print(out, args, d + 1);
  fprintf(out, ")");
}

void OpExp::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "opExp(\n");
  indent(out, d + 1);
  pr_oper(out, oper);
  fprintf(out, ",\n");
  left->Print(out, d + 1);
  fprintf(out, ",\n");
  right->Print(out, d + 1);
  fprintf(out, ")");
}

void RecordExp::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "recordExp(%s,\n", typ->Name().c_str());
  EFieldList::Print(out, fields, d + 1);
  fprintf(out, ")");
}

void SeqExp::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "seqExp(\n");
  ExpList::Print(out, seq, d + 1);
  fprintf(out, ")");
}

void AssignExp::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "assignExp(\n");
  var->Print(out, d + 1);
  fprintf(out, ",\n");
  exp->Print(out, d + 1);
  fprintf(out, ")");
}

void IfExp::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "iffExp(\n");
  test->Print(out, d + 1);
  fprintf(out, ",\n");
  then->Print(out, d + 1);
  if (elsee) { /* else is optional */
    fprintf(out, ",\n");
    elsee->Print(out, d + 1);
  }
  fprintf(out, ")");
}

void WhileExp::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "whileExp(\n");
  test->Print(out, d + 1);
  fprintf(out, ",\n");
  body->Print(out, d + 1);
  fprintf(out, ")");
}

void ForExp::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "forExp(%s,\n", var->Name().c_str());
  lo->Print(out, d + 1);
  fprintf(out, ",\n");
  hi->Print(out, d + 1);
  fprintf(out, ",\n");
  body->Print(out, d + 1);
  fprintf(out, ",\n");
  indent(out, d + 1);
  fprintf(out, "%s", escape ? "TRUE)" : "FALSE)");
}

void BreakExp::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "breakExp()");
}

void LetExp::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "letExp(\n");
  DecList::Print(out, decs, d + 1);
  fprintf(out, ",\n");
  body->Print(out, d + 1);
  fprintf(out, ")");
}

void ArrayExp::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "arrayExp(%s,\n", typ->Name().c_str());
  size->Print(out, d + 1);
  fprintf(out, ",\n");
  init->Print(out, d + 1);
  fprintf(out, ")");
}

void VoidExp::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "voidExp()");
}

void FunctionDec::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "functionDec(\n");
  FunDecList::Print(out, functions, d + 1);
  fprintf(out, ")");
}

void VarDec::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "varDec(%s,\n", var->Name().c_str());
  if (typ) {
    indent(out, d + 1);
    fprintf(out, "%s,\n", typ->Name().c_str());
  }
  init->Print(out, d + 1);
  fprintf(out, ",\n");
  indent(out, d + 1);
  fprintf(out, "%s", escape ? "TRUE)" : "FALSE)");
}

void TypeDec::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "typeDec(\n");
  NameAndTyList::Print(out, types, d + 1);
  fprintf(out, ")");
}

void NameTy::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "nameTy(%s)", name->Name().c_str());
}

void RecordTy::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "recordTy(\n");
  FieldList::Print(out, record, d + 1);
  fprintf(out, ")");
}

void ArrayTy::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "arrayTy(%s)", array->Name().c_str());
}

void Field::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "field(%s,\n", name->Name().c_str());
  indent(out, d + 1);
  fprintf(out, "%s,\n", typ->Name().c_str());
  indent(out, d + 1);
  fprintf(out, "%s", escape ? "TRUE)" : "FALSE)");
}

void FieldList::Print(FILE *out, FieldList *v, int d) {
  indent(out, d);
  if (v) {
    fprintf(out, "fieldList(\n");
    v->head->Print(out, d + 1);
    fprintf(out, ",\n");
    FieldList::Print(out, v->tail, d + 1);
    fprintf(out, ")");
  } else
    fprintf(out, "fieldList()");
}

void ExpList::Print(FILE *out, ExpList *v, int d) {
  indent(out, d);
  if (v) {
    fprintf(out, "expList(\n");
    v->head->Print(out, d + 1);
    fprintf(out, ",\n");
    ExpList::Print(out, v->tail, d + 1);
    fprintf(out, ")");
  } else
    fprintf(out, "expList()");
}

void FunDec::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "fundec(%s,\n", name->Name().c_str());
  FieldList::Print(out, params, d + 1);
  fprintf(out, ",\n");
  if (result) {
    indent(out, d + 1);
    fprintf(out, "%s,\n", result->Name().c_str());
  }
  body->Print(out, d + 1);
  fprintf(out, ")");
}

void FunDecList::Print(FILE *out, FunDecList *v, int d) {
  indent(out, d);
  if (v) {
    fprintf(out, "fundecList(\n");
    v->head->Print(out, d + 1);
    fprintf(out, ",\n");
    FunDecList::Print(out, v->tail, d + 1);
    fprintf(out, ")");
  } else
    fprintf(out, "fundecList()");
}

void DecList::Print(FILE *out, DecList *v, int d) {
  indent(out, d);
  if (v) {
    fprintf(out, "decList(\n");
    v->head->Print(out, d + 1);
    fprintf(out, ",\n");
    DecList::Print(out, v->tail, d + 1);
    fprintf(out, ")");
  } else
    fprintf(out, "decList()");
}

void NameAndTy::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "nameAndTy(%s,\n", name->Name().c_str());
  ty->Print(out, d + 1);
  fprintf(out, ")");
}

void NameAndTyList::Print(FILE *out, NameAndTyList *v, int d) {
  indent(out, d);
  if (v) {
    fprintf(out, "nameAndTyList(\n");
    v->head->Print(out, d + 1);
    fprintf(out, ",\n");
    NameAndTyList::Print(out, v->tail, d + 1);
    fprintf(out, ")");
  } else
    fprintf(out, "nameAndTyList()");
}

void EField::Print(FILE *out, EField *v, int d) {
  indent(out, d);
  if (v) {
    fprintf(out, "efield(%s,\n", v->name->Name().c_str());
    v->exp->Print(out, d + 1);
    fprintf(out, ")");
  } else
    fprintf(out, "efield()");
}

void EFieldList::Print(FILE *out, EFieldList *v, int d) {
  indent(out, d);
  if (v) {
    fprintf(out, "efieldList(\n");
    EField::Print(out, v->head, d + 1);
    fprintf(out, ",\n");
    EFieldList::Print(out, v->tail, d + 1);
    fprintf(out, ")");
  } else
    fprintf(out, "efieldList()");
}

}  // namespace A
