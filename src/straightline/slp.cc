#include "straightline/slp.h"

namespace A {
int A::CompoundStm::MaxArgs() const {
  
  int args = 0;
  if (A::CompoundStm::stm1 != nullptr) {
    args = A::CompoundStm::stm1->MaxArgs();
  }
  if (A::CompoundStm::stm2 != nullptr) {
    int tmp = A::CompoundStm::stm2->MaxArgs();
    args = tmp > args ? tmp : args;
  }
  
  return args;
}

Table *A::CompoundStm::Interp(Table *t) const {
  Table * table = t;
  if (A::CompoundStm::stm1 != nullptr) {
    table = A::CompoundStm::stm1->Interp(table);
  } 
  if (A::CompoundStm::stm2 != nullptr) {
    table = A::CompoundStm::stm2->Interp(table);
  }
  return table;
}

int A::AssignStm::MaxArgs() const {
  return A::AssignStm::exp->MaxArgs();
}

Table *A::AssignStm::Interp(Table *t) const {
  if (!t) t = new A::Table(A::AssignStm::id, A::AssignStm::exp->Interp(nullptr)->i, t);
  else t = t->Update(A::AssignStm::id, A::AssignStm::exp->Interp(t)->i);
  
  return t;
}

int A::PrintStm::MaxArgs() const {
  int args = A::PrintStm::exps->MaxArgs();

  int tmp = A::PrintStm::exps->NumExps();
  return args > tmp ? args : tmp;
}

Table *A::PrintStm::Interp(Table *t) const {
  
  return A::PrintStm::exps->Interp(t)->t;
}

int A::IdExp::MaxArgs() const {
  return 0;
}
IntAndTable *A::IdExp::Interp(Table *t) const{ 
  return new IntAndTable(t->Lookup(A::IdExp::id), t);
}

int A::NumExp::MaxArgs() const {
  return 0;
}
IntAndTable *A::NumExp::Interp(Table *t) const{ 
  return new IntAndTable(A::NumExp::num, t);
}

int A::OpExp::MaxArgs() const {
  int args = 0;
  if(A::OpExp::left != nullptr) {
    args = A::OpExp::left->MaxArgs();
  }
  if (A::OpExp::right != nullptr){
    int tmp = A::OpExp::right->MaxArgs();
    args = tmp > args ? tmp : args;
  }

  return args;
}
IntAndTable *A::OpExp::Interp(Table *t) const{ 
  IntAndTable *leftIat, *rightIat;
  leftIat = A::OpExp::left->Interp(t);
  t = leftIat->t;
  rightIat = A::OpExp::right->Interp(t);
  t = rightIat->t;
  switch (A::OpExp::oper)
  {
  case A::PLUS:
    return new IntAndTable(
      leftIat->i + rightIat->i ,
       t);
  case A::MINUS:
    return new IntAndTable(
      leftIat->i - rightIat->i ,
      t);
  case A::TIMES:
    return new IntAndTable(
      leftIat->i * rightIat->i ,
      t);
  case A::DIV:
    return new IntAndTable(
      leftIat->i / rightIat->i ,
      t);
  default:
    break;
  }
  return nullptr;
}

int A::EseqExp::MaxArgs() const {
  int args = 0;
  if(A::EseqExp::stm != nullptr) {
    args = A::EseqExp::stm->MaxArgs();
  }
  if (A::EseqExp::exp != nullptr){
    int tmp = A::EseqExp::exp->MaxArgs();
    args = tmp > args ? tmp : args;
  }

  return args;
}
IntAndTable *A::EseqExp::Interp(Table *t) const{ 
  t = A::EseqExp::stm->Interp(t);
  return new IntAndTable(
    A::EseqExp::exp->Interp(t)->i,
    t
  );
}

int A::PairExpList::MaxArgs() const{
  int args = 0;
  if(A::PairExpList::head != nullptr) {
    args = A::PairExpList::head->MaxArgs();
  }
  if (A::PairExpList::tail != nullptr){
    int tmp = A::PairExpList::tail->MaxArgs();
    args = tmp > args ? tmp : args;
  }

  return args;
}

IntAndTable *A::PairExpList::Interp(Table *t) const {
  IntAndTable *iat = A::PairExpList::head->Interp(t);

  printf("%d ", iat->i);
  return new IntAndTable(iat -> i, A::PairExpList::tail->Interp(t)->t);
}

int A::PairExpList::NumExps() const {
  return 1 + A::PairExpList::tail->NumExps();
}

int A::LastExpList::MaxArgs() const{
  return A::LastExpList::last->MaxArgs();
}

IntAndTable *A::LastExpList::Interp(Table *t) const {
  IntAndTable *iat = A::LastExpList::last->Interp(t);
  printf("%d\n", iat->i);
  return new IntAndTable(iat->i, t);
}
int A::LastExpList::NumExps() const {
  return 1;
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
