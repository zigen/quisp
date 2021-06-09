%{
    #pragma once
    #include <stdio.h>
    #include <stdlib.h>
    #include "RuleLangParser.tab.hh"

    #include "RuleLangLexer.h"

    #undef YY_DECL
    using namespace quisp::rules::lang;
    #define YY_DECL int RuleLangLexer::yylex(RuleLangParser::semantic_type& lval, RuleLangParser::location_type& loc)
    #define YY_USER_ACTION	loc.columns(yyleng);
%}

white   [ \t]

%%

{white}+    {};
if          {return yy::parser::token::IF;};
and         {return yy::parser::token::AND;};

%%
