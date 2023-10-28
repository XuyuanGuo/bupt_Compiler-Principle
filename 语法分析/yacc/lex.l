%{
#include "y.tab.h"
%}
digit [0-9]
oct_digit [0-7]
hex_digit [a-fA-F0-9]
oct_num 0{oct_digit}+
hex_num 0x{hex_digit}+
num {digit}+(\.{digit}+)?(E[+\-]?{digit}+)?
%%

{oct_num}      { return(NUM); }
{hex_num}      { return(NUM); }
{num}          { return(NUM); }
[\+\-\*/()]    { return yytext[0]; }
[ \t\n]         ;
.              { printf("Unexpected character %s\n", yytext); exit(1); }

%%

int yywrap() {
    return 1;
}
