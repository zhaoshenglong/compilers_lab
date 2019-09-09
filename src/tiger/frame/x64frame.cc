#include "tiger/frame/frame.h"

#include <string>

namespace F {

class X64Frame : public Frame {
  // TODO: Put your codes here (lab6).
};

class InFrameAccess : public Access {
 public:
  int offset;

  InFrameAccess(int offset) : Access(INFRAME), offset(offset) {}
};

class InRegAccess : public Access {
 public:
  TEMP::Temp* reg;

  InRegAccess(TEMP::Temp* reg) : Access(INREG), reg(reg) {}
};

}  // namespace F