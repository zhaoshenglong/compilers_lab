#include "tiger/frame/frame.h"

#include <string>

namespace F {

TEMP::Temp *FP() {
  return TEMP::Temp::NewTemp();
}

T::Exp *externalCall(std::string s, T::ExpList *args) {
  return new T::CallExp(
          new T::NameExp(
            TEMP::NamedLabel(s)),
            args);
}

class X64Frame : public Frame {

  // X64Frame members & methods
  Access *allocLocal(bool escape) {

  }
};

class InFrameAccess : public Access {
 public:
  int offset;

  InFrameAccess(int offset) : Access(INFRAME), offset(offset) {}

  T::Exp *ToExp(T::Exp *framePtr) {    
    return new T::MemExp(new T::BinopExp(
      T::BinOp::PLUS_OP, framePtr, new T::ConstExp(offset)
    ));
  }
};

class InRegAccess : public Access {
 public:
  TEMP::Temp* reg;

  InRegAccess(TEMP::Temp* reg) : Access(INREG), reg(reg) {}
  T::Exp *ToExp(T::Exp *framePtr) {
    return new T::TempExp(reg);
  }
};

}  // namespace F