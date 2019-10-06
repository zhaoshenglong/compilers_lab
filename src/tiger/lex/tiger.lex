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
        std::string match = matched();
        match = match.substr(1, match.length() - 2);
        

        std::string res;

        int i = 0;
        while(i < match.length()) {
          if (match[i] == '\\') {
            if (match[i + 1] == 't') {
              res += "\t";
              i += 2;
            } else if (match[i + 1] == 'n') {
              res += "\n";
              i += 2;
            } else if (match[i + 1] == '\\') {
              res += "\\";
              i += 2;
            } else if (match[i + 1] == '"'){
              res += "\"";
              i += 2;
            } else if (match[i + 1] == '^') {
              switch(match[i + 2]) {
                case 'C':
                  res += (char)3;
                  break;
                case 'O':
                  res += (char)15;
                  break;
                case 'M':
                  res += (char)13;
                  break;
                case 'P':
                  res += (char)16;
                  break;
                case 'I':
                  res += (char)9;
                  break;
                case 'L':
                  res += (char)12;
                  break;
                case 'E':
                  res += (char)5;
                  break;
                case 'R':
                  res += (char)18;
                  break;
                case 'S':
                  res += (char)19;
                  break;
              }
              i += 3;
            } else if (match[i + 1] <= '9' && match[i + 1] >= '0') {
              int val = std::stoi(match.substr(i + 1, i + 4));
              res += std::string(1, (char)val);
              i += 4;
            } else if (match[i + 1] == '\n') {
              i += 1;
              while(match[i] == '\n' || match[i] == ' ' || match[i] == '\t') {
                i++;
              }
            } else {
              i++;
            }
          } else {
            res += match[i];
            i++;
          }
        } 

        setMatched(res);
      
        return Parser::STRING;
      }
  \\.|.|\\\n more();

}