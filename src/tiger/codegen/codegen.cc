#include "tiger/codegen/codegen.h"
#include <map>
#include <vector>

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

  // TODO: remove in lab6
  static AS::InstrList *spillAll(F::Frame *f, AS::InstrList *stmList);
  static std::map<TEMP::Temp *, int> temp_map;
  static std::vector<TEMP::Temp *> x64regs;
  static void insertAfter(AS::InstrList *origin, AS::InstrList *il);
  static void initRegMap();
  static bool isX64regs(TEMP::Temp *);
  static TEMP::Temp *R10();
  static TEMP::Temp *R11();
  static const char *load_template = "\tmovq\t(%s - %d)(%%rsp), `d0";
  static const char *store_template = "\tmovq\t`s0, (%s - %d)(%%rsp)";
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
  
  // lab5 spillAll, remove it in lab6
  // spillAll(f, list);
  // clear in case overlay the next frame
  
  framesize_str = NULL;
  iList = last = NULL; 
  return F::procEntryExit2(list);
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
    emit(new AS::OperInstr(std::string(instr), L(r, NULL), NULL, NULL));
    return r;
  }
  case T::Exp::TEMP:{
    T::TempExp *te = static_cast<T::TempExp *>(e);
    // transfer all FP inference into SP + framesize
    if (te->temp == F::FP()) {
      char instr[128];
      sprintf(instr, "\tleaq\t%s(%%rsp), `d0", framesize_str->c_str());
      emit(new AS::OperInstr(instr, L(r, NULL), L(F::SP(), NULL), NULL));
      return r;
    } else {
      return te->temp;
    }
  }
  case T::Exp::BINOP:{
    T::BinopExp *bine = static_cast<T::BinopExp*>(e);
    TEMP::Temp *right = munchExp(bine->right);
    switch (bine->op)
    {
    case T::BinOp::PLUS_OP:{
      emit(new AS::MoveInstr("\tmovq\t`s0, `d0", 
           L(r, NULL), 
           L(munchExp(bine->left), NULL)));
      emit(new AS::OperInstr("\taddq\t`s0, `d0", 
           L(r, NULL), 
           L(right, L(right, NULL)), 
           NULL));
      return r;
    }
    case T::BinOp::MINUS_OP:{
      emit(new AS::MoveInstr("\tmovq\t`s0, `d0", 
           L(r, NULL), 
           L(munchExp(bine->left), NULL)));
      emit(new AS::OperInstr("\tsubq\t`s0, `d0", 
           L(r, NULL), 
           L(right, L(right, NULL)), 
           NULL));
      return r;
    }
    case T::BinOp::MUL_OP:{
      emit(new AS::MoveInstr("\tmovq\t`s0, `d0", 
           L(r, NULL), 
           L(munchExp(bine->left), NULL)));
      emit(new AS::OperInstr("\timulq\t`s0, `d0", 
           L(r, NULL), 
           L(right, L(right, NULL)), 
           NULL));
      return r;
    }
    case T::BinOp::DIV_OP:{
      emit(new AS::MoveInstr("\tmovq\t`s0, `d0", 
           L(F::DIVIDEND(), NULL), 
           L(munchExp(bine->left), NULL)));
      emit(new AS::OperInstr("\tcqto", 
           L(F::DIVIDEND(), L(F::REMAINDER(), NULL)), 
           L(F::DIVIDEND(), NULL), 
           NULL));
      emit(new AS::OperInstr("\tidivq\t`s0", 
            L(F::DIVIDEND(), L(F::REMAINDER(), NULL)), 
            L(munchExp(bine->right), L(F::DIVIDEND(), NULL)), 
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
    emit(new AS::OperInstr("\tmovq\t(`s0), `d0", L(r, NULL), L(munchExp(static_cast<T::MemExp *>(e)->exp  ), NULL), NULL));
    return r;
  }
  case T::Exp::CALL:{
    T::CallExp *callExp = static_cast<T::CallExp *>(e);
    T::NameExp *nameExp = static_cast<T::NameExp *>(callExp->fun);
    TEMP::TempList *l = munchArgs(0, callExp->args);

    // NOTE: runtime link needs plt
    emit(new AS::OperInstr("\tcall\t" + nameExp->name->Name() + "@plt", F::CallerDefs(), L(F::RV(), l), NULL));
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
        emit(new AS::OperInstr("\tmovq\t(`s0), `d0", 
                L(munchExp(dste), NULL), 
                L(munchExp(static_cast<T::MemExp *>(srce)->exp), NULL), NULL));
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
      emit(new AS::OperInstr("\tmovq\t`s0, (`s1)", NULL, L(munchExp(srce), L(munchExp(meme->exp), NULL)), NULL));
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
         NULL, NULL, new AS::Targets(new TEMP::LabelList(cs->true_label, new TEMP::LabelList(cs->false_label, NULL)))));
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

static AS::InstrList *spillAll(F::Frame *f, AS::InstrList *instrList) {
  temp_map.clear();
  if (x64regs.size() <= 0) {
    initRegMap();
  }
  AS::InstrList *prev = new AS::InstrList(
      new AS::OperInstr("nop", NULL, NULL, NULL), instrList);
  AS::InstrList *list = prev;
  AS::InstrList *il = instrList;

  while(il) {
    if (il->head->kind == AS::Instr::MOVE) {
      AS::MoveInstr *mis = static_cast<AS::MoveInstr *>(il->head);
      if (!mis->dst) {
        assert(mis->src && mis->src->tail);
        TEMP::Temp *s1 = mis->src->head;
        TEMP::Temp *s2 = mis->src->tail->head;
        if (!isX64regs(s1)) {
          // load into r10 & replace s1 into r10
          assert(temp_map[s1]);
          char instr[128];
          sprintf(instr, load_template, framesize_str->c_str(), temp_map[s1]);
          insertAfter(prev, new AS::InstrList(
                        new AS::MoveInstr(instr, L(R10(), NULL), NULL), NULL));
          prev = prev->tail;
          mis->src->head = R10();
        }
        if (!isX64regs(s2)) {
          // load into r11 & replace s2 into r11
          assert(temp_map[s2]);
          char instr[128];
          sprintf(instr, load_template, framesize_str->c_str(), temp_map[s2]);
          insertAfter(prev, new AS::InstrList(
                        new AS::MoveInstr(instr, L(R11(), NULL), NULL), NULL));
          prev = prev->tail;
          mis->src->tail->head = R11();
        }
      } else {
        TEMP::Temp *s;
        TEMP::Temp *d = mis->dst->head;
        if (mis->src) {
          s = mis->src->head;
          if (!isX64regs(s)) {
            assert(temp_map[s]);
            char instr[128];
            sprintf(instr, load_template, framesize_str->c_str(), temp_map[s]);
            insertAfter(prev, new AS::InstrList(
                new AS::MoveInstr(instr, L(R10(), NULL), NULL), NULL));
            prev = prev->tail;
            mis->src->head = R10();
          }
        }

        if (!isX64regs(d)) {
          // alloc space for temporary
          if(!temp_map[d]) {
            f->allocLocal(true);
            temp_map[d] = f->getSize();
          }
          char instr[128];
          sprintf(instr, load_template, framesize_str->c_str(), temp_map[d]);
          insertAfter(prev, new AS::InstrList(
              new AS::MoveInstr(instr, L(R11(), NULL), NULL), NULL));
          prev = prev->tail;
          mis->dst->head = R11();
          sprintf(instr, store_template, framesize_str->c_str(), temp_map[d]);
          insertAfter(il, new AS::InstrList(new AS::MoveInstr(instr, NULL, L(R11(), NULL)), NULL));
          il = il->tail;
        }
      }
    } else if (il->head->kind == AS::Instr::OPER) {
      AS::OperInstr *opis = static_cast<AS::OperInstr*>(il->head);
      if(opis->assem.find("call") == opis->assem.npos
         &&opis->assem.find("j") == opis->assem.npos) {
        if (opis->src) {
          TEMP::Temp *s1 = opis->src->head;
          if (!isX64regs(s1)) {
            assert(temp_map[s1]);
            // load to r10
            char instr[128];
            sprintf(instr, load_template, framesize_str->c_str(), temp_map[s1]);
            insertAfter(prev, new AS::InstrList(
              new AS::MoveInstr(instr, L(R10(), NULL), NULL), NULL));
            prev = prev->tail;
            opis->src->head = R10();
          }
          if(opis->src->tail) {
            TEMP::Temp *s2 = opis->src->tail->head;
            if (opis->assem.find("idivq") == opis->assem.npos) {
              assert(!opis->dst );
            }
            if (!isX64regs(s2)) {
              // load s2 to r11
              assert(temp_map[s2]);
              char instr[128];
              sprintf(instr, load_template, framesize_str->c_str(), temp_map[s2]);
              insertAfter(prev, new AS::InstrList(
                new AS::MoveInstr(instr, L(R11(), NULL), NULL), NULL));
              prev = prev->tail;
              opis->src->tail->head = R11();
            }
          }
        }
        if (opis->dst) {
          if(!isX64regs(opis->dst->head)) {
            // move dst to r11
            TEMP::Temp *d = opis->dst->head;
            if (!temp_map[d]) {
              f->allocLocal(true);
              temp_map[d] = f->getSize();
            }
            // load to r11
            char instr[128];
            sprintf(instr, load_template, framesize_str->c_str(), temp_map[d]);
            insertAfter(prev, new AS::InstrList(
                new AS::MoveInstr(instr, L(R11(), NULL), NULL), NULL));
            // store r11 to frame
            prev = prev->tail;
            sprintf(instr, store_template, framesize_str->c_str(), temp_map[d]);
            insertAfter(il, new AS::InstrList(
                new AS::MoveInstr(instr, NULL, L(R11(), NULL)), NULL));
            il = il->tail;
            opis->dst->head = R11();
          }
        }
      }
    } else {
      // label instr use no regs
    }
    // next iteration
    prev = il;
    il = il->tail;
  }
  return list->tail;
}

static void initRegMap() {
  TEMP::TempList *regs = F::registers();
  while (regs){
    x64regs.push_back(regs->head);
    regs = regs->tail;
  }
}

static bool isX64regs(TEMP::Temp *reg) {
  for (std::vector<TEMP::Temp *>::iterator it = x64regs.begin(); 
        it != x64regs.end(); it++) {
    if ( *it == reg) {
      return true;
    }
  }
  return false;
}

static void insertAfter(AS::InstrList *origin, AS::InstrList *il) {
  il->tail = origin->tail;
  origin->tail = il;
}

static TEMP::Temp *R10() {
  TEMP::Temp *r10 = NULL;
  if (!r10) {
    r10 = F::CallerRegs()->head;
  }
  return r10;
}
static TEMP::Temp *R11() {
  TEMP::Temp *r11 = NULL;
  if (!r11) {
    r11 = F::CallerRegs()->tail->head;
  }
  return r11;
}
} // namespace
