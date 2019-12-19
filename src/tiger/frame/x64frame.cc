#include "tiger/frame/frame.h"

#include <string>

namespace   
{
  TEMP::Temp *RAX();
  TEMP::Temp *RBX();
  TEMP::Temp *RCX();
  TEMP::Temp *RDX();
  TEMP::Temp *RDI();
  TEMP::Temp *RSI();
  TEMP::Temp *RBP();
  TEMP::Temp *RSP();
  TEMP::Temp *R8();
  TEMP::Temp *R9();
  TEMP::Temp *R10();
  TEMP::Temp *R11();
  TEMP::Temp *R12();
  TEMP::Temp *R13();
  TEMP::Temp *R14();
  TEMP::Temp *R15();
  const int ARG_REG_NUM = 6;
  const int CALLEE_REG_NUM = 6;

  // Array of pointers which points to function that reurn TEMP::Temp *
  TEMP::Temp *(*argregs[ARG_REG_NUM])() = {RDI, RSI, RDX, RCX, R8, R9};
  TEMP::Temp *(*calleeregs[CALLEE_REG_NUM])() = {RBX, RBP, R12, R13, R14, R15};
} // namespace  

namespace F {
class Frame;
class InFrameAccess : public Access {
 public:
  int offset;

  InFrameAccess(int offset) : Access(INFRAME), offset(offset) {}

  T::Exp *ToExp(T::Exp *framePtr) const override {    
    return new T::MemExp(new T::BinopExp(
      T::BinOp::PLUS_OP, framePtr, new T::ConstExp(offset)
    ));
  }
};

class InRegAccess : public Access {
 public:
  TEMP::Temp* reg;

  InRegAccess(TEMP::Temp* reg) : Access(INREG), reg(reg) {}
  T::Exp *ToExp(T::Exp *framePtr) const override {
    return new T::TempExp(reg);
  }
};

class X64Frame : public Frame {
 public:
  TEMP::Label *label;
  AccessList *formals;
  AccessList *locals;
  T::SeqStm  *saveFormalStm;
  int size;

  X64Frame(TEMP::Label *name, U::BoolList *formalBools): Frame(){
    F::AccessList *accPtr = NULL;
    F::Access *acc;
    int cnt = 0;
    T::Exp *fp = new T::TempExp(F::FP());

    // alloc formals
    while (formalBools) {
      acc = allocLocal(formalBools->head);
      if (!accPtr) {
        formals = new F::AccessList(acc, NULL);
        accPtr = formals;
      } else {
        F::AccessList *al = new F::AccessList(acc, NULL);
        accPtr->tail = al;
        accPtr = al;
      }

      // Move escape parameters to Frame
      T::Stm *stm;
      if (cnt < ARG_REG_NUM) {
        stm = new T::MoveStm(
                acc->ToExp(fp), 
                new T::TempExp(argregs[cnt]()));
      } else {
        stm = new T::MoveStm(
                acc->ToExp(fp), 
                new T::BinopExp(
                  T::BinOp::PLUS_OP, 
                  new T::ConstExp((cnt - 5) * wordsize), 
                  fp));
      }

      // NOTE: order here make no sense
      if (formalBools->head) {
        if (!saveFormalStm) {
          saveFormalStm = new T::SeqStm(stm, new T::ExpStm(new T::ConstExp(0)));
        } else {
          saveFormalStm = new T::SeqStm(stm, saveFormalStm);
        }
      }
      formalBools = formalBools->tail;
      cnt++;
    }
  }
  T::SeqStm *getSaveEscFormalStm() const override {
    return saveFormalStm;
  }
  TEMP::Label *getName() const override {
    return label;
  }
  std::string getLabel() const override {
    return label->Name();
  }
  AccessList *getFormals() const override {
    return formals;
  }
  Access *allocLocal(bool escape) override {
    if (escape) {
      size += wordsize;
      return new InFrameAccess(-size);
    } else {
      return new InRegAccess(TEMP::Temp::NewTemp());
    }
  }
};

const int wordsize = 8;

TEMP::Temp *FP() {
  return RBP();
}
TEMP::Temp *RV() {
  return RAX();
}
TEMP::Temp *SP() {
  return RSP();
}
TEMP::Temp *DIVIDEND() {
  return RAX();
}
TEMP::Temp *REMAINDER() {
  return RDX();
}

TEMP::TempList *registers() {
  static TEMP::TempList *registers = NULL;
  if (!registers) {
    registers = new TEMP::TempList(RAX(), 
                new TEMP::TempList(RBX(), 
                new TEMP::TempList(RCX(), 
                new TEMP::TempList(RDX(), 
                new TEMP::TempList(RDI(), 
                new TEMP::TempList(RSI(), 
                new TEMP::TempList(R8(), 
                new TEMP::TempList(R9(), 
                new TEMP::TempList(R10(), 
                new TEMP::TempList(R11(), 
                new TEMP::TempList(R12(), 
                new TEMP::TempList(R13(), 
                new TEMP::TempList(R14(), 
                new TEMP::TempList(R15(), NULL))))))))))))));
  }
  return registers;
}

TEMP::TempList *SpecialRegs() {
  static TEMP::TempList *specialRegs = NULL;
  if (!specialRegs) {
    specialRegs = new TEMP::TempList(F::SP(),
                  new TEMP::TempList(F::RV(), nullptr));;
  }
  return specialRegs;
}

TEMP::TempList *CalleeRegs() {
  static TEMP::TempList *calleeSaves = NULL;
  if (!calleeSaves) {
    calleeSaves = new TEMP::TempList(RBX(),
                  new TEMP::TempList(RBP(),
                  new TEMP::TempList(R12(),
                  new TEMP::TempList(R13(),
                  new TEMP::TempList(R14(),
                  new TEMP::TempList(R15(), nullptr))))));
  }
  return calleeSaves;
}

TEMP::TempList *ArgRegs() {
  static TEMP::TempList *argRegs = NULL;
  if (!argRegs) {
    argRegs = new TEMP::TempList(RDI(),
              new TEMP::TempList(RSI(),
              new TEMP::TempList(RDX(),
              new TEMP::TempList(RCX(),
              new TEMP::TempList(R8(),
              new TEMP::TempList(R9(), nullptr))))));
  }
  return argRegs;
}

TEMP::TempList *CallerRegs() {
  static TEMP::TempList *callerSaves = NULL;
  if (!callerSaves){
    callerSaves = new TEMP::TempList(R10(),
                  new TEMP::TempList(R11(), nullptr));
  }
  return callerSaves;
}

// P168, args must provided by the caller
T::Exp *externalCall(std::string s, T::ExpList *args) {
  return new T::CallExp(
          new T::NameExp(
            TEMP::NamedLabel(s)),
            args);
}

Frame *newFrame(TEMP::Label *name, U::BoolList *formals) {
  return new X64Frame(name, formals);
}

StringFrag *String(TEMP::Label *lab, std::string str) {
  return new F::StringFrag(lab, str);
}

ProcFrag *NewProcFrag(T::Stm *body, Frame *frame) {
  return new F::ProcFrag(body, frame); 
}

T::Stm *procEntryExit1(Frame *frame, T::Stm *stm) {
  T::Stm *saveFormalStm = frame->getSaveEscFormalStm();
  if (!saveFormalStm) {
    saveFormalStm = new T::ExpStm(new T::ConstExp(0));
  }
  T::Exp *fp = new T::TempExp(F::FP());
  // save & restore callee save regs
  T::Stm *saveCalleeStm = NULL;
  T::Stm *restoreCalleeStm = NULL;
  for (int i = 0; i < CALLEE_REG_NUM; i++) {
    // alloc space for saving callee saved regs
    F::Access *acc = frame->allocLocal(true);
    
    // generate saving statements
    if (saveCalleeStm ) {
      saveCalleeStm = new T::SeqStm(saveCalleeStm, 
                        new T::MoveStm(
                          acc->ToExp(fp), 
                          new T::TempExp(calleeregs[i]())));
    } else {
      saveCalleeStm = new T::MoveStm(acc->ToExp(fp), new T::TempExp(calleeregs[i]()));
    }
    if (restoreCalleeStm) {
      restoreCalleeStm = new T::SeqStm(
                          restoreCalleeStm, 
                          new T::MoveStm(
                            new T::TempExp(calleeregs[i]()), 
                              acc->ToExp(fp)));
    } else {
      restoreCalleeStm = new T::MoveStm(new T::TempExp(calleeregs[i]()), acc->ToExp(fp));
    }
  }

  return new T::SeqStm(
          saveFormalStm, 
          new T::SeqStm(
            saveCalleeStm, 
            new T::SeqStm(
              stm, 
              restoreCalleeStm)));
}

AS::InstrList *procEntryExit2(AS::InstrList *body) {
  static TEMP::TempList *returnSink = NULL;
  if(!returnSink) {
    returnSink = new TEMP::TempList(RV(), 
                 new TEMP::TempList(SP(), CalleeRegs()));
  }
  return AS::InstrList::Splice(
          body, 
          new AS::InstrList(
            new AS::OperInstr("", NULL, returnSink, NULL), NULL));
}

// TODO: Add frame pointer adjust instructions
AS::Proc *procEntryExit3(Frame *frame, AS::InstrList *body) {
  char buf[100];
  sprintf(buf, "PROCEDURE %s\n", frame->getName()->Name().c_str());
  return new AS::Proc(buf, body, "END\n");
}
}  // namespace F

namespace   
{
  TEMP::Temp *RAX() {
    static TEMP::Temp *rax = NULL;
    if (!rax) {
      rax = TEMP::Temp::NewTemp();
    }
    return rax;
  }
  TEMP::Temp *RBX() {
    static TEMP::Temp *rbx = NULL;
    if (!rbx) {
      rbx = TEMP::Temp::NewTemp();
    }
    return rbx;
  }
  TEMP::Temp *RCX() {
    static TEMP::Temp *rcx = NULL;
    if (!rcx) {
      rcx = TEMP::Temp::NewTemp();
    }
    return rcx;
  }
  TEMP::Temp *RDX() {
    static TEMP::Temp *rdx = NULL;
    if (!rdx) {
      rdx = TEMP::Temp::NewTemp();
    }
    return rdx;
  }
  TEMP::Temp *RDI() {
    static TEMP::Temp *rdi = NULL;
    if (!rdi) {
      rdi = TEMP::Temp::NewTemp();
    }
    return rdi;
  }
  TEMP::Temp *RSI() {
    static TEMP::Temp *rsi = NULL;
    if (!rsi) {
      rsi = TEMP::Temp::NewTemp();
    }
    return rsi;
  }
  TEMP::Temp *RBP() {
    static TEMP::Temp *rbp = NULL;
    if (!rbp) {
      rbp = TEMP::Temp::NewTemp();
    }
    return rbp;
  }
  TEMP::Temp *RSP() {
    static TEMP::Temp *rsp = NULL;
    if (!rsp) {
      rsp = TEMP::Temp::NewTemp();
    }
    return rsp;
  }
  TEMP::Temp *R8() {
    static TEMP::Temp *r8 = NULL;
    if (!r8) {
      r8 = TEMP::Temp::NewTemp();
    }
    return r8;
  }
  TEMP::Temp *R9() {
    static TEMP::Temp *r9 = NULL;
    if (!r9) {
      r9 = TEMP::Temp::NewTemp();
    }
    return r9;
  }
  TEMP::Temp *R10() {
    static TEMP::Temp *r10 = NULL;
    if (!r10) {
      r10 = TEMP::Temp::NewTemp();
    }
    return r10;
  }
  TEMP::Temp *R11() {
    static TEMP::Temp *r11 = NULL;
    if (!r11) {
      r11 = TEMP::Temp::NewTemp();
    }
    return r11;
  }
  TEMP::Temp *R12() {
    static TEMP::Temp *r12 = NULL;
    if (!r12) {
      r12 = TEMP::Temp::NewTemp();
    }
    return r12;
  }
  TEMP::Temp *R13() {
    static TEMP::Temp *r13 = NULL;
    if (!r13) {
      r13 = TEMP::Temp::NewTemp();
    }
    return r13;
  }
  TEMP::Temp *R14() {
    static TEMP::Temp *r14 = NULL;
    if (!r14) {
      r14 = TEMP::Temp::NewTemp();
    }
    return r14;
  }
  TEMP::Temp *R15() {
    static TEMP::Temp *r15 = NULL;
    if (!r15) {
      r15 = TEMP::Temp::NewTemp();
    }
    return r15;
  }
} // namespace  