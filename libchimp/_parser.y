%{

#include "chimp/gc.h"
#include "chimp/any.h"
#include "chimp/ast.h"
#include "chimp/str.h"
#include "chimp/int.h"
#include "chimp/array.h"
#include "chimp/hash.h"
#include "chimp/object.h"

extern int yylex(void);

void yyerror(const char *format, ...);

/* TODO make all this stuff reentrant */
extern ChimpRef *main_mod;
extern chimp_bool_t chimp_parsing;

%}

%union {
    ChimpRef *ref;
}

%token TOK_TRUE TOK_FALSE TOK_NIL
%token TOK_LBRACKET TOK_RBRACKET TOK_SEMICOLON TOK_COMMA TOK_COLON
%token TOK_FULLSTOP
%token TOK_LSQBRACKET TOK_RSQBRACKET TOK_LBRACE TOK_RBRACE
%token TOK_ASSIGN
%token TOK_IF TOK_ELSE TOK_USE TOK_RET TOK_PANIC TOK_FN

%left TOK_OR TOK_AND
%left TOK_NEQ TOK_EQ
%left TOK_PLUS TOK_MINUS
%left TOK_ASTERISK TOK_SLASH

%token <ref> TOK_IDENT TOK_STR TOK_INT

%type <ref> module
%type <ref> stmt simple_stmt compound_stmt
%type <ref> assign
%type <ref> opt_stmts block opt_else
%type <ref> opt_expr expr simple
%type <ref> opt_simple_tail
%type <ref> opt_decls opt_uses
%type <ref> use
%type <ref> func_decl
%type <ref> opt_params opt_params_tail param
%type <ref> opt_args args opt_args_tail
%type <ref> opt_array_elements array_elements opt_array_elements_tail
%type <ref> opt_hash_elements hash_elements opt_hash_elements_tail
%type <ref> ident str array hash bool nil int
%type <ref> ret panic

%%

module : opt_uses opt_decls { main_mod = chimp_ast_mod_new_root (CHIMP_STR_NEW("main"), $1, $2); }
       ;

opt_uses : use opt_uses { $$ = $2; chimp_array_unshift ($$, $1); }
         | /* empty */ { $$ = chimp_array_new (); }
         ;

use : TOK_USE ident TOK_SEMICOLON { $$ = chimp_ast_decl_new_use (CHIMP_AST_EXPR($2)->ident.id); }
    ;

opt_decls : func_decl opt_decls { $$ = $2; chimp_array_unshift ($$, $1); }
          | /* empty */ { $$ = chimp_array_new (); }
          ;

func_decl : ident opt_params TOK_LBRACE opt_stmts TOK_RBRACE {
            $$ = chimp_ast_decl_new_func (CHIMP_AST_EXPR($1)->ident.id, $2, $4);
          }
          ;

opt_params : param opt_params_tail { $$ = $2; chimp_array_unshift ($$, $1); }
           | /* empty */ { $$ = chimp_array_new (); }
           ;

opt_params_tail : TOK_COMMA param opt_params_tail { $$ = $3; chimp_array_unshift ($$, $2); }
                 | /* empty */ { $$ = chimp_array_new (); }
                 ;

param : ident { $$ = chimp_ast_decl_new_var (CHIMP_AST_EXPR($1)->ident.id); }
      ;

opt_stmts : stmt opt_stmts { $$ = $2; chimp_array_unshift ($$, $1); }
          | /* empty */ { $$ = chimp_array_new (); }
          ;

stmt : simple_stmt TOK_SEMICOLON { $$ = $1; }
     | compound_stmt { $$ = $1; }
     ;

simple_stmt : expr { $$ = chimp_ast_stmt_new_expr ($1); }
            | assign { $$ = $1; }
            | ret { $$ = $1; }
            | panic { $$ = $1; }
            ;

compound_stmt : TOK_IF expr block opt_else { $$ = chimp_ast_stmt_new_if_ ($2, $3, $4); }
              ;

opt_else : TOK_ELSE block { $$ = $2; }
         | /* empty */ { $$ = NULL; }
         ;

block : TOK_LBRACE opt_stmts TOK_RBRACE { $$ = $2; }
      | stmt { $$ = chimp_array_new (); chimp_array_unshift ($$, $1); }
      | TOK_SEMICOLON { $$ = chimp_array_new (); }
      ;

assign : ident TOK_ASSIGN expr { $$ = chimp_ast_stmt_new_assign ($1, $3); }
       ;

expr : expr TOK_OR expr  { $$ = chimp_ast_expr_new_binop (CHIMP_BINOP_OR, $1, $3); }
     | expr TOK_AND expr { $$ = chimp_ast_expr_new_binop (CHIMP_BINOP_AND, $1, $3); }
     | expr TOK_EQ expr  { $$ = chimp_ast_expr_new_binop (CHIMP_BINOP_EQ, $1, $3); }
     | expr TOK_NEQ expr { $$ = chimp_ast_expr_new_binop (CHIMP_BINOP_NEQ, $1, $3); }
     | expr TOK_PLUS expr     { $$ = chimp_ast_expr_new_binop (CHIMP_BINOP_ADD, $1, $3); }
     | expr TOK_MINUS expr    { $$ = chimp_ast_expr_new_binop (CHIMP_BINOP_SUB, $1, $3); }
     | expr TOK_ASTERISK expr { $$ = chimp_ast_expr_new_binop (CHIMP_BINOP_MUL, $1, $3); }
     | expr TOK_SLASH expr    { $$ = chimp_ast_expr_new_binop (CHIMP_BINOP_DIV, $1, $3); }
     | TOK_FN opt_params TOK_LBRACE opt_stmts TOK_RBRACE { $$ = chimp_ast_expr_new_fn ($2, $4); }
     | simple opt_simple_tail {
        $$ = $1;
        if ($2 != NULL) {
            $$ = $2;
            CHIMP_AST_EXPR_TARGETABLE_INNER($$, $1);
        }
     }
     ;

simple : nil { $$ = $1; }
       | str { $$ = $1; }
       | int { $$ = $1; }
       | array { $$ = $1; }
       | hash { $$ = $1; }
       | ident { $$ = $1; }
       | bool { $$ = $1; }
       ;

opt_simple_tail : TOK_LBRACKET opt_args TOK_RBRACKET opt_simple_tail {
                    ChimpRef *call;
                    call = chimp_ast_expr_new_call (NULL, $2);
                    if ($4 != NULL) {
                        $$ = $4;
                        CHIMP_AST_EXPR_TARGETABLE_INNER($$, call);
                    }
                    else {
                        $$ = call;
                    }
                }
                | TOK_FULLSTOP ident opt_simple_tail {
                    ChimpRef *getattr;
                    getattr = chimp_ast_expr_new_getattr (NULL, CHIMP_AST_EXPR($2)->ident.id);
                    if ($3 != NULL) {
                        $$ = $3;
                        CHIMP_AST_EXPR_TARGETABLE_INNER($$, getattr);
                    }
                    else {
                        $$ = getattr;
                    }
                }
                | /* empty */ { $$ = NULL; }
                ;

opt_args : args        { $$ = $1; }
         | /* empty */ { $$ = chimp_array_new (); }
         ;

args : expr opt_args_tail { $$ = $2; chimp_array_unshift ($$, $1); }
     ;

opt_args_tail : TOK_COMMA expr opt_args_tail { $$ = $3; chimp_array_unshift ($$, $2); }
              | /* empty */ { $$ = chimp_array_new (); }
              ;

ident : TOK_IDENT { $$ = chimp_ast_expr_new_ident ($1); }
      ;

str : TOK_STR { $$ = chimp_ast_expr_new_str ($1); }
    ;

int : TOK_INT { $$ = chimp_ast_expr_new_int_ ($1); }
    ;

nil : TOK_NIL { $$ = chimp_ast_expr_new_nil (); }
    ;

bool : TOK_TRUE { $$ = chimp_ast_expr_new_bool (chimp_true); }
     | TOK_FALSE { $$ = chimp_ast_expr_new_bool (chimp_false); }
     ;

hash : TOK_LBRACE opt_hash_elements TOK_RBRACE { $$ = chimp_ast_expr_new_hash ($2); }
     ;

opt_hash_elements : hash_elements { $$ = $1; }
                  | /* empty */ { $$ = chimp_array_new (); }
                  ;

hash_elements : expr TOK_COLON expr opt_hash_elements_tail { $$ = $4; chimp_array_unshift ($$, $3); chimp_array_unshift ($$, $1); }
              ;

opt_hash_elements_tail : TOK_COMMA expr TOK_COLON expr opt_hash_elements_tail { $$ = $5; chimp_array_unshift ($$, $4); chimp_array_unshift ($$, $2); }
                       | /* empty */ { $$ = chimp_array_new (); }
                       ;

array : TOK_LSQBRACKET opt_array_elements TOK_RSQBRACKET { $$ = chimp_ast_expr_new_array ($2); }
      ;

opt_array_elements : array_elements { $$ = $1; }
                   | /* empty */ { $$ = chimp_array_new (); }
                   ;

array_elements : expr opt_array_elements_tail { $$ = $2; chimp_array_unshift ($$, $1); }
               ;

opt_array_elements_tail : TOK_COMMA expr opt_array_elements_tail { $$ = $3; chimp_array_unshift ($$, $2); }
                        | /* empty */ { $$ = chimp_array_new (); }
                        ;

opt_expr : expr { $$ = $1; }
         | /* empty */ { $$ = NULL; }
         ;

panic : TOK_PANIC opt_expr { $$ = chimp_ast_stmt_new_panic ($2); }
      ;

ret : TOK_RET opt_expr { $$ = chimp_ast_stmt_new_ret ($2); }
    ;

%%

ChimpRef *main_mod = NULL;
chimp_bool_t chimp_parsing = CHIMP_FALSE;

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

