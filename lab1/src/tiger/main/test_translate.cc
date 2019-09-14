#include <cstdio>
#include <fstream>

#include "tiger/absyn/absyn.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/parse/parser.h"
#include "tiger/semant/semant.h"
#include "tiger/translate/translate.h"

extern EM::ErrorMsg errormsg;

A::Exp *absyn_root;
std::ifstream infile;

int main(int argc, char **argv) {
  int syntax_only = 1;

  if (argc < 2) {
    fprintf(stderr, "usage: a.out filename\n");
    exit(1);
  }

  if (argc == 3) syntax_only = atoi(argv[2]);

  errormsg.Reset(argv[1], infile);

  Parser parser(infile, std::cerr);
  parser.parse();
  F::FragList *flist = TR::TranslateProgram(absyn_root);
  int k = 0;
  for (; flist; flist = flist->tail) k++;
  fprintf(stdout, "%d\n", k);
  return 0;
}