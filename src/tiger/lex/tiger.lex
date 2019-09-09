%filenames = "scanner"

/* You can add lex definitions here. */

%x COMMENT STR

%%

 /*
 * skip white space chars. 
 * space, tabs and LF
 */
[ \t]+ {adjust();}
\n {adjust(); errormsg.Newline();}
 
 /* reserved words */
"array" {adjust(); return Parser::ARRAY;}

. {adjust(); errormsg.Error(errormsg.tokPos, "illegal token");}
