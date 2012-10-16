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

[#][^\n]*                   { /* do nothing (comment) */ }

"true"                      { CHIMP_TOKEN(TOK_TRUE) }
"false"                     { CHIMP_TOKEN(TOK_FALSE) }
"nil"                       { CHIMP_TOKEN(TOK_NIL) }
"or"                        { CHIMP_TOKEN(TOK_OR) }
"and"                       { CHIMP_TOKEN(TOK_AND) }
"use"                       { CHIMP_TOKEN(TOK_USE) }
"ret"                       { CHIMP_TOKEN(TOK_RET) }
[ \t\r\n]                   { /* do nothing */ }
"."                         { CHIMP_TOKEN(TOK_FULLSTOP) }
"("                         { CHIMP_TOKEN(TOK_LBRACKET) }
")"                         { CHIMP_TOKEN(TOK_RBRACKET) }
";"                         { CHIMP_TOKEN(TOK_SEMICOLON) }
","                         { CHIMP_TOKEN(TOK_COMMA) }
"["                         { CHIMP_TOKEN(TOK_LSQBRACKET) }
"]"                         { CHIMP_TOKEN(TOK_RSQBRACKET) }
"="                         { CHIMP_TOKEN(TOK_ASSIGN) }
"=="                        { CHIMP_TOKEN(TOK_EQ) }
"!="                        { CHIMP_TOKEN(TOK_NEQ) }
":"                         { CHIMP_TOKEN(TOK_COLON) }
"{"                         { CHIMP_TOKEN(TOK_LBRACE) }
"}"                         { CHIMP_TOKEN(TOK_RBRACE) }
"if"                        { CHIMP_TOKEN(TOK_IF) }
"else"                      { CHIMP_TOKEN(TOK_ELSE) }
["][^"]*["]                 { CHIMP_REF_TOKEN(TOK_STR, chimp_str_new (NULL, yytext + 1, yyleng - 2)) }
[a-zA-Z_][a-zA-Z_0-9]*[!?]? { CHIMP_REF_TOKEN(TOK_IDENT, chimp_str_new (NULL, yytext, yyleng)) }
[0-9]+                      { CHIMP_REF_TOKEN(TOK_INT, chimp_int_new (NULL, yylval)) }
