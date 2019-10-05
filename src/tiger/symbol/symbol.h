#ifndef TIGER_SYMBOL_SYMBOL_H_
#define TIGER_SYMBOL_SYMBOL_H_

#include <string>
#include "tiger/util/table.h"

namespace S {
class Symbol {
  template <typename ValueType>
  friend class Table;

 public:
  static Symbol *UniqueSymbol(std::string);
  const std::string &Name() const { return name; }

 private:
  Symbol() {}
  Symbol(std::string name, Symbol *next) : name(name), next(next) {}

  std::string name;
  Symbol *next;
};

template <typename ValueType>
class Table : public TAB::Table<Symbol, ValueType> {
 public:
  Table() : TAB::Table<Symbol, ValueType>() {}
  void BeginScope();
  void EndScope();

 private:
  Symbol marksym = {"<mark>", nullptr};
};

template <typename ValueType>
void Table<ValueType>::BeginScope() {
  this->Enter(&marksym, nullptr);
}

template <typename ValueType>
void Table<ValueType>::EndScope() {
  Symbol *s;
  do
    s = this->Pop();
  while (s != &marksym);
}

};  // namespace S

#endif  // TIGER_SYMBOL_SYMBOL_H_
