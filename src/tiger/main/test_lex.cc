#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <string>

#include "tiger/absyn/absyn.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/lex/scanner.h"

extern EM::ErrorMsg errormsg;
std::ifstream infile;
A::Exp *absyn_root;

namespace {
std::map<int, std::string> tokname = {{Parser::ID, "ID"},
                                      {Parser::STRING, "STRING"},
                                      {Parser::INT, "INT"},
                                      {Parser::COMMA, "COMMA"},
                                      {Parser::COLON, "COLON"},
                                      {Parser::SEMICOLON, "SEMICOLON"},
                                      {Parser::LPAREN, "LPAREN"},
                                      {Parser::RPAREN, "RPAREN"},
                                      {Parser::LBRACK, "LBRACK"},
                                      {Parser::RBRACK, "RBRACK"},
                                      {Parser::LBRACE, "LBRACE"},
                                      {Parser::RBRACE, "RBRACE"},
                                      {Parser::DOT, "DOT"},
                                      {Parser::PLUS, "PLUS"},
                                      {Parser::MINUS, "MINUS"},
                                      {Parser::TIMES, "TIMES"},
                                      {Parser::DIVIDE, "DIVIDE"},
                                      {Parser::EQ, "EQ"},
                                      {Parser::NEQ, "NEQ"},
                                      {Parser::LT, "LT"},
                                      {Parser::LE, "LE"},
                                      {Parser::GT, "GT"},
                                      {Parser::GE, "GE"},
                                      {Parser::AND, "AND"},
                                      {Parser::OR, "OR"},
                                      {Parser::ASSIGN, "ASSIGN"},
                                      {Parser::ARRAY, "ARRAY"},
                                      {Parser::IF, "IF"},
                                      {Parser::THEN, "THEN"},
                                      {Parser::ELSE, "ELSE"},
                                      {Parser::WHILE, "WHILE"},
                                      {Parser::FOR, "FOR"},
                                      {Parser::TO, "TO"},
                                      {Parser::DO, "DO"},
                                      {Parser::LET, "LET"},
                                      {Parser::IN, "IN"},
                                      {Parser::END, "END"},
                                      {Parser::OF, "OF"},
                                      {Parser::BREAK, "BREAK"},
                                      {Parser::NIL, "NIL"},
                                      {Parser::FUNCTION, "FUNCTION"},
                                      {Parser::VAR, "VAR"},
                                      {Parser::TYPE, "TYPE"}};

}  // namespace

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "usage: a.out filename\n");
    exit(1);
  }

  std::string fname = argv[1];
  errormsg.Reset(fname, infile);

  Scanner scanner(infile);

  while (int tok = scanner.lex()) {
    switch (tok) {
      case Parser::ID:
      case Parser::STRING:
        printf("%10s %4d %s\n", tokname[tok].c_str(), errormsg.tokPos,
               scanner.matched() != "" ? scanner.matched().c_str() : "(null)");
        break;
      case Parser::INT:
        printf("%10s %4d %d\n", tokname[tok].c_str(), errormsg.tokPos,
               std::stoi(scanner.matched()));
        break;
      default:
        printf("%10s %4d\n", tokname[tok].c_str(), errormsg.tokPos);
    }
  }
  return 0;
}
