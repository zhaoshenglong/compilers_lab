#include "tiger/symbol/symbol.h"

namespace {

const unsigned int HASH_TABSIZE = 109;
S::Symbol *hashtable[HASH_TABSIZE];

unsigned int hash(std::string &str) {
  unsigned int h = 0;
  for (const char *s = str.c_str(); *s; s++) h = h * 65599 + *s;
  return h;
}

}  // namespace

namespace S {

Symbol *Symbol::UniqueSymbol(std::string name) {
  int index = hash(name) % HASH_TABSIZE;
  Symbol *syms = hashtable[index], *sym;
  for (sym = syms; sym; sym = sym->next)
    if (sym->name == name) return sym;
  sym = new Symbol(name, syms);
  hashtable[index] = sym;
  return sym;
}

}  // namespace S
