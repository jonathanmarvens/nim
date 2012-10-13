%{

#include "chimp/gc.h"
#include "chimp/any.h"
#include "chimp/ast.h"
#include "chimp/str.h"
#include "chimp/array.h"
#include "chimp/hash.h"
#include "chimp/object.h"

extern int yylex(void);

void yyerror(const char *format, ...);

/* TODO make all this stuff reentrant */
extern ChimpRef *main_mod;

%}

%union {
    ChimpRef *ref;
}

%token TOK_TRUE TOK_FALSE
%token TOK_LBRACKET TOK_RBRACKET TOK_SEMICOLON TOK_COMMA TOK_COLON
%token TOK_LSQBRACKET TOK_RSQBRACKET TOK_LBRACE TOK_RBRACE
%token TOK_ASSIGN

%token <ref> TOK_IDENT TOK_STR

%type <ref> module
%type <ref> stmt
%type <ref> assign
%type <ref> opt_stmts
%type <ref> expr
%type <ref> call
%type <ref> opt_args args opt_args_tail
%type <ref> opt_array_elements array_elements opt_array_elements_tail
%type <ref> opt_hash_elements hash_elements opt_hash_elements_tail
%type <ref> ident str array hash bool

%%

module : opt_stmts { main_mod = chimp_ast_mod_new_root (CHIMP_STR_NEW(NULL, "main"), $1); }
       ;

opt_stmts : stmt opt_stmts { $$ = $2; chimp_array_unshift ($$, $1); }
          | /* empty */ { $$ = chimp_array_new (NULL); }
          ;

stmt : expr TOK_SEMICOLON { $$ = chimp_ast_stmt_new_expr ($1); }
     | assign TOK_SEMICOLON { $$ = $1; }
     ;

assign : ident TOK_ASSIGN expr { $$ = chimp_ast_stmt_new_assign ($1, $3); }
       ;

expr : call { $$ = $1; }
     | str { $$ = $1; }
     | array { $$ = $1; }
     | hash { $$ = $1; }
     | ident { $$ = $1; }
     | bool { $$ = $1; }
     ;

call : ident TOK_LBRACKET opt_args TOK_RBRACKET { $$ = chimp_ast_expr_new_call ($1, $3); }
     ;

opt_args : args        { $$ = $1; }
         | /* empty */ { $$ = chimp_array_new (NULL); }
         ;

args : expr opt_args_tail { $$ = $2; chimp_array_unshift ($$, $1); }
     ;

opt_args_tail : TOK_COMMA expr opt_args_tail { $$ = $3; chimp_array_unshift ($$, $2); }
              | /* empty */ { $$ = chimp_array_new (NULL); }
              ;

ident : TOK_IDENT { $$ = chimp_ast_expr_new_ident ($1); }
      ;

str : TOK_STR { $$ = chimp_ast_expr_new_str ($1); }
    ;

bool : TOK_TRUE { $$ = chimp_ast_expr_new_bool (chimp_true); }
     | TOK_FALSE { $$ = chimp_ast_expr_new_bool (chimp_false); }
     ;

hash : TOK_LBRACE opt_hash_elements TOK_RBRACE { $$ = chimp_ast_expr_new_hash ($2); }
     ;

opt_hash_elements : hash_elements { $$ = $1; }
                  | /* empty */ { $$ = chimp_array_new (NULL); }
                  ;

hash_elements : expr TOK_COLON expr opt_hash_elements_tail { $$ = $4; chimp_array_unshift ($$, $3); chimp_array_unshift ($$, $1); }
              ;

opt_hash_elements_tail : TOK_COMMA expr TOK_COLON expr opt_hash_elements_tail { $$ = $5; chimp_array_unshift ($$, $4); chimp_array_unshift ($$, $2); }
                       | /* empty */ { $$ = chimp_array_new (NULL); }
                       ;

array : TOK_LSQBRACKET opt_array_elements TOK_RSQBRACKET { $$ = chimp_ast_expr_new_array ($2); }
      ;

opt_array_elements : array_elements { $$ = $1; }
                   | /* empty */ { $$ = chimp_array_new (NULL); }
                   ;

array_elements : expr opt_array_elements_tail { $$ = $2; chimp_array_unshift ($$, $1); }
               ;

opt_array_elements_tail : TOK_COMMA expr opt_array_elements_tail { $$ = $3; chimp_array_unshift ($$, $2); }
                        | /* empty */ { $$ = chimp_array_new (NULL); }
                        ;

%%

ChimpRef *main_mod = NULL;

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

