#ifndef TIGER_ERRORMSG_ERROMSG_H_
#define TIGER_ERRORMSG_ERROMSG_H_

#include <fstream>
#include <string>

namespace EM {
class ErrorMsg {
 public:
  class IntList {
   public:
    int i;
    IntList *rest;

    IntList(int i, IntList *rest) : i(i), rest(rest){};
  };

  void Newline();
  void Error(int, std::string, ...);
  void Reset(std::string, std::ifstream &);

  bool anyErrors = false;
  int tokPos;
  int lineNum;
  IntList *linePos;

  std::string fileName;
};
};  // namespace EM

#endif  // TIGER_ERRORMSG_ERROMSG_H_
