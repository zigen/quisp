%{
    #include <string.h>
    #include <stdio.h>
    #include "RuleLangParser.tab.h"
%}

white   [ \t]
integer [0-9]+
other   .

%%

{white}+    { printf("white space\n");}
{integer}   { printf("integer"); return NUMBER;}
{other}     { fprintf(stderr, "Illegal char %c\n", yytext[0]);}

%%

// int main() {
//     printf("hello world\n");
//     return yylex();
// }
