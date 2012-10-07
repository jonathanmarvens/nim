%{

#include "chimp/gc.h"
#include "chimp/any.h"
#include "chimp/ast.h"
#include "chimp/str.h"
#include "chimp/array.h"

extern int yylex(void);

void yyerror(const char *format, ...);

/* TODO make all this stuff reentrant */
extern ChimpRef *main_mod;

%}

%union {
    ChimpRef *ref;
}

%token TOK_LBRACKET TOK_RBRACKET TOK_SEMICOLON

%token <ref> TOK_IDENT

%type <ref> module
%type <ref> stmt
%type <ref> expr
%type <ref> call ident

%%

module : stmt { $$ = chimp_ast_mod_new_root (CHIMP_STR_NEW(NULL, "main"), chimp_array_new_var (NULL, $1, NULL)); }
       ;

stmt : expr TOK_SEMICOLON { $$ = chimp_ast_stmt_new_expr ($1); }
     ;

expr : call
     ;

call : ident TOK_LBRACKET TOK_RBRACKET { $$ = chimp_ast_expr_new_call ($1, chimp_array_new (NULL)); }
     ;

ident : TOK_IDENT { $$ = chimp_ast_expr_new_ident ($1); }
      ;

%%

void
yyerror (const char *format, ...)
{
    va_list args;

    fprintf (stderr, "error: ");
    va_start (args, format);
    vfprintf (stderr, format, args);
    va_end (args);
    fprintf (stderr, "\n");
    exit (1);
}

