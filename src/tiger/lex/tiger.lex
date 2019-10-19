%filenames = "scanner"

 /*
  * Please don't modify the lines above.
  */


 /* You can add lex definitions here. */

%x COMMENT STR

// definitions

digits [0-9]+
id [a-zA-Z_][a-zA-Z0-9_]*


%%

 /*
  * TODO: Put your codes here (lab2).
  *
  * Below is examples, which you can wipe out
  * and write regular expressions and actions of your own.
  *
  * All the tokens:
  *   Parser::ID
  *   Parser::STRING
  *   Parser::INT
  *   Parser::COMMA
  *   Parser::COLON
  *   Parser::SEMICOLON
  *   Parser::LPAREN
  *   Parser::RPAREN
  *   Parser::LBRACK
  *   Parser::RBRACK
  *   Parser::LBRACE
  *   Parser::RBRACE
  *   Parser::DOT
  *   Parser::PLUS
  *   Parser::MINUS
  *   Parser::TIMES
  *   Parser::DIVIDE
  *   Parser::EQ
  *   Parser::NEQ
  *   Parser::LT
  *   Parser::LE
  *   Parser::GT
  *   Parser::GE
  *   Parser::AND
  *   Parser::OR
  *   Parser::ASSIGN
  *   Parser::ARRAY
  *   Parser::IF
  *   Parser::THEN
  *   Parser::ELSE
  *   Parser::WHILE
  *   Parser::FOR
  *   Parser::TO
  *   Parser::DO
  *   Parser::LET
  *   Parser::IN
  *   Parser::END
  *   Parser::OF
  *   Parser::BREAK
  *   Parser::NIL
  *   Parser::FUNCTION
  *   Parser::VAR
  *   Parser::TYPE
  */

 /*
  * skip white space chars.
  * space, tabs and LF
  */
[ \t]+ {adjust();}
\n {adjust(); errormsg.Newline();}
 

{digits} {adjust(); return Parser::INT;}


 /* punctuation symbols */
"/*" {beginComment(); more(); begin(StartCondition__::COMMENT);}
\" {more(); begin(StartCondition__::STR); }
"," {adjust(); return Parser::COMMA;}
":" {adjust(); return Parser::COLON;}
";" {adjust(); return Parser::SEMICOLON;}
"(" {adjust(); return Parser::LPAREN;}
")" {adjust(); return Parser::RPAREN;}
"[" {adjust(); return Parser::LBRACK;}
"]" {adjust(); return Parser::RBRACK;}
"{" {adjust(); return Parser::LBRACE;}
"}" {adjust(); return Parser::RBRACE;}
"." {adjust(); return Parser::DOT;}
"+" {adjust(); return Parser::PLUS;}
"-" {adjust(); return Parser::MINUS;}
"*" {adjust(); return Parser::TIMES;}
"/" {adjust(); return Parser::DIVIDE;}
"=" {adjust(); return Parser::EQ;}
"<>" {adjust(); return Parser::NEQ;}
"<" {adjust(); return Parser::LT;}
"<=" {adjust(); return Parser::LE;}
">" {adjust(); return Parser::GT;}
">=" {adjust(); return Parser::GE;}
"&" {adjust(); return Parser::AND;}
"|" {adjust(); return Parser::OR;}
":=" {adjust(); return Parser::ASSIGN;}


 /* reserved words */ 
"array" {adjust(); return Parser::ARRAY;}
"if" {adjust(); return Parser::IF;}
"then" {adjust(); return Parser::THEN;}
"else" {adjust(); return Parser::ELSE;}
"while" {adjust(); return Parser::WHILE;}
"for" {adjust(); return Parser::FOR;}
"to" {adjust(); return Parser::TO;}
"do" {adjust(); return Parser::DO;}
"let" {adjust(); return Parser::LET;}
"in" {adjust(); return Parser::IN;}
"end" {adjust(); return Parser::END;}
"of" {adjust(); return Parser::OF;}
"break" {adjust(); return Parser::BREAK;}
"nil" {adjust(); return Parser::NIL;}
"function" {adjust(); return Parser::FUNCTION;}
"var" {adjust(); return Parser::VAR;}
"type" {adjust(); return Parser::TYPE;}


{id} {adjust(); return Parser::ID;}
. {adjust(); errormsg.Error(errormsg.tokPos, "illegal token");}

<COMMENT>{
  
  "*/" {
          adjust(); 
          int i = endComment(); 
          if(i == 0) 
            begin(StartCondition__::INITIAL);
      }

  "/*" {adjust(); beginComment();}

  . more();

  \n more();

}

<STR>{
  \"  {
        adjust();
        begin(StartCondition__::INITIAL);

        setMatched(getStringBuf());
        setStringBuf(std::string());

        return Parser::STRING;
      }

  \\n {
        more();
        std::string match = getStringBuf();
        setStringBuf(match + std::string("\n"));
      }
  \\t {
        more();
        std::string match = getStringBuf();
        setStringBuf(match + std::string("\t"));
      }
  \\\\ {
        more();
        std::string match = getStringBuf();
        setStringBuf(match + std::string("\\"));
      }
  \\\" {
        more();
        std::string match = getStringBuf();
        setStringBuf(match + std::string("\""));
      }
  \\[0-9]+ {
        more();
        std::string match = matched();
        std::string matchBuf = getStringBuf();
        matchBuf += stoi(match.substr(match.length() - 3, match.length()));
        setStringBuf(matchBuf);
      }
  \\\^[A-Z] {
        more();
        std::string match = matched();
        char escape = match[match.length() - 1];

        std::string matchBuf = getStringBuf();
        matchBuf += escape - 'A' + 1;
        setStringBuf(matchBuf);
      }
  \\[ \r\n\t\v]+\\ more();

  . {
      more();
      std::string matchBuf = getStringBuf();
      matchBuf += matched()[length() - 1];
      setStringBuf(matchBuf);
    }

}