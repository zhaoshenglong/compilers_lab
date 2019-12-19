#include "tiger/codegen/codegen.h"


namespace 
{
  static AS::InstrList *iList = NULL, *last = NULL;
  static std::string *framesize_str = NULL;

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
  framesize_str = f->getFramesizeStr();

  for (sl = stmList; sl;sl = sl->tail) {
    munchStm(sl->head);
  }
  list = iList; 
  
  // clear in case overlay the next frame
  framesize_str = NULL;
  iList = last = NULL; 
  return list;
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
  TEMP::Temp *r = TEMP::Temp::NewTemp();
  switch (e->kind)
  {
  case T::Exp::CONST:{
    char instr[128];
    T::ConstExp *ce = static_cast<T::ConstExp *>(e);
    sprintf(instr, "\tmovq\t$%d, `d0", ce->consti);
    emit(new AS::MoveInstr(std::string(instr), L(r, NULL), NULL));
    // printf("case const");
    return r;
  }
  case T::Exp::TEMP:{
    T::TempExp *te = static_cast<T::TempExp *>(e);
    // transfer all FP inference into SP + framesize
    // printf("case temp");
    if (te->temp == F::FP()) {
      char instr[128];
      if (!framesize_str) {
        printf("framesize str is null!\n");
      }
      sprintf(instr, "\tleaq\t%s(%%rsp), `d0", framesize_str->c_str());
      emit(new AS::OperInstr(instr, L(r, NULL), NULL, NULL));
      return r;
    } else {
      return te->temp;
    }
  }
  case T::Exp::BINOP:{
    T::BinopExp *bine = static_cast<T::BinopExp*>(e);
    // printf("case binop");
    switch (bine->op)
    {
    case T::BinOp::PLUS_OP:{
      emit(new AS::OperInstr("\tmovq\t`s0, `d0", 
           L(r, NULL), 
           L(munchExp(bine->left), NULL), 
           NULL));
      emit(new AS::OperInstr("\taddq\t`s0, `d0", 
           L(r, NULL), 
           L(munchExp(bine->right), NULL), 
           NULL));
      return r;
    }
    case T::BinOp::MINUS_OP:{
      emit(new AS::OperInstr("\tmovq\t`s0, `d0", 
           L(r, NULL), 
           L(munchExp(bine->left), NULL), 
           NULL));
      emit(new AS::OperInstr("\tsubq\t`s0, `d0", 
           L(r, NULL), 
           L(munchExp(bine->right), NULL), 
           NULL));
      return r;
    }
    case T::BinOp::MUL_OP:{
      emit(new AS::OperInstr("\tmovq\t`s0, `d0", 
           L(r, NULL), 
           L(munchExp(bine->left), NULL), 
           NULL));
      emit(new AS::OperInstr("\timulq\t`s0, `d0", 
           L(r, NULL), 
           L(munchExp(bine->right), NULL), 
           NULL));
      return r;
    }
    case T::BinOp::DIV_OP:{
      emit(new AS::OperInstr("\tmovq\t`s0, `d0", 
           L(F::DIVIDEND(), NULL), 
           L(munchExp(bine->right), NULL), 
           NULL));
      emit(new AS::OperInstr("\tcqto", 
           L(F::DIVIDEND(), L(F::REMAINDER(), NULL)), 
           L(F::DIVIDEND(), NULL), 
           NULL));
      emit(new AS::OperInstr("\tidivq\ts0", 
            L(F::DIVIDEND(), L(F::REMAINDER(), NULL)), 
            L(munchExp(bine->left), L(F::DIVIDEND(), NULL)), 
            NULL));
      emit(new AS::MoveInstr("\tmovq\t`s0, `d0", 
            L(r, NULL), 
            L(F::DIVIDEND(), NULL)));
      return r;
    }
    default:{
      printf("invalid arithmatic operator\n");
      assert(0);
    }
    }
    return r;
  }
  case T::Exp::NAME:{
    // printf("case name");
    T::NameExp *ne = (T::NameExp *)e;
    // case: mov string ref to register
    // case: call 
    // case: jump
    // only need to handle string reference
    char instr[128];
    sprintf(instr, "\tleaq\t%s(%%rip), `d0", ne->name->Name().c_str());
    emit(new AS::OperInstr(instr, L(r, NULL), NULL, NULL));
    return r;
  }
  case T::Exp::MEM:{
    // printf("case mem");
    emit(new AS::MoveInstr("\tmovq\t(`s0), `d0", L(r, NULL), L(munchExp(static_cast<T::MemExp *>(e)->exp  ), NULL)));
    return r;
  }
  case T::Exp::CALL:{
    // printf("case call");
    T::CallExp *callExp = static_cast<T::CallExp *>(e);
    T::NameExp *nameExp = static_cast<T::NameExp *>(callExp->fun);
    TEMP::TempList *calldefs = new TEMP::TempList(F::RV(), F::CallerRegs());
    TEMP::TempList *l = munchArgs(0, callExp->args);
    // NOTE: runtime link needs plt
    emit(new AS::OperInstr("\tcall\t" + nameExp->name->Name() + "@plt", calldefs, l, NULL));
    emit(new AS::MoveInstr("\tmovq\t`s0, `d0", L(r, NULL), L(F::RV(), NULL)));
    return r;
  }
  default:{
    assert(0);
    break;
  }
  }
}


static TEMP::TempList* munchArgs(int off, T::ExpList *args) {
  TEMP::TempList *argregs = F::ArgRegs();
  int cnt = 0;
  TEMP::TempList *tl = NULL;
  TEMP::TempList *tlPtr = tl;

  while (args){
    if (cnt >= 6) {
      // Should alloc space in caller frame, but not cover by test
      // Worse is better
      emit(new AS::OperInstr("\tpushq `s0", NULL, L(munchExp(args->head), NULL), NULL));
    } else {
      emit(new AS::MoveInstr("\tmovq\t`s0, `d0", L(argregs->head, NULL), L(munchExp(args->head), NULL)));
      if (!tl) {
        tl = new TEMP::TempList(argregs->head, NULL);
        tlPtr = tl;
      } else {
        TEMP::TempList *t = new TEMP::TempList(argregs->head, NULL);
        tlPtr->tail = t;
        tlPtr = t;
      }
      cnt ++;
      argregs = argregs->tail;
    }
    args = args->tail;
  }
  return tl;
}

static void munchStm(T::Stm *s) {
  std::string cjmp;
  switch (s->kind)
  {
  case T::Stm::MOVE:{
    T::MoveStm *mvStm = (T::MoveStm *)s;
    T::Exp *dste = mvStm->dst;
    T::Exp *srce = mvStm->src;
    if (dste->kind == T::Exp::TEMP) {
      if (srce->kind == T::Exp::MEM) {
        // move memory data to register
        emit(new AS::MoveInstr("\tmovq\t(`s0), `d0", 
                L(munchExp(dste), NULL), 
                L(munchExp(static_cast<T::MemExp *>(srce)->exp), NULL)));
      } else if (srce->kind == T::Exp::CONST){
        // move immediate to register
        char instr[128];
        sprintf(instr, "\tmovq\t$%d, `d0", static_cast<T::ConstExp *>(srce)->consti);
        emit(new AS::OperInstr(instr, 
                L(munchExp(dste), NULL), 
                NULL, NULL));
      } else {
        emit(new AS::MoveInstr("\tmovq\t`s0, `d0", L(munchExp(dste), NULL), L(munchExp(srce), NULL)));
      }
    } else if(dste->kind == T::Exp::MEM) {
      T::MemExp *meme = (T::MemExp *)dste;
      emit(new AS::MoveInstr("\tmovq\t`s0, (`d0)", L(munchExp(meme->exp), NULL), L(munchExp(srce), NULL)));
    } else {
      // A good translate would never go here
      assert(0);
    }
    break;
  }
  case T::Stm::EXP:{
    T::ExpStm *expStm = (T::ExpStm *)s;
    munchExp(expStm->exp);
    break;
  }
  case T::Stm::JUMP:{
    T::JumpStm *jumpStm = (T::JumpStm *)s;
    T::NameExp *namee = (T::NameExp *)jumpStm->exp;
    char inst[128];
    sprintf(inst, "\tjmp\t%s", namee->name->Name().c_str());
    emit(new AS::OperInstr(inst, NULL, NULL, new AS::Targets(jumpStm->jumps)));
    break;
  }
  case T::Stm::CJUMP:{
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
    emit(new AS::OperInstr("\tcmpq\t`s1, `s0", NULL, L(munchExp(cs->left), L(munchExp(cs->right), NULL)), NULL));
    emit(new AS::OperInstr(cjmp + cs->true_label->Name(), 
         NULL, NULL, new AS::Targets(new TEMP::LabelList(cs->true_label, NULL))));
    break;
  }
  case T::Stm::LABEL:{
    T::LabelStm *ls = (T::LabelStm *)s;
    emit(new AS::LabelInstr(ls->label->Name(), ls->label));
    break;
  }
  case T::Stm::SEQ:{
    // should removed by cannon
    assert(0);
    break;
  }
  default:
    break;
  }
}
} // namespace
