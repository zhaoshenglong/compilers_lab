%filenames = "scanner"

 /*
  * Please don't modify the lines above.
  */

 /* You can add lex definitions here. */

%x COMMENT STR

%%

 /*
  * TODO: Put your codes here (lab2).
  * 
  * Below is examples, which you can wipe out
  * and write regular expressions and actions of your own.
  *
  */ 

 /*
  * skip white space chars. 
  * space, tabs and LF
  */
[ \t]+ {adjust();}
\n {adjust(); errormsg.Newline();}
 
 /* reserved words */
"array" {adjust(); return Parser::ARRAY;}

. {adjust(); errormsg.Error(errormsg.tokPos, "illegal token");}
