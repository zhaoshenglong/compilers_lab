#ifndef TIGER_PARSE_PARSER_H_
#define TIGER_PARSE_PARSER_H_

#include <list>

#include "tiger/errormsg/errormsg.h"
#include "tiger/lex/scanner.h"
#include "tiger/parse/parserbase.h"
#include "tiger/symbol/symbol.h"

#undef Parser

extern A::Exp *absyn_root;
extern EM::ErrorMsg errormsg;

class Parser : public ParserBase {
  Scanner d_scanner;

 public:
  Parser() = default;
  Parser(std::istream &in = std::cin, std::ostream &out = std::cout)
      : d_scanner(in, out) {}
  int parse();

 private:
  std::list<std::string> string_pool_;

  void error();
  int lex();
  void print();
  void exceptionHandler(std::exception const &exc);

  void executeAction__(int ruleNr);
  void errorRecovery__();
  void nextCycle__();
  void nextToken__();
  void print__();
};

#endif  // TIGER_PARSE_PARSER_H_
