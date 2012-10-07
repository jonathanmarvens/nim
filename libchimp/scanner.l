%{

#include "chimp/str.h"
#include "chimp/_parser.h"

#define CHIMP_REF_TOKEN(token, r) yylval.ref = (r); return (token);
#define CHIMP_TOKEN(token) return (token);

#define YY_NO_INPUT /* silence 'warning: ‘input’ defined but not used' */

%}

%option nounput
%option noyywrap

%%

[ \t\r\n]                   { /* do nothing */ }
"("                         { CHIMP_TOKEN(TOK_LBRACKET) }
")"                         { CHIMP_TOKEN(TOK_RBRACKET) }
";"                         { CHIMP_TOKEN(TOK_SEMICOLON) }
[a-zA-Z_][a-zA-Z_0-9]*[!?]? { CHIMP_REF_TOKEN(TOK_IDENT, chimp_str_new (NULL, yytext, yyleng)) }

