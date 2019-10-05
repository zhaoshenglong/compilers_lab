#include "tiger/errormsg/errormsg.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <fstream>

EM::ErrorMsg errormsg;
namespace EM {

void ErrorMsg::Newline() {
  lineNum++;
  linePos = new IntList(tokPos, linePos);
}

void ErrorMsg::Error(int pos, std::string message, ...) {
  va_list ap;
  IntList *lines = linePos;
  int num = lineNum;

  anyErrors = true;
  while (lines && lines->i >= pos) {
    lines = lines->rest;
    num--;
  }

  if (!fileName.empty()) fprintf(stderr, "%s:", fileName.c_str());
  if (lines) fprintf(stderr, "%d.%d: ", num, pos - lines->i);
  va_start(ap, message);
  vfprintf(stderr, message.c_str(), ap);
  va_end(ap);
  fprintf(stderr, "\n");
}

void ErrorMsg::Reset(std::string fname, std::ifstream &infile) {
  anyErrors = false;
  fileName = fname;
  lineNum = 1;
  tokPos = 1;
  linePos = new IntList(0, nullptr);
  infile.open(fileName);
  if (!infile.good()) {
    Error(0, "cannot open");
    exit(1);
  }
}

}  // namespace EM
