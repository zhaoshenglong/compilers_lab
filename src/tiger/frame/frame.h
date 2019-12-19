#ifndef TIGER_FRAME_FRAME_H_
#define TIGER_FRAME_FRAME_H_

#include <string>

#include "tiger/codegen/assem.h"
#include "tiger/translate/tree.h"
#include "tiger/util/util.h"

// forward declarations

namespace AS{
  class InstrList;
  class Proc;
} // namespace AS;

namespace F {
class Frame;
class Frag;
class StringFrag;
class ProcFrag;
class FragList;
class Access;
class AccessList;

extern const int wordsize;
TEMP::Temp *FP();
TEMP::Temp *RV();
TEMP::Temp *SP();
TEMP::Temp *DIVIDEND();   // special regs for division in X86_64, %rax
TEMP::Temp *REMAINDER();  // special regs for division in X86_64, %rdx
TEMP::TempList *registers();  
TEMP::TempList *SpecialRegs();  // rsp, rax
TEMP::TempList *CalleeRegs();   // rbp, rbx, r12-r15
TEMP::TempList *ArgRegs();      // rdi, rsi, rdx, rcx, r8, r9
TEMP::TempList *CallerRegs();   // r10, r11

// TODO: implement temp map
TEMP::Map *tempMap();         

F::Frame *newFrame(TEMP::Label *, U::BoolList *); 
T::Exp *externalCall(std::string s, T::ExpList * args);   // call external func, no static links
F::StringFrag *String(TEMP::Label *lab, std::string str); // create new string fragment(useless)
F::ProcFrag *NewProcFrag(T::Stm *body, F::Frame *frame);  // create new proc fragment

T::Stm *procEntryExit1(F::Frame *frame, T::Stm *stm);     // P171, 268: 4. 5. (6. 7.) 8
AS::InstrList *procEntryExit2(AS::InstrList *body);       // P215
AS::Proc *procEntryExit3(F::Frame *frame, AS::InstrList *body); // P171, P216, P269: 1.2.3.9.10.11

class Frame {
  // Base class
 public:
  TEMP::Label *label;
  AccessList *formals;    // saved parameters
  AccessList *locals;     // saved local variables
  T::SeqStm  *saveFormalStm;  
  int size;               // frame size
  std::string *framesize_str;
  Frame(TEMP::Label *name, U::BoolList *formals) : label(name) {} 
  virtual T::SeqStm *getSaveEscFormalStm() const = 0;
  virtual TEMP::Label *getName() const = 0;
  virtual std::string getLabel() const = 0;
  virtual AccessList *getFormals() const = 0;
  virtual Access *allocLocal(bool escape) = 0;
  virtual std::string *getFramesizeStr() const = 0;
};

class Access {
 public:
  enum Kind { INFRAME, INREG };

  Kind kind;

  Access(Kind kind) : kind(kind) {}

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