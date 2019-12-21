#include "tiger/translate/tree.h"

#include <cassert>

namespace {

static void indent(FILE *out, int d) {
  for (int i = 0; i <= d; i++) fprintf(out, " ");
}

static char bin_oper[][12] = {"PLUS", "MINUS",  "TIMES",  "DIVIDE",  "AND",
                              "OR",   "LSHIFT", "RSHIFT", "ARSHIFT", "XOR"};

static char rel_oper[][12] = {"EQ", "NE",  "LT",  "GT",  "LE",
                              "GE", "ULT", "ULE", "UGT", "UGE"};

}  // namespace

namespace T {

void SeqStm::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "SEQ(\n");
  this->left->Print(out, d + 1);
  fprintf(out, ",\n");
  this->right->Print(out, d + 1);
  fprintf(out, ")");
}

void LabelStm::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "LABEL %s", this->label->Name().c_str());
}

void JumpStm::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "JUMP(\n");
  this->exp->Print(out, d + 1);
  fprintf(out, ")");
}

void CjumpStm::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "CJUMP(%s,\n", rel_oper[this->op]);
  this->left->Print(out, d + 1);
  fprintf(out, ",\n");
  this->right->Print(out, d + 1);
  fprintf(out, ",\n");
  indent(out, d + 1);
  fprintf(out, "%s,", this->true_label->Name().c_str());
  fprintf(out, "%s", this->false_label->Name().c_str());
  fprintf(out, ")");
}

void MoveStm::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "MOVE(\n");
  this->dst->Print(out, d + 1);
  fprintf(out, ",\n");
  this->src->Print(out, d + 1);
  fprintf(out, ")");
}

void ExpStm::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "EXP(\n");
  this->exp->Print(out, d + 1);
  fprintf(out, ")");
}

void BinopExp::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "BINOP(%s,\n", bin_oper[this->op]);
  left->Print(out, d + 1);
  fprintf(out, ",\n");
  right->Print(out, d + 1);
  fprintf(out, ")");
}

void MemExp::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "MEM");
  fprintf(out, "(\n");
  this->exp->Print(out, d + 1);
  fprintf(out, ")");
}

void TempExp::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "TEMP t%s", TEMP::Map::Name()->Look(this->temp)->c_str());
}

void EseqExp::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "ESEQ(\n");
  this->stm->Print(out, d + 1);
  fprintf(out, ",\n");
  this->exp->Print(out, d + 1);
  fprintf(out, ")");
}

void NameExp::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "NAME %s", this->name->Name().c_str());
}

void ConstExp::Print(FILE *out, int d) const {
  indent(out, d);
  fprintf(out, "CONST %d", this->consti);
}

void CallExp::Print(FILE *out, int d) const {
  ExpList *args = this->args;
  indent(out, d);
  fprintf(out, "CALL(\n");
  this->fun->Print(out, d + 1);
  for (; args; args = args->tail) {
    fprintf(out, ",\n");
    args->head->Print(out, d + 2);
  }
  fprintf(out, ")");
}

void StmList::Print(FILE *out) const {
  this->head->Print(out, 0);
  fprintf(out, "\n");
  if (this->tail) this->tail->Print(out);
}

RelOp notRel(RelOp r) {
  switch (r) {
    case EQ_OP:
      return NE_OP;
    case NE_OP:
      return EQ_OP;
    case LT_OP:
      return GE_OP;
    case GE_OP:
      return LT_OP;
    case GT_OP:
      return LE_OP;
    case LE_OP:
      return GT_OP;
    case ULT_OP:
      return UGE_OP;
    case UGE_OP:
      return ULT_OP;
    case ULE_OP:
      return UGT_OP;
    case UGT_OP:
      return ULE_OP;
    default:
      assert(false);
  }
}

RelOp commute(RelOp r) {
  switch (r) {
    case EQ_OP:
      return EQ_OP;
    case NE_OP:
      return NE_OP;
    case LT_OP:
      return GT_OP;
    case GE_OP:
      return LE_OP;
    case GT_OP:
      return LT_OP;
    case LE_OP:
      return GE_OP;
    case ULT_OP:
      return UGT_OP;
    case UGE_OP:
      return ULE_OP;
    case ULE_OP:
      return UGE_OP;
    case UGT_OP:
      return ULT_OP;
    default:
      assert(false);
  }
}

}  // namespace T
