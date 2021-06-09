%require "3.2"
%language "c++"
%skeleton "lalr1.cc"
%defines
%define api.parser.class {RuleLangParser}

%locations
%define api.namespace   {quisp::rules::lang}
%define parse.error     verbose
%define api.value.type  variant

%token <int> IF AND
%token <std::string> IDENTIFIER
%nterm <std::string> item;
%token <std::string> TEXT;
%token <int> NUMBER;
%type <double> program

%start program

%{
    #include <stdio.h>
    #include <string>
    #include "lex.yy.hh"
    #include "RuleLangLexer.h"
    using namespace quisp::rules::lang;

%}

%%
item:
    TEXT
    | NUMBER    { $$ = std::to_string($1); }
    ;
program :               {  printf("program\n");}
        | item { $$ = $1; printf("program -> statement"); }
        ;

// statement   : item { $$ = $1; printf("empty statement"); }
//             | IF IDENTIFIER     { $$ = $1; printf("if stmt"); }
//             ;
%%


int main() {
    return 0;
}
