%{
    #include <stdio.h>
    #include <math.h>
    int yylex(void);
    void yyerror(char const *);
    double it;
%}

%union {
    int ival;
    double dval;
    char cval;
}

%token NUMBER
%token IF ELSE AND OR
%type <ival> exp NUMBER

%%

input:
    | input line
    ;

line: '\n'
    | exp '\n' {printf("%.10d\n", $1);}
    ;

exp: NUMBER
    | exp exp '+' { $$ = $1 + $2; }
    ;
%%

void yyerror(char const *s) {
    printf("%s\n", s);
}
int main(void){
    return yyparse();
}
