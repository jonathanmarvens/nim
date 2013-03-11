%{

#include <inttypes.h>

#include "nim/str.h"
#include "nim/_parser_ext.h"
#include "nim/_parser.h"
#include "nim/int.h"
#include "nim/float.h"

#define NIM_REF_TOKEN(token, r) yylval->ref = (r); return (token);
#define NIM_TOKEN(token) return (token);
#define NIM_NEWLINE(n) \
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
                                NIM_NEWLINE(yyleng);
                                BEGIN(NEWLINES);
                                NIM_TOKEN(TOK_NEWLINE)
                            }
"true"                      { NIM_TOKEN(TOK_TRUE) }
"false"                     { NIM_TOKEN(TOK_FALSE) }
"nil"                       { NIM_TOKEN(TOK_NIL) }
"or"                        { NIM_TOKEN(TOK_OR) }
"and"                       { NIM_TOKEN(TOK_AND) }
"use"                       { NIM_TOKEN(TOK_USE) }
"ret"                       { NIM_TOKEN(TOK_RET) }
"fn"                        { NIM_TOKEN(TOK_FN) }
"var"                       { NIM_TOKEN(TOK_VAR) }
"not"                       { NIM_TOKEN(TOK_NOT) }
"spawn"                     { NIM_TOKEN(TOK_SPAWN) }
"match"                     { NIM_TOKEN(TOK_MATCH) }
"class"                     { NIM_TOKEN(TOK_CLASS) }
[ \t\r]                     { /* do nothing */ }
"."                         { NIM_TOKEN(TOK_FULLSTOP) }
"_"                         { NIM_TOKEN(TOK_UNDERSCORE) }
"("                         { NIM_TOKEN(TOK_LBRACKET) }
")"                         { NIM_TOKEN(TOK_RBRACKET) }
";"                         { NIM_TOKEN(TOK_SEMICOLON) }
":"                         { NIM_TOKEN(TOK_COLON) }
","                         { NIM_TOKEN(TOK_COMMA) }
"|"                         { NIM_TOKEN(TOK_PIPE) }
"["                         { NIM_TOKEN(TOK_LSQBRACKET) }
"]"                         { NIM_TOKEN(TOK_RSQBRACKET) }
"="                         { NIM_TOKEN(TOK_ASSIGN) }
"=="                        { NIM_TOKEN(TOK_EQ) }
"!="                        { NIM_TOKEN(TOK_NEQ) }
"{"                         { NIM_TOKEN(TOK_LBRACE) }
"}"                         { NIM_TOKEN(TOK_RBRACE) }
"<"                         { NIM_TOKEN(TOK_LT) }
"<="                        { NIM_TOKEN(TOK_LTE) }
">"                         { NIM_TOKEN(TOK_GT) }
">="                        { NIM_TOKEN(TOK_GTE) }
"+"                         { NIM_TOKEN(TOK_PLUS) }
"-"                         { NIM_TOKEN(TOK_MINUS) }
"*"                         { NIM_TOKEN(TOK_ASTERISK) }
"/"                         { NIM_TOKEN(TOK_SLASH) }
"if"                        { NIM_TOKEN(TOK_IF) }
"else"                      { NIM_TOKEN(TOK_ELSE) }
"while"                     { NIM_TOKEN(TOK_WHILE) }
"break"                     { NIM_TOKEN(TOK_BREAK) }
\"                          { BEGIN(IN_STRING); yylval->ref = nim_str_new ("", 0); }
<IN_STRING>\"               { BEGIN(INITIAL); NIM_REF_TOKEN(TOK_STR, yylval->ref); }
<IN_STRING>\\r              { nim_str_append_char (yylval->ref, '\r'); }
<IN_STRING>\\n              { nim_str_append_char (yylval->ref, '\n'); }
<IN_STRING>\\t              { nim_str_append_char (yylval->ref, '\t'); }
<IN_STRING>\\b              { nim_str_append_char (yylval->ref, '\b'); }
<IN_STRING>\\f              { nim_str_append_char (yylval->ref, '\f'); }
<IN_STRING>\\(.|\n)         { nim_str_append_char (yylval->ref, yytext[1]); }
<IN_STRING>[^\\\"]+         { nim_str_append_strn (yylval->ref, yytext, yyleng); }
<NEWLINES>[#][^\n]+         { /* do nothing (comment) */ }
<NEWLINES>[#]               { /* do nothing */ }
<NEWLINES>[ \t\r]           { /* do nothing */ }
<NEWLINES>\n+               { NIM_NEWLINE(yyleng); }
<NEWLINES>.                 { BEGIN(INITIAL); unput(*yytext); }
[a-zA-Z_][a-zA-Z_0-9]*[!?]? { NIM_REF_TOKEN(TOK_IDENT, nim_str_new (yytext, yyleng)) }
[+-]?[0-9]+                 { NIM_REF_TOKEN(TOK_INT, nim_int_new (strtoll(yytext, NULL, 0))) }
[+-]?[0-9]*\.?[0-9]*        { NIM_REF_TOKEN(TOK_FLOAT, nim_float_new (strtod(yytext, NULL))) }

