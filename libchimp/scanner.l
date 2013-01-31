%{

#include <inttypes.h>

#include "chimp/str.h"
#include "chimp/_parser_ext.h"
#include "chimp/_parser.h"
#include "chimp/int.h"
#include "chimp/float.h"

#define CHIMP_REF_TOKEN(token, r) yylval->ref = (r); return (token);
#define CHIMP_TOKEN(token) return (token);
#define CHIMP_NEWLINE(n) \
    do { \
        yylloc->first_line += (n); \
        yylloc->last_line = yylloc->first_line; \
        yylloc->first_column = 0; \
        yylloc->last_column = 0; \
    } while (0)

#define YY_NO_INPUT /* silence 'warning: ‘input’ defined but not used' */

%}

/* silence warnings / errors */
/* %option nounput */
%option noyywrap

%option debug

%option 8bit

/* enable bison location info */
%option bison-locations

%option bison-bridge

/* reentrant scanner */
%option reentrant

/* prefer a fast scanner to a small one */
%option full

/* strings need special escaping voodoo */
%x IN_STRING

/* newlines need to perform acrobatics to properly cope with comments */
%x NEWLINES

%%

[#][^\n]*                   { /* do nothing (comment) */ }
[\n]+                       {
                                CHIMP_NEWLINE(yyleng);
                                BEGIN(NEWLINES);
                                CHIMP_TOKEN(TOK_NEWLINE)
                            }
"true"                      { CHIMP_TOKEN(TOK_TRUE) }
"false"                     { CHIMP_TOKEN(TOK_FALSE) }
"nil"                       { CHIMP_TOKEN(TOK_NIL) }
"or"                        { CHIMP_TOKEN(TOK_OR) }
"and"                       { CHIMP_TOKEN(TOK_AND) }
"use"                       { CHIMP_TOKEN(TOK_USE) }
"ret"                       { CHIMP_TOKEN(TOK_RET) }
"fn"                        { CHIMP_TOKEN(TOK_FN) }
"var"                       { CHIMP_TOKEN(TOK_VAR) }
"not"                       { CHIMP_TOKEN(TOK_NOT) }
"spawn"                     { CHIMP_TOKEN(TOK_SPAWN) }
"match"                     { CHIMP_TOKEN(TOK_MATCH) }
"class"                     { CHIMP_TOKEN(TOK_CLASS) }
[ \t\r]                     { /* do nothing */ }
"."                         { CHIMP_TOKEN(TOK_FULLSTOP) }
"_"                         { CHIMP_TOKEN(TOK_UNDERSCORE) }
"("                         { CHIMP_TOKEN(TOK_LBRACKET) }
")"                         { CHIMP_TOKEN(TOK_RBRACKET) }
";"                         { CHIMP_TOKEN(TOK_SEMICOLON) }
":"                         { CHIMP_TOKEN(TOK_COLON) }
","                         { CHIMP_TOKEN(TOK_COMMA) }
"|"                         { CHIMP_TOKEN(TOK_PIPE) }
"["                         { CHIMP_TOKEN(TOK_LSQBRACKET) }
"]"                         { CHIMP_TOKEN(TOK_RSQBRACKET) }
"="                         { CHIMP_TOKEN(TOK_ASSIGN) }
"=="                        { CHIMP_TOKEN(TOK_EQ) }
"!="                        { CHIMP_TOKEN(TOK_NEQ) }
"{"                         { CHIMP_TOKEN(TOK_LBRACE) }
"}"                         { CHIMP_TOKEN(TOK_RBRACE) }
"<"                         { CHIMP_TOKEN(TOK_LT) }
"<="                        { CHIMP_TOKEN(TOK_LTE) }
">"                         { CHIMP_TOKEN(TOK_GT) }
">="                        { CHIMP_TOKEN(TOK_GTE) }
"+"                         { CHIMP_TOKEN(TOK_PLUS) }
"-"                         { CHIMP_TOKEN(TOK_MINUS) }
"*"                         { CHIMP_TOKEN(TOK_ASTERISK) }
"/"                         { CHIMP_TOKEN(TOK_SLASH) }
"if"                        { CHIMP_TOKEN(TOK_IF) }
"else"                      { CHIMP_TOKEN(TOK_ELSE) }
"while"                     { CHIMP_TOKEN(TOK_WHILE) }
"break"                     { CHIMP_TOKEN(TOK_BREAK) }
\"                          { BEGIN(IN_STRING); yylval->ref = chimp_str_new ("", 0); }
<IN_STRING>\"               { BEGIN(INITIAL); CHIMP_REF_TOKEN(TOK_STR, yylval->ref); }
<IN_STRING>\\r              { chimp_str_append_char (yylval->ref, '\r'); }
<IN_STRING>\\n              { chimp_str_append_char (yylval->ref, '\n'); }
<IN_STRING>\\t              { chimp_str_append_char (yylval->ref, '\t'); }
<IN_STRING>\\b              { chimp_str_append_char (yylval->ref, '\b'); }
<IN_STRING>\\f              { chimp_str_append_char (yylval->ref, '\f'); }
<IN_STRING>\\(.|\n)         { chimp_str_append_char (yylval->ref, yytext[1]); }
<IN_STRING>[^\\\"]+         { chimp_str_append_strn (yylval->ref, yytext, yyleng); }
<NEWLINES>[#][^\n]+         { /* do nothing (comment) */ }
<NEWLINES>[#]               { /* do nothing */ }
<NEWLINES>[ \t\r]           { /* do nothing */ }
<NEWLINES>\n+               { CHIMP_NEWLINE(yyleng); }
<NEWLINES>.                 { BEGIN(INITIAL); unput(*yytext); }
[a-zA-Z_][a-zA-Z_0-9]*[!?]? { CHIMP_REF_TOKEN(TOK_IDENT, chimp_str_new (yytext, yyleng)) }
[+-]?[0-9]+                 { CHIMP_REF_TOKEN(TOK_INT, chimp_int_new (strtoll(yytext, NULL, 0))) }
[+-]?[0-9]*\.?[0-9]*        { CHIMP_REF_TOKEN(TOK_FLOAT, chimp_float_new (strtod(yytext, NULL))) }

