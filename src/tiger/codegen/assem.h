#ifndef TIGER_CODEGEN_ASSEM_H_
#define TIGER_CODEGEN_ASSEM_H_

#include <cstdio>
#include <string>

#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"

namespace AS {

class Targets {
 public:
  TEMP::LabelList* labels;

  Targets(TEMP::LabelList* labels) : labels(labels) {}
};

class Instr {
 public:
  enum Kind { OPER, LABEL, MOVE };

  Kind kind;

  Instr(Kind kind) : kind(kind) {}

  virtual void Print(FILE* out, TEMP::Map* m) const = 0;
};

class OperInstr : public Instr {
 public:
  std::string assem;
  TEMP::TempList *dst, *src;
  Targets* jumps;

  OperInstr(std::string assem, TEMP::TempList* dst, TEMP::TempList* src,
            Targets* jumps)
      : Instr(OPER), assem(assem), dst(dst), src(src), jumps(jumps) {}

  void Print(FILE* out, TEMP::Map* m) const override;
};

class LabelInstr : public Instr {
 public:
  std::string assem;
  TEMP::Label* label;

  LabelInstr(std::string assem, TEMP::Label* label)
      : Instr(LABEL), assem(assem), label(label) {}

  void Print(FILE* out, TEMP::Map* m) const override;
};

class MoveInstr : public Instr {
 public:
  std::string assem;
  TEMP::TempList *dst, *src;

  MoveInstr(std::string assem, TEMP::TempList* dst, TEMP::TempList* src)
      : Instr(MOVE), assem(assem), dst(dst), src(src) {}

  void Print(FILE* out, TEMP::Map* m) const override;
};

class InstrList {
 public:
  Instr* head;
  InstrList* tail;

  InstrList(Instr* head, InstrList* tail) : head(head), tail(tail) {}

  void Print(FILE* out, TEMP::Map* m) const;

  static InstrList* Splice(InstrList* a, InstrList* b);
};

class Proc {
 public:
  std::string prolog;
  InstrList* body;
  std::string epilog;

  Proc(std::string prolog, InstrList* body, std::string epilog)
      : prolog(prolog), body(body), epilog(epilog) {}
};

}  // namespace AS

#endif