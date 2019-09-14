#include "straightline/slp.h"

namespace A {
int A::CompoundStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return 0;
}

Table *A::CompoundStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  return nullptr;
}

int A::AssignStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return 0;
}

Table *A::AssignStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  return nullptr;
}

int A::PrintStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return 0;
}

Table *A::PrintStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  return nullptr;
}

int Table::Lookup(std::string key) const {
  if (id == key) {
    return value;
  } else if (tail != nullptr) {
    return tail->Lookup(key);
  } else {
    assert(false);
  }
}

Table *Table::Update(std::string key, int value) const {
  return new Table(key, value, this);
}
}  // namespace A
