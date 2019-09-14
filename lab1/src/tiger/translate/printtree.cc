#include "tiger/translate/printtree.h"

#include <cstdio>

namespace T {

void PrintStmList(FILE* out, StmList* stmList) {
  for (; stmList; stmList = stmList->tail) {
    stmList->head->Print(out, 0);
    fprintf(out, "\n");
  }
}

}  // namespace T