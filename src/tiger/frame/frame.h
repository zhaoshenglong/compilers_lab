#ifndef TIGER_FRAME_FRAME_H_
#define TIGER_FRAME_FRAME_H_

#include <string>

#include "tiger/codegen/assem.h"
#include "tiger/translate/tree.h"
#include "tiger/util/util.h"


namespace F {
extern const int wordsize;
TEMP::Temp *FP();

T::Exp *externalCall(std::string s, T::ExpList * args);

class Frame {
  // Base class
 public:
  TEMP::Label name;
  AccessList *formals;
  int size;

  Frame(TEMP::Label name, U::BoolList *f) : name(name){}
  /* Frame instance members & methods */
  TEMP::Label name();
  std::string getlabel();
  AccessList getFormals();

  /* Frame children members & methods */
  virtual Access *allocLocal(bool escape) const = 0;
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