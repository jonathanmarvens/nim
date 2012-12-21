%{

#include <inttypes.h>

#include "chimp/gc.h"
#include "chimp/any.h"
#include "chimp/ast.h"
#include "chimp/str.h"
#include "chimp/int.h"
#include "chimp/array.h"
#include "chimp/hash.h"
#include "chimp/object.h"
#include "chimp/_parser_ext.h"

void yyerror(const ChimpAstNodeLocation *loc, ChimpRef *filename, ChimpRef **mod, const char *format, ...);

%}

%union {
    ChimpRef *ref;
}

/* token location support for tracking lineno & column info */
%locations

/* necessary to support %locations properly afaict */
/* Bison 2.4 adds api.pure & deprecates %pure-parser but OSX has 2.3 ... */
/*%define api.pure*/
%pure-parser

/* better error messages for free */
%error-verbose

%parse-param { ChimpRef *filename }
/* We pass this around so we can avoid using a global */
/* XXX odd. doing this seems to trigger %locations for yyerror too */
/*     handy, but unexpected. */
%parse-param { ChimpRef **mod }
%lex-param { ChimpRef *filename }
%lex-param { ChimpRef **mod }

/* setting a custom YYLTYPE means bison no longer initializes yylval for us */
%initial-action {
    memset (&@$, 0, sizeof(@$));
    @$.filename = filename;
    @$.first_line = @$.last_line = 1;
    @$.first_column = @$.last_column = 1;
}

%{
extern int yylex(YYSTYPE *lvalp, YYLTYPE *llocp, ChimpRef *filename, ChimpRef **mod);
%}

%expect 1

%token TOK_TRUE "true"
%token TOK_FALSE "false"
%token TOK_NIL "nil"
%token TOK_LBRACKET "("
%token TOK_RBRACKET ")"
%token TOK_SEMICOLON ";"
%token TOK_COMMA ","
%token TOK_COLON ":"
%token TOK_FULLSTOP "."
%token TOK_LSQBRACKET "["
%token TOK_RSQBRACKET "]"
%token TOK_LBRACE "{"
%token TOK_RBRACE "}"
%token TOK_PIPE "|"
%token TOK_ASSIGN "="
%token TOK_IF "if"
%token TOK_ELSE "else"
%token TOK_USE "use"
%token TOK_RET "ret"
%token TOK_PANIC "panic"
%token TOK_FN "fn"
%token TOK_VAR "var"
%token TOK_WHILE "while"
%token TOK_SPAWN "spawn"
%token TOK_NOT "not"
%token TOK_RECEIVE "receive"
%token TOK_MATCH "match"
%token TOK_UNDERSCORE "_"

%right TOK_NOT
%left TOK_OR TOK_AND
%left TOK_NEQ TOK_EQ TOK_LT TOK_LTE TOK_GT TOK_GTE
%left TOK_PLUS TOK_MINUS
%left TOK_ASTERISK TOK_SLASH

%token <ref> TOK_IDENT "identifier"
%token <ref> TOK_STR "string literal"
%token <ref> TOK_INT "integer literal"

%type <ref> module
%type <ref> stmt simple_stmt compound_stmt
%type <ref> var_decl assign
%type <ref> stmts opt_stmts block else
%type <ref> opt_expr expr simple simpler
%type <ref> opt_simple_tail
%type <ref> opt_decls opt_uses
%type <ref> use
%type <ref> func_decl
%type <ref> opt_params opt_params_tail param
%type <ref> opt_args args opt_args_tail
%type <ref> opt_patterns pattern pattern_test opt_pattern_array_elements
%type <ref> pattern_array_elements opt_pattern_array_elements_tail
%type <ref> opt_array_elements array_elements opt_array_elements_tail
%type <ref> hash_elements opt_hash_elements_tail
%type <ref> opt_pattern_hash_elements pattern_hash_elements
%type <ref> opt_pattern_hash_elements_tail
%type <ref> ident str array hash bool nil int
%type <ref> ret panic

%%

module : opt_uses opt_decls { *mod = chimp_ast_mod_new_root (CHIMP_STR_NEW("main"), $1, $2, &@$); }
       ;

opt_uses : use opt_uses { $$ = $2; chimp_array_unshift ($$, $1); }
         | /* empty */ { $$ = chimp_array_new (); }
         ;

use : TOK_USE ident TOK_SEMICOLON { $$ = chimp_ast_decl_new_use (CHIMP_AST_EXPR($2)->ident.id, &@$); }
    ;

opt_decls : func_decl opt_decls { $$ = $2; chimp_array_unshift ($$, $1); }
          | /* empty */ { $$ = chimp_array_new (); }
          ;

func_decl : ident opt_params TOK_LBRACE opt_stmts TOK_RBRACE {
            $$ = chimp_ast_decl_new_func (CHIMP_AST_EXPR($1)->ident.id, $2, $4, &@$);
          }
          ;

opt_params : param opt_params_tail { $$ = $2; chimp_array_unshift ($$, $1); }
           | /* empty */ { $$ = chimp_array_new (); }
           ;

opt_params_tail : TOK_COMMA param opt_params_tail { $$ = $3; chimp_array_unshift ($$, $2); }
                 | /* empty */ { $$ = chimp_array_new (); }
                 ;

param : ident { $$ = chimp_ast_decl_new_var (CHIMP_AST_EXPR($1)->ident.id, NULL, &@$); }
      ;

stmts: stmt opt_stmts { $$ = $2; chimp_array_unshift ($$, $1); }
     ;

opt_stmts : stmt opt_stmts { $$ = $2; chimp_array_unshift ($$, $1); }
          | /* empty */ { $$ = chimp_array_new (); }
          ;

stmt : simple_stmt TOK_SEMICOLON { $$ = $1; }
     | compound_stmt { $$ = $1; }
     ;

simple_stmt : expr { $$ = chimp_ast_stmt_new_expr ($1, &@$); }
            | var_decl { $$ = $1; }
            | assign { $$ = $1; }
            | ret { $$ = $1; }
            | panic { $$ = $1; }
            ;

compound_stmt : TOK_IF expr block else { $$ = chimp_ast_stmt_new_if_ ($2, $3, $4, &@$); }
              | TOK_IF expr block { $$ = chimp_ast_stmt_new_if_ ($2, $3, NULL, &@$); }
              | TOK_WHILE expr block { $$ = chimp_ast_stmt_new_while_ ($2, $3, &@$); }
              | TOK_MATCH expr TOK_LBRACE opt_patterns TOK_RBRACE {
                    $$ = chimp_ast_stmt_new_match ($2, $4, &@$);
              }
              ;

else : TOK_ELSE block { $$ = $2; }

block : TOK_LBRACE stmts TOK_RBRACE { $$ = $2; }
      | TOK_LBRACE TOK_RBRACE { $$ = chimp_array_new (); }
      | stmt { $$ = chimp_array_new (); chimp_array_unshift ($$, $1); }
      | TOK_SEMICOLON { $$ = chimp_array_new (); }
      ;

var_decl : TOK_VAR ident { $$ = chimp_ast_decl_new_var (CHIMP_AST_EXPR($2)->ident.id, NULL, &@$); }
         | TOK_VAR ident TOK_ASSIGN expr { $$ = chimp_ast_decl_new_var (CHIMP_AST_EXPR($2)->ident.id, $4, &@$); }
         ;

assign : ident TOK_ASSIGN expr { $$ = chimp_ast_stmt_new_assign ($1, $3, &@$); }
       ;

expr : TOK_NOT expr { $$ = chimp_ast_expr_new_not ($2, &@$); }
     | expr TOK_OR expr  { $$ = chimp_ast_expr_new_binop (CHIMP_BINOP_OR, $1, $3, &@$); }
     | expr TOK_AND expr { $$ = chimp_ast_expr_new_binop (CHIMP_BINOP_AND, $1, $3, &@$); }
     | expr TOK_GT expr  { $$ = chimp_ast_expr_new_binop (CHIMP_BINOP_GT, $1, $3, &@$); }
     | expr TOK_GTE expr  { $$ = chimp_ast_expr_new_binop (CHIMP_BINOP_GTE, $1, $3, &@$); }
     | expr TOK_LT expr  { $$ = chimp_ast_expr_new_binop (CHIMP_BINOP_LT, $1, $3, &@$); }
     | expr TOK_LTE expr  { $$ = chimp_ast_expr_new_binop (CHIMP_BINOP_LTE, $1, $3, &@$); }
     | expr TOK_EQ expr  { $$ = chimp_ast_expr_new_binop (CHIMP_BINOP_EQ, $1, $3, &@$); }
     | expr TOK_NEQ expr { $$ = chimp_ast_expr_new_binop (CHIMP_BINOP_NEQ, $1, $3, &@$); }
     | expr TOK_PLUS expr     { $$ = chimp_ast_expr_new_binop (CHIMP_BINOP_ADD, $1, $3, &@$); }
     | expr TOK_MINUS expr    { $$ = chimp_ast_expr_new_binop (CHIMP_BINOP_SUB, $1, $3, &@$); }
     | expr TOK_ASTERISK expr { $$ = chimp_ast_expr_new_binop (CHIMP_BINOP_MUL, $1, $3, &@$); }
     | expr TOK_SLASH expr    { $$ = chimp_ast_expr_new_binop (CHIMP_BINOP_DIV, $1, $3, &@$); }
     | TOK_FN TOK_LBRACE TOK_PIPE opt_params TOK_PIPE opt_stmts TOK_RBRACE {
        $$ = chimp_ast_expr_new_fn ($4, $6, &@$); }
     | TOK_SPAWN TOK_LBRACE opt_stmts TOK_RBRACE {
        $$ = chimp_ast_expr_new_spawn (chimp_array_new (), $3, &@$);
     }
     | TOK_RECEIVE { $$ = chimp_ast_expr_new_receive (&@$); }
     | simple opt_simple_tail {
        $$ = $1;
        if ($2 != NULL) {
            $$ = $2;
            CHIMP_AST_EXPR_TARGETABLE_INNER($$, $1);
        }
     }
     ;

opt_patterns: pattern opt_patterns { $$ = $2; chimp_array_unshift ($$, $1); }
            | /* empty */ { $$ = chimp_array_new (); }
            ;

pattern: pattern_test TOK_COLON block { $$ = chimp_ast_stmt_new_pattern ($1, $3, &@$); }
       ;

pattern_test: simpler { $$ = $1; }
            | TOK_UNDERSCORE { $$ = chimp_ast_expr_new_wildcard (&@$); }
            | TOK_LSQBRACKET opt_pattern_array_elements TOK_RSQBRACKET {
                $$ = chimp_ast_expr_new_array ($2, &@$);
            }
            | TOK_LBRACE opt_pattern_hash_elements TOK_RBRACE {
                $$ = chimp_ast_expr_new_hash ($2, &@$);
            }
            ;

opt_pattern_array_elements : pattern_array_elements { $$ = $1; }
                           | /* empty */ { $$ = chimp_array_new (); }
                           ;

pattern_array_elements: pattern_test opt_pattern_array_elements_tail {
                          $$ = $2; chimp_array_unshift ($$, $1);
                      }
                      ;

opt_pattern_array_elements_tail:
            TOK_COMMA pattern_test opt_pattern_array_elements_tail {
              $$ = $3; chimp_array_unshift ($$, $2);
            }
            | /* empty */ { $$ = chimp_array_new (); }
            ;

opt_pattern_hash_elements : pattern_hash_elements { $$ = $1; }
                          | /* empty */ { $$ = chimp_array_new (); }
                          ;

pattern_hash_elements :
    pattern_test TOK_COLON pattern_test opt_pattern_hash_elements_tail {
        $$ = $4; chimp_array_unshift ($$, $3); chimp_array_unshift ($$, $1);
    }
    ;

opt_pattern_hash_elements_tail :
        TOK_COMMA pattern_test TOK_COLON pattern_test
            opt_pattern_hash_elements_tail
        {
            $$ = $5; chimp_array_unshift ($$, $4); chimp_array_unshift ($$, $2);
        }
        | /* empty */ { $$ = chimp_array_new (); }
        ;

simple : simpler { $$ = $1; }
       | array { $$ = $1; }
       | hash { $$ = $1; }
       ;

simpler: nil { $$ = $1; }
       | str { $$ = $1; }
       | int { $$ = $1; }
       | ident { $$ = $1; }
       | bool { $$ = $1; }
       ;

opt_simple_tail : TOK_LBRACKET opt_args TOK_RBRACKET opt_simple_tail {
                    ChimpRef *call;
                    call = chimp_ast_expr_new_call (NULL, $2, &@$);
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
                    getattr = chimp_ast_expr_new_getattr (NULL, CHIMP_AST_EXPR($2)->ident.id, &@$);
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

ident : TOK_IDENT { $$ = chimp_ast_expr_new_ident ($1, &@$); }
      ;

str : TOK_STR { $$ = chimp_ast_expr_new_str ($1, &@$); }
    ;

int : TOK_INT { $$ = chimp_ast_expr_new_int_ ($1, &@$); }
    ;

nil : TOK_NIL { $$ = chimp_ast_expr_new_nil (&@$); }
    ;

bool : TOK_TRUE { $$ = chimp_ast_expr_new_bool (chimp_true, &@$); }
     | TOK_FALSE { $$ = chimp_ast_expr_new_bool (chimp_false, &@$); }
     ;

hash : TOK_LBRACE hash_elements TOK_RBRACE { $$ = chimp_ast_expr_new_hash ($2, &@$); }
     | TOK_LBRACE TOK_RBRACE { $$ = chimp_array_new (); }
     ;

hash_elements : expr TOK_COLON expr opt_hash_elements_tail { $$ = $4; chimp_array_unshift ($$, $3); chimp_array_unshift ($$, $1); }
              ;

opt_hash_elements_tail : TOK_COMMA expr TOK_COLON expr opt_hash_elements_tail { $$ = $5; chimp_array_unshift ($$, $4); chimp_array_unshift ($$, $2); }
                       | /* empty */ { $$ = chimp_array_new (); }
                       ;

array : TOK_LSQBRACKET opt_array_elements TOK_RSQBRACKET { $$ = chimp_ast_expr_new_array ($2, &@$); }
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

panic : TOK_PANIC opt_expr { $$ = chimp_ast_stmt_new_panic ($2, &@$); }
      ;

ret : TOK_RET opt_expr { $$ = chimp_ast_stmt_new_ret ($2, &@$); }
    ;

%%

void
yyerror (const ChimpAstNodeLocation *loc, ChimpRef *filename, ChimpRef **mod, const char *format, ...)
{
    va_list args;

    *mod = NULL;

    fprintf (stderr, "[\033[1;31merror\033[0m] \033[1;34m%s\033[0m ["
                        "\033[1;33mline %" PRId64 ", "
                        "col %" PRId64 "\033[0m]: ",
                        CHIMP_STR_DATA(loc->filename),
                        loc->first_line, loc->first_column);
    va_start (args, format);
    vfprintf (stderr, format, args);
    va_end (args);
    fprintf (stderr, "\n");
    exit (1);
}

