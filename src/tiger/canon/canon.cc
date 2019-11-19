#include "tiger/canon/canon.h"

namespace {

S::Table<T::StmList>* block_env;
C::Block global_block;

C::StmAndExp do_exp(T::Exp* exp);
C::StmListList* mk_blocks(T::StmList* stms, TEMP::Label* done);
T::StmList* get_next();

bool is_nop(T::Stm* x) {
  //[TODO] remove judgement using "kind" property
  if (x->kind == T::Stm::Kind::EXP) {
    return dynamic_cast<T::ExpStm*>(x)->exp->kind == T::Exp::Kind::CONST;
  } else
    return false;
}

T::Stm* seq(T::Stm* x, T::Stm* y) {
  if (is_nop(x)) return y;
  if (is_nop(y)) return x;
  return new T::SeqStm(x, y);
}

bool commute(T::Stm* x, T::Exp* y) {
  if (is_nop(x)) return true;
  if (y->kind == T::Exp::Kind::NAME || y->kind == T::Exp::Kind::CONST)
    return true;
  return false;
}

T::Stm* reorder(C::ExpRefList* rlist) {
  if (!rlist)
    return new T::ExpStm(new T::ConstExp(0)); /* nop */
  else if ((*rlist->head)->kind == T::Exp::Kind::CALL) {
    TEMP::Temp* t = TEMP::Temp::NewTemp();
    *rlist->head = new T::EseqExp(
        new T::MoveStm(new T::TempExp(t), *rlist->head), new T::TempExp(t));
    return reorder(rlist);
  } else {
    C::StmAndExp hd = do_exp(*rlist->head);
    T::Stm* s = reorder(rlist->tail);
    if (commute(s, hd.e)) {
      *rlist->head = hd.e;
      return seq(hd.s, s);
    } else {
      TEMP::Temp* t = TEMP::Temp::NewTemp();
      *rlist->head = new T::TempExp(t);
      return seq(hd.s, seq(new T::MoveStm(new T::TempExp(t), hd.e), s));
    }
  }
}

C::ExpRefList* get_call_rlist(T::Exp* exp) {
  C::ExpRefList *rlist, *curr;
    T::CallExp* callexp = dynamic_cast<T::CallExp*>(exp);
    if(!callexp) assert(0);
    T::ExpList* args = callexp->args;
    curr = rlist = new C::ExpRefList(&callexp->fun, nullptr);
    for (;args; args=args->tail) {
        curr = curr->tail = new C::ExpRefList(&args->head, nullptr);
    }
    return rlist;
}

/* processes stm so that it contains no ESEQ nodes */
T::Stm* do_stm(T::Stm* stm) { return stm->Canon(stm); }

C::StmAndExp do_exp(T::Exp* exp) { return exp->Canon(exp); }

/* linear gets rid of the top-level SEQ's, producing a list */
T::StmList* linear(T::Stm* stm, T::StmList* right) {
  if (stm->kind == T::Stm::Kind::SEQ) {
    T::SeqStm* seqstm = dynamic_cast<T::SeqStm*>(stm);
    if (!seqstm) assert(0);
    return linear(seqstm->left, linear(seqstm->right, right));
  } else
    return new T::StmList(stm, right);
}

/* Go down a list looking for end of basic block */
C::StmListList* next(T::StmList* prevstms, T::StmList* stms,
                     TEMP::Label* done) {
  if (!stms)
    return next(
        prevstms,
        new T::StmList(new T::JumpStm(new T::NameExp(done),
                                      new TEMP::LabelList(done, nullptr)),
                       nullptr),
        done);
  if (stms->head->kind == T::Stm::Kind::JUMP ||
      stms->head->kind == T::Stm::Kind::CJUMP) {
    C::StmListList* stmLists;
    prevstms->tail = stms;
    stmLists = mk_blocks(stms->tail, done);
    stms->tail = nullptr;
    return stmLists;
  } else if (stms->head->kind == T::Stm::Kind::LABEL) {
    T::LabelStm* labelstm = dynamic_cast<T::LabelStm*>(stms->head);
    if (!labelstm) assert(0);
    TEMP::Label* lab = labelstm->label;
    return next(
        prevstms,
        new T::StmList(new T::JumpStm(new T::NameExp(lab),
                                      new TEMP::LabelList(lab, nullptr)),
                       stms),
        done);
  } else {
    prevstms->tail = stms;
    return next(stms, stms->tail, done);
  }
}

/* Create the beginning of a basic block */
C::StmListList* mk_blocks(T::StmList* stms, TEMP::Label* done) {
  if (!stms) {
    return nullptr;
  }
  if (stms->head->kind != T::Stm::Kind::LABEL) {
    return mk_blocks(new T::StmList(new T::LabelStm(TEMP::NewLabel()), stms),
                     done);
  }
  /* else there already is a label */
  return new C::StmListList(stms, next(stms, stms->tail, done));
}

T::StmList* get_last(T::StmList* list) {
  T::StmList* last = list;
  while (last->tail->tail) last = last->tail;
  return last;
}

void trace(T::StmList* list) {
  T::StmList* last = get_last(list);
  T::LabelStm* lab = dynamic_cast<T::LabelStm*>(list->head);
  if (!lab) assert(0);
  T::Stm* s = last->tail->head;
  block_env->Enter(lab->label, nullptr);
  if (s->kind == T::Stm::Kind::JUMP) {
    T::JumpStm* jumpstm = dynamic_cast<T::JumpStm*>(s);
    if (!jumpstm) assert(0);
    T::StmList* target = (T::StmList*)block_env->Look(jumpstm->jumps->head);
    if (!jumpstm->jumps->tail && target) {
      last->tail = target; /* merge the 2 lists removing JUMP stm */
      trace(target);
    } else
      last->tail->tail = get_next(); /* merge and keep JUMP stm */
  }
  /* we want false label to follow CJUMP */
  else if (s->kind == T::Stm::Kind::CJUMP) {
    T::CjumpStm* cjumpstm = dynamic_cast<T::CjumpStm*>(s);
    if (!cjumpstm) assert(0);
    T::StmList* truelist = (T::StmList*)block_env->Look(cjumpstm->true_label);
    T::StmList* falselist = (T::StmList*)block_env->Look(cjumpstm->false_label);
    if (falselist) {
      last->tail->tail = falselist;
      trace(falselist);
    } else if (truelist) { /* convert so that existing label is a false label */
      last->tail->head = new T::CjumpStm(
          T::notRel(cjumpstm->op), cjumpstm->left, cjumpstm->right,
          cjumpstm->false_label, cjumpstm->true_label);
      last->tail->tail = truelist;
      trace(truelist);
    } else {
      TEMP::Label* falselabel = TEMP::NewLabel();
      last->tail->head =
          new T::CjumpStm(cjumpstm->op, cjumpstm->left, cjumpstm->right,
                          cjumpstm->true_label, falselabel);
      last->tail->tail = new T::StmList(
          new T::LabelStm(falselabel),
          new T::StmList(new T::JumpStm(new T::NameExp(cjumpstm->false_label),
                                        new TEMP::LabelList(
                                            cjumpstm->false_label, nullptr)),
                         get_next()));
    }
  } else
    assert(0);
}

/* get the next block from the list of stmLists, using only those that have
 * not been traced yet */
T::StmList* get_next() {
  if (!global_block.stmLists)
    return new T::StmList(new T::LabelStm(global_block.label), nullptr);
  else {
    T::StmList* s = global_block.stmLists->head;
    T::LabelStm* lab = dynamic_cast<T::LabelStm*>(s->head);
    if (!lab) assert(0);
    if (block_env->Look(lab->label)) { /* label exists in the table */
      trace(s);
      return s;
    } else {
      global_block.stmLists = global_block.stmLists->tail;
      return get_next();
    }
  }
}

}  // namespace

namespace C {

/* From an arbitrary Tree statement, produce a list of cleaned trees
   satisfying the following properties:
      1.  No SEQ's or ESEQ's
      2.  The parent of every CALL is an EXP(..) or a MOVE(TEMP t,..) */
T::StmList* Linearize(T::Stm* stm) { return linear(do_stm(stm), nullptr); }

/* basicBlocks : Tree.stm list -> (Tree.stm list list * Tree.label)
       From a list of cleaned trees, produce a list of
 basic blocks satisfying the following properties:
      1. and 2. as above;
      3.  Every block begins with a LABEL;
      4.  A LABEL appears only at the beginning of a block;
      5.  Any JUMP or CJUMP is the last stm in a block;
      6.  Every block ends with a JUMP or CJUMP;
   Also produce the "label" to which control will be passed
   upon exit.
*/
Block BasicBlocks(T::StmList* stmList) {
  Block b;
  b.label = TEMP::NewLabel();
  b.stmLists = mk_blocks(stmList, b.label);

  return b;
}

/* traceSchedule : Tree.stm list list * Tree.label -> Tree.stm list
   From a list of basic blocks satisfying properties 1-6,
   along with an "exit" label,
   produce a list of stms such that:
     1. and 2. as above;
     7. Every CJUMP(_,t,f) is immediately followed by LABEL f.
   The blocks are reordered to satisfy property 7; also
   in this reordering as many JUMP(T.NAME(lab)) statements
   as possible are eliminated by falling through into T.LABEL(lab).
*/
T::StmList* TraceSchedule(Block b) {
  StmListList* sList;
  block_env = new S::Table<T::StmList>();
  global_block = b;

  for (sList = global_block.stmLists; sList; sList = sList->tail) {
    T::LabelStm* lab = dynamic_cast<T::LabelStm*>(sList->head->head);
    if (!lab) assert(0);
    block_env->Enter(lab->label, sList->head);
  }

  return get_next();
}

}  // namespace C

namespace T {

/* processes stm so that it contains no ESEQ nodes */
Stm* SeqStm::Canon(Stm* self) {
  return seq(do_stm(this->left), do_stm(this->right));
}

Stm* LabelStm::Canon(Stm* self) { return self; }

Stm* JumpStm::Canon(Stm* self) {
  return seq(reorder(new C::ExpRefList((T::Exp**)&this->exp, nullptr)), self);
}

Stm* CjumpStm::Canon(Stm* self) {
  return seq(reorder(new C::ExpRefList(
                 &this->left, new C::ExpRefList(&this->right, nullptr))),
             self);
}

Stm* MoveStm::Canon(Stm* self) {
  if (this->dst->kind == Exp::Kind::TEMP && this->src->kind == Exp::Kind::CALL)
    return seq(reorder(get_call_rlist(this->src)), self);
  else if (this->dst->kind == Exp::Kind::TEMP)
    return seq(reorder(new C::ExpRefList(&this->src, nullptr)), self);
  else if (this->dst->kind == Exp::Kind::MEM) {
    MemExp* memexp = dynamic_cast<MemExp*>(this->dst);
    if (!memexp) assert(0);
    return seq(reorder(new C::ExpRefList(
                   &memexp->exp, new C::ExpRefList(&this->src, nullptr))),
               self);
  } else if (this->dst->kind == Exp::Kind::ESEQ) {
    EseqExp* eseqexp = dynamic_cast<EseqExp*>(this->dst);
    if (!eseqexp) assert(0);
    Stm* s = eseqexp->stm;
    this->dst = eseqexp->exp;
    return do_stm(new SeqStm(s, self));
  }
  assert(0); /* dst should be temp or mem only */
}

Stm* ExpStm::Canon(Stm* self) {
  if (this->exp->kind == Exp::Kind::CALL)
    return seq(reorder(get_call_rlist(this->exp)), self);
  else
    return seq(reorder(new C::ExpRefList(&this->exp, nullptr)), self);
}

C::StmAndExp BinopExp::Canon(Exp* self) {
  return C::StmAndExp(
      reorder(new C::ExpRefList(&this->left,
                                new C::ExpRefList(&this->right, nullptr))),
      self);
}

C::StmAndExp MemExp::Canon(Exp* self) {
  return C::StmAndExp(reorder(new C::ExpRefList(&this->exp, nullptr)), self);
}

C::StmAndExp TempExp::Canon(Exp* self) {
  return C::StmAndExp(reorder(nullptr), self);
}

C::StmAndExp EseqExp::Canon(Exp* self) {
  C::StmAndExp x = do_exp(this->exp);
  return C::StmAndExp(seq(do_stm(this->stm), x.s), x.e);
}

C::StmAndExp NameExp::Canon(Exp* self) {
  return C::StmAndExp(reorder(nullptr), self);
}

C::StmAndExp ConstExp::Canon(Exp* self) {
  return C::StmAndExp(reorder(nullptr), self);
}

C::StmAndExp CallExp::Canon(Exp* self) {
  return C::StmAndExp(reorder(get_call_rlist(self)), self);
}

}  // namespace T