#include <cstdio>
#include <fstream>

#include "tiger/absyn/absyn.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/parse/parser.h"

extern EM::ErrorMsg errormsg;

A::Exp *absyn_root;
std::ifstream infile;

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: a.out filename\n");
    exit(1);
  }

  errormsg.Reset(argv[1], infile);

  Parser parser(infile, std::cerr);
  parser.parse();
  absyn_root->Print(stderr, 0);
  fprintf(stderr, "\n");
  return 0;
}