#pragma once
#pragma once
#include "lex.yy.hh"
#include "RuleLangParser.tab.hh"

namespace quisp {
namespace rules {
namespace lang {

class RuleLangLexer : public yyFlexLexer {
 public:
  int yylex(RuleLangParser::semantic_type& lval, RuleLangParser::location_type& loc);
};
}  // namespace lang
}  // namespace rules
}  // namespace quisp
