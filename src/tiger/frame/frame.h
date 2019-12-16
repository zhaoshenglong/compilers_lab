#ifndef TIGER_FRAME_FRAME_H_
#define TIGER_FRAME_FRAME_H_

#include <string>

#include "tiger/codegen/assem.h"
#include "tiger/translate/tree.h"
#include "tiger/util/util.h"


namespace F {

extern const int wordsize;
TEMP::Temp *FP();
TEMP::Temp *RV();
TEMP::Temp *SP();
TEMP::Temp *DIVIDEND();   // special regs for division in X86_64, rax
TEMP::Temp *REMAINDER();  // special regs for division in X86_64, rdx
TEMP::TempList *registers();
TEMP::TempList *SpecialRegs();
TEMP::TempList *CalleeRegs();
TEMP::TempList *ArgRegs();
TEMP::TempList *CallerRegs();

TEMP::Map *tempMap();

F::Frame *newFrame(TEMP::Label *, U::BoolList *);
T::Exp *externalCall(std::string s, T::ExpList * args);
F::StringFrag *String(TEMP::Label *lab, std::string str);
F::ProcFrag *NewProcFrag(T::Stm *body, F::Frame *frame);

T::Stm *procEntryExit1(F::Frame *frame, T::Stm *stm);
AS::InstrList *procEntryExit2(AS::InstrList *body);
AS::Proc *procEntryExit3(F::Frame *frame, AS::InstrList *body);

class Frame {
  // Base class
 public:
  TEMP::Label *label;
  AccessList *formals;
  AccessList *locals;
  T::SeqStm  *saveFormalStm;
  int size;

  Frame(TEMP::Label *name, U::BoolList *f) : label(name){};

  virtual T::Stm *getSaveEscFormalStm() const = 0;
  virtual TEMP::Label *getName() const = 0;
  virtual std::string getLabel() const = 0;
  virtual AccessList *getFormals() const = 0;
  virtual Access *allocLocal(bool escape);
};

class Access {
 public:
  enum Kind { INFRAME, INREG };

  Kind kind;

  Access(Kind kind) : kind(kind) {}

  // Hints: You may add interface like
  //        `virtual T::Exp* ToExp(T::Exp* framePtr) const = 0`
  virtual T::Exp* ToExp(T::Exp* framePtr) const = 0;
};



class AccessList {
 public:
  Access *head;
  AccessList *tail;

  AccessList(Access *head, AccessList *tail) : head(head), tail(tail) {}
};

/*
 * Fragments
 */

class Frag {
 public:
  enum Kind { STRING, PROC };

  Kind kind;

  Frag(Kind kind) : kind(kind) {}
};

class StringFrag : public Frag {
 public:
  TEMP::Label *label;
  std::string str;

  StringFrag(TEMP::Label *label, std::string str)
      : Frag(STRING), label(label), str(str) {}
};

class ProcFrag : public Frag {
 public:
  T::Stm *body;
  Frame *frame;

  ProcFrag(T::Stm *body, Frame *frame) : Frag(PROC), body(body), frame(frame) {}
};

class FragList {
 public:
  Frag *head;
  FragList *tail;

  FragList(Frag *head, FragList *tail) : head(head), tail(tail) {}
};

}  // namespace F

#endif