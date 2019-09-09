#include "tiger/codegen/assem.h"

namespace {

TEMP::Temp* nth_temp(TEMP::TempList* list, int i) {
  assert(list);
  if (i == 0)
    return list->head;
  else
    return nth_temp(list->tail, i - 1);
}

TEMP::Label* nth_label(TEMP::LabelList* list, int i) {
  assert(list);
  if (i == 0)
    return list->head;
  else
    return nth_label(list->tail, i - 1);
}

}  // namespace

namespace AS {
/* first param is string created by this function by reading 'assem' string
 * and replacing `d `s and `j stuff.
 * Last param is function to use to determine what to do with each temp.
 */
static std::string format(std::string assem, TEMP::TempList* dst,
                          TEMP::TempList* src, Targets* jumps, TEMP::Map* m) {
  std::string result;
  for (int i = 0; i < assem.size(); i++) {
    char ch = assem.at(i);
    if (ch == '`') {
      i++;
      switch (assem.at(i)) {
        case 's': {
          i++;
          int n = assem.at(i) - '0';
          std::string* s = m->Look(nth_temp(src, n));
          result += *s;
        } break;
        case 'd': {
          i++;
          int n = assem.at(i) - '0';
          std::string* s = m->Look(nth_temp(dst, n));
          result += *s;
        } break;
        case 'j': {
          i++;
          assert(jumps);
          int n = assem.at(i) - '0';
          std::string s = TEMP::LabelString(nth_label(jumps->labels, n));
          result += s;
        } break;
        case '`': {
          result += '`';
        } break;
        default:
          assert(0);
      }
    } else {
      result += ch;
    }
  }
  return result;
}

void OperInstr::Print(FILE* out, TEMP::Map* m) const {
  std::string result =
      format(this->assem, this->dst, this->src, this->jumps, m);
  fprintf(out, "%s\n", result.c_str());
}

void LabelInstr::Print(FILE* out, TEMP::Map* m) const {
  std::string result = format(this->assem, nullptr, nullptr, nullptr, m);
  fprintf(out, "%s:\n", result.c_str());
}

void MoveInstr::Print(FILE* out, TEMP::Map* m) const {
  if ((this->dst == nullptr) && (this->src == nullptr)) {
    std::size_t srcpos = this->assem.find_first_of('%');
    if (srcpos != std::string::npos) {
      std::size_t dstpos = this->assem.find_first_of('%', srcpos + 1);
      if (dstpos != std::string::npos) {
        if ((this->assem[srcpos + 1] == this->assem[dstpos + 1]) &&
            (this->assem[srcpos + 2] == this->assem[dstpos + 2]) &&
            (this->assem[srcpos + 3] == this->assem[dstpos + 3]))
          return;
      }
    }
  }
  std::string result = format(this->assem, this->dst, this->src, nullptr, m);
  fprintf(out, "%s\n", result.c_str());
}

void InstrList::Print(FILE* out, TEMP::Map* m) const {
  const InstrList* p = this;
  for (; p; p = p->tail) {
    p->head->Print(out, m);
  }
  fprintf(out, "\n");
}

/* put list b at the end of list a */
InstrList* InstrList::Splice(InstrList* a, InstrList* b) {
  InstrList* p;
  if (a == nullptr) return b;
  for (p = a; p->tail != nullptr; p = p->tail)
    ;
  p->tail = b;
  return a;
}

}  // namespace AS