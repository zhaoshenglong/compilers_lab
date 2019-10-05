#include "tiger/escape/escape.h"

namespace ESC {

class EscapeEntry {
 public:
  int depth;
  bool* escape;

  EscapeEntry(int depth, bool* escape) : depth(depth), escape(escape) {}
};

void FindEscape(A::Exp* exp) {
  // TODO: Put your codes here (lab6).
}

}  // namespace ESC