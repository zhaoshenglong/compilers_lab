#include "tiger/codegen/codegen.h"


namespace 
{
  static AS::InstrList *iList = NULL, *last = NULL;


  static TEMP::TempList *L(TEMP::Temp *r1, TEMP::TempList *r2) {
    return new TEMP::TempList(r1, r2);
  }

  static void emit(AS::Instr *inst);
  static TEMP::TempList *munchArgs(int off, T::ExpList *args);
  static TEMP::Temp *munchExp(T::Exp *e);
  static void munchStm(T::Stm *s);

} // namespace 

namespace CG {

AS::InstrList* Codegen(F::Frame* f, T::StmList* stmList) {
  AS::InstrList *list; 
  T::StmList *sl;
  for (sl = stmList; sl;sl = sl->tail) {
    munchStm(sl->head);
  }
  list = iList; iList = last = NULL; return list;
}

}  // namespace CG

namespace {

static void emit(AS::Instr *inst) {
    if(last != NULL)
      last = last->tail = new AS::InstrList(inst, NULL);
    else
    {
      last = iList = new AS::InstrList(inst, NULL);
    }
    
  }


static TEMP::Temp *munchExp(T::Exp *e) {
  switch (e->kind)
  {
  case T::Exp::CONST:
    TEMP::Temp *r = TEMP::Temp::NewTemp();
    T::ConstExp *ce = (T::ConstExp *)e;
    char s[128];
    sprintf(s, "%s%d%s\n", "\tmovq\t`$", ce->consti, ", `d0");
    emit(new AS::OperInstr(std::string(s), L(r, NULL), NULL, NULL));
    return r;
  case T::Exp::TEMP:
    return ((T::TempExp *)e)->temp;
  case T::Exp::BINOP:
    TEMP::Temp *r = TEMP::Temp::NewTemp();
    T::BinopExp *bine = (T::BinopExp*)e;
    switch (bine->op)
    {
    case T::BinOp::PLUS_OP:
      emit(new AS::OperInstr("\tmovq\t`s0, `d0\n", 
           L(r, NULL), 
           L(munchExp(bine->left), NULL), 
           NULL));
      emit(new AS::OperInstr("\taddq\t`s0, `d0\n", 
           L(r, NULL), 
           L(munchExp(bine->right), NULL), 
           NULL));
      break;
    case T::BinOp::MINUS_OP:
      emit(new AS::OperInstr("\tmovq\t`s0, `d0\n", 
           L(r, NULL), 
           L(munchExp(bine->left), NULL), 
           NULL));
      emit(new AS::OperInstr("\tsubq\t`s0, `d0\n", 
           L(r, NULL), 
           L(munchExp(bine->right), NULL), 
           NULL));
      break;
    case T::BinOp::MUL_OP:
      emit(new AS::OperInstr("\tmovq\t`s0, `d0\n", 
           L(r, NULL), 
           L(munchExp(bine->left), NULL), 
           NULL));
      emit(new AS::OperInstr("\timulq\t`s0, `d0\n", 
           L(r, NULL), 
           L(munchExp(bine->right), NULL), 
           NULL));
      break;
    case T::BinOp::DIV_OP:
      emit(new AS::OperInstr("\tmovq\t`s0, `d0\n", 
           L(F::DIVIDEND(), NULL), 
           L(munchExp(bine->right), NULL), 
           NULL));
      emit(new AS::OperInstr("\tcqto\n", 
           L(F::DIVIDEND(), L(F::REMAINDER(), NULL)), 
           L(F::DIVIDEND(), NULL), 
           NULL));
      emit(new AS::OperInstr("\tidivq\ts0\n", 
            L(F::DIVIDEND(), L(F::REMAINDER(), NULL)), 
            L(munchExp(bine->left), L(F::DIVIDEND(), NULL)), 
            NULL));
      emit(new AS::MoveInstr("\tmovq\t`s0, `d0", 
            L(r, NULL), 
            L(F::DIVIDEND(), NULL)));
      break;
    default:
      printf("invalid arithmatic operator\n");
      assert(0);
    }
    return r;
  case T::Exp::NAME:
    // TODO: how to deal with this case, should not happen?
    assert(0);
    // TEMP::Temp *r = TEMP::Temp::NewTemp();
    // T::NameExp *ne = (T::NameExp *)e;
    // ne->name->Name();
    // emit(new AS::MoveInstr("\tmovq\t`s0, `d0\n", L(r, NULL), L(ne->name, NULL)));
  case T::Exp::MEM:
    TEMP::Temp *r = TEMP::Temp::NewTemp();
    emit(new AS::MoveInstr("\tmovq\t(`s0), `d0\n", L(r, NULL), L(munchExp(e), NULL)));
    return r;
  case T::Exp::CALL:
    T::CallExp *callExp = (T::CallExp *)e;
    T::NameExp *nameExp = (T::NameExp *)callExp->fun;
    TEMP::TempList *calldefs = new TEMP::TempList(F::RV(), F::CallerRegs());
    TEMP::TempList *l = munchArgs(0, callExp->args);
    // NOTE: runtime link needs plt
    emit(new AS::OperInstr("\tcall " + nameExp->name->Name() + "@plt\n", calldefs, L(munchExp(callExp->fun), l), NULL));
    emit(new AS::MoveInstr("\tmovq\t`s0, `d0\n", L(r, NULL), L(F::RV(), NULL)));
    return r;
  default:
    assert(0);
    break;
  }
}


static TEMP::TempList* munchArgs(int off, T::ExpList *args) {
  TEMP::TempList *argregs = F::ArgRegs();
  int cnt = 0;
  TEMP::TempList *tl = NULL;
  TEMP::TempList *tlPtr = tl;

  while (args){
    if (cnt >= 6) {
      emit(new AS::MoveInstr("\tpushq `s0\n", NULL, L(munchExp(args->head), NULL)));
    } else {
      emit(new AS::MoveInstr("\tmovq\t`d0, `s0\n", L(argregs->head, NULL), L(munchExp(args->head), NULL)));
      cnt ++;
      if (!tl) {
        tl = new TEMP::TempList(argregs->head, NULL);
        tlPtr = tl;
      } else {
        TEMP::TempList *t = new TEMP::TempList(argregs->head, NULL);
        tlPtr->tail = t;
        tlPtr = t;
      }
    }
    args = args->tail;
  }
  return tl;
}

static void munchStm(T::Stm *s) {
  std::string cjmp;
  switch (s->kind)
  {
  case T::Stm::MOVE:
    T::MoveStm *mvStm = (T::MoveStm *)s;
    T::Exp *dste = mvStm->dst;
    T::Exp *srce = mvStm->src;
    if (dste->kind == T::Exp::TEMP) {
      emit(new AS::MoveInstr("\tmovq\t`s0, `d0\n", L(munchExp(dste), NULL), L(munchExp(srce), NULL)));
    } else if(dste->kind == T::Exp::MEM) {
      T::MemExp *meme = (T::MemExp *)dste;
      emit(new AS::MoveInstr("\tmovq\t(`s0), `d0\n", L(munchExp(meme->exp), NULL), L(munchExp(srce), NULL)));
    } else {
      // should never go here
      assert(0);
    }
    break;
  case T::Stm::EXP:
    T::ExpStm *expStm = (T::ExpStm *)s;
    munchExp(expStm->exp);
    break;
  case T::Stm::JUMP:
    T::JumpStm *jumpStm = (T::JumpStm *)s;
    T::NameExp *namee = (T::NameExp *)jumpStm->exp;
    char inst[128];
    sprintf(inst, "\tjmp %s", namee->name->Name().c_str());
    emit(new AS::OperInstr(inst, NULL, NULL, new AS::Targets(jumpStm->jumps)));
    break;
  case T::Stm::CJUMP:
    T::CjumpStm *cs = (T::CjumpStm *)s;
    cjmp = "";
    switch (cs->op)
    {
    case T::RelOp::EQ_OP :
      cjmp = "\tje ";
      break;
    case T::RelOp::GE_OP:
      cjmp = "\tjge ";
      break;
    case T::RelOp::GT_OP:
      cjmp = "\tjg ";
      break;
    case T::RelOp::LE_OP:
      cjmp = "\tjle ";
      break;
    case T::RelOp::LT_OP:
      cjmp = "\tjl ";
      break;
    case T::RelOp::NE_OP:
      cjmp = "\tjne ";
      break;
    default:
      assert(0);
      break;
    }
    emit(new AS::OperInstr("\tcmpq\t`s1, `s0\n", NULL, L(munchExp(cs->left), L(munchExp(cs->right), NULL)), NULL));
    emit(new AS::OperInstr(cjmp + cs->true_label->Name(), 
         NULL, NULL, new AS::Targets(new TEMP::LabelList(cs->true_label, NULL))));
    break;
  case T::Stm::LABEL:
    T::LabelStm *ls = (T::LabelStm *)s;
    emit(new AS::LabelInstr(ls->label->Name() + ":\n", ls->label));
    break;
  case T::Stm::SEQ:
    // should removed by cannon
    assert(0);
    break;
  default:
    break;
  }
}
} // namespace
