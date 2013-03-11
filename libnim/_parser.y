%{

#include <inttypes.h>

#include "nim/object.h"
#include "nim/_parser_ext.h"

void yyerror(const NimAstNodeLocation *loc, void *scanner, NimRef *filename, NimRef **mod, const char *format, ...);

#ifdef NIM_PARSER_DEBUG
#define YYDEBUG 1
#endif

%}

%union {
    NimRef *ref;
}

/* token location support for tracking lineno & column info */
%locations

/* necessary to support %locations properly afaict */
/* Bison 2.4 adds api.pure & deprecates %pure-parser but OSX has 2.3 ... */
/*%define api.pure*/
%pure-parser

/* better error messages for free */
%error-verbose

%parse-param { void *scanner }
%parse-param { NimRef *filename }
/* We pass this around so we can avoid using a global */
/* XXX odd. doing this seems to trigger %locations for yyerror too */
/*     handy, but unexpected. */
%parse-param { NimRef **mod }
%lex-param { void *scanner }
%lex-param { NimRef *filename }
%lex-param { NimRef **mod }

/* setting a custom YYLTYPE means bison no longer initializes yylval for us */
%initial-action {
    memset (&@$, 0, sizeof(@$));
    @$.filename = filename;
    @$.first_line = @$.last_line = 1;
    @$.first_column = @$.last_column = 1;
}

%{
extern int yylex(YYSTYPE *lvalp, YYLTYPE *llocp, void *scanner, NimRef *filename, NimRef **mod);
%}

%expect 0

%token TOK_TRUE "`true`"
%token TOK_FALSE "`false`"
%token TOK_NIL "`nil`"
%token TOK_LBRACKET "`(`"
%token TOK_RBRACKET "`)`"
%token TOK_SEMICOLON "`;`"
%token TOK_COMMA "`,`"
%token TOK_COLON "`:`"
%token TOK_FULLSTOP "`.`"
%token TOK_LSQBRACKET "`[`"
%token TOK_RSQBRACKET "`]`"
%token TOK_LBRACE "`{`"
%token TOK_RBRACE "`}`"
%token TOK_PIPE "`|`"
%token TOK_ASSIGN "`=`"
%token TOK_IF "`if`"
%token TOK_ELSE "`else`"
%token TOK_USE "`use`"
%token TOK_RET "`ret`"
%token TOK_FN "`fn`"
%token TOK_VAR "`var`"
%token TOK_WHILE "`while`"
%token TOK_SPAWN "`spawn`"
%token TOK_NOT "`not`"
%token TOK_MATCH "`match`"
%token TOK_BREAK "`break`"
%token TOK_NEWLINE "newline"
%token TOK_UNDERSCORE "`_`"

%token TOK_OR "`or`"
%token TOK_AND "`and`"
%token TOK_NEQ "`!=`"
%token TOK_EQ "`==`"
%token TOK_LT "`<`"
%token TOK_LTE "`<=`"
%token TOK_GT "`>`"
%token TOK_GTE "`>=`"
%token TOK_PLUS "`+`"
%token TOK_MINUS "`-`"
%token TOK_ASTERISK "`*`"
%token TOK_SLASH "`/`"
%token TOK_CLASS "`class`"

%token <ref> TOK_IDENT "identifier"
%token <ref> TOK_STR "string literal"
%token <ref> TOK_INT "integer literal"
%token <ref> TOK_FLOAT "float literal"

%right TOK_NOT TOK_SPAWN
%nonassoc TOK_LSQBRACKET TOK_LBRACKET TOK_FULLSTOP
%nonassoc TOK_OR TOK_AND
%left TOK_NEQ TOK_EQ TOK_LT TOK_LTE TOK_GT TOK_GTE
%left TOK_PLUS TOK_MINUS
%left TOK_ASTERISK TOK_SLASH

%type <ref> module
%type <ref> stmt single_stmt simple_stmt compound_stmt
%type <ref> var_decl assign
%type <ref> stmts opt_stmts block else
%type <ref> opt_expr expr expr2 expr3 expr4 expr5 
%type <ref> simple_tail
%type <ref> opt_decls opt_uses
%type <ref> use
%type <ref> func_decl class_decl func_ident
%type <ref> opt_extends type_name opt_class_decls
%type <ref> opt_params opt_params_tail param
%type <ref> opt_args args opt_args_tail
%type <ref> opt_patterns pattern pattern_test opt_pattern_array_elements
%type <ref> pattern_array_elements opt_pattern_array_elements_tail
%type <ref> opt_array_elements array_elements opt_array_elements_tail
%type <ref> hash_elements opt_hash_elements_tail
%type <ref> opt_pattern_hash_elements pattern_hash_elements
%type <ref> opt_pattern_hash_elements_tail
%type <ref> ident str array hash bool nil int float
%type <ref> ret break

%%

module : opt_newline opt_uses opt_decls { *mod = nim_ast_mod_new_root (NIM_STR_NEW("main"), $2, $3, &@$); }
       ;

opt_newline : TOK_NEWLINE
            | /* empty */
            ;

opt_uses : use opt_uses { $$ = $2; nim_array_unshift ($$, $1); }
         | /* empty */ { $$ = nim_array_new (); }
         ;

use : TOK_USE ident TOK_NEWLINE { $$ = nim_ast_decl_new_use (NIM_AST_EXPR($2)->ident.id, &@$); }
    ;

opt_decls : func_decl opt_newline opt_decls { $$ = $3; nim_array_unshift ($$, $1); }
          | class_decl opt_newline opt_decls { $$ = $3; nim_array_unshift ($$, $1); }
          | /* empty */ { $$ = nim_array_new (); }
          ;

opt_class_decls : func_decl opt_class_decls { $$ = $2; nim_array_unshift ($$, $1); }
                | /* empty */ { $$ = nim_array_new (); }
                ;

class_decl : TOK_CLASS ident opt_extends TOK_LBRACE TOK_NEWLINE opt_class_decls TOK_RBRACE {
            $$ = nim_ast_decl_new_class (
                NIM_AST_EXPR($2)->ident.id, $3, $6, &@$);
           }
           ;

opt_extends : TOK_COLON type_name { $$ = $2; }
            | /* empty */ { $$ = nim_array_new (); }
            ;

type_name : ident TOK_FULLSTOP type_name {
            $$ = $3;
            nim_array_unshift ($$, NIM_AST_EXPR($1)->ident.id);
          }
          | ident { $$ = nim_array_new_var (NIM_AST_EXPR($1)->ident.id, NULL); }
          ;

func_decl : func_ident opt_params TOK_LBRACE TOK_NEWLINE opt_stmts TOK_RBRACE {
            $$ = nim_ast_decl_new_func (NIM_AST_EXPR($1)->ident.id, $2, $5, &@$);
          }
          ;

opt_params : param opt_params_tail { $$ = $2; nim_array_unshift ($$, $1); }
           | /* empty */ { $$ = nim_array_new (); }
           ;

opt_params_tail : TOK_COMMA param opt_params_tail { $$ = $3; nim_array_unshift ($$, $2); }
                 | /* empty */ { $$ = nim_array_new (); }
                 ;

param : ident { $$ = nim_ast_decl_new_var (NIM_AST_EXPR($1)->ident.id, NULL, &@$); }
      ;

stmts: stmt opt_stmts { $$ = $2; nim_array_unshift ($$, $1); }
     ;

opt_stmts : stmt opt_stmts { $$ = $2; nim_array_unshift ($$, $1); }
          | /* empty */ { $$ = nim_array_new (); }
          ;

single_stmt : simple_stmt { $$ = $1; }
            | compound_stmt { $$ = $1; }
            ;

stmt : simple_stmt TOK_NEWLINE { $$ = $1; }
     | compound_stmt TOK_NEWLINE { $$ = $1; }
     ;

simple_stmt : expr { $$ = nim_ast_stmt_new_expr ($1, &@$); }
            | var_decl { $$ = $1; }
            | assign { $$ = $1; }
            | ret { $$ = $1; }
            | break { $$ = $1; }
            ;

compound_stmt : TOK_IF expr block else { $$ = nim_ast_stmt_new_if_ ($2, $3, $4, &@$); }
              | TOK_IF expr block { $$ = nim_ast_stmt_new_if_ ($2, $3, NULL, &@$); }
              | TOK_WHILE expr block { $$ = nim_ast_stmt_new_while_ ($2, $3, &@$); }
              | TOK_MATCH expr TOK_LBRACE TOK_NEWLINE opt_patterns TOK_RBRACE {
                    $$ = nim_ast_stmt_new_match ($2, $5, &@$);
              }
              ;

else : TOK_ELSE block { $$ = $2; }

block : TOK_LBRACE TOK_NEWLINE stmts TOK_RBRACE { $$ = $3; }
      | TOK_LBRACE single_stmt TOK_RBRACE { $$ = nim_array_new_var ($2, NULL); }
      | TOK_LBRACE TOK_RBRACE { $$ = nim_array_new (); }
      /* | stmt { $$ = nim_array_new (); nim_array_unshift ($$, $1); } */
      | TOK_NEWLINE { $$ = nim_array_new (); }
      ;

var_decl : TOK_VAR ident { $$ = nim_ast_decl_new_var (NIM_AST_EXPR($2)->ident.id, NULL, &@$); }
         | TOK_VAR ident TOK_ASSIGN expr {
            $$ = nim_ast_decl_new_var (NIM_AST_EXPR($2)->ident.id, $4, &@$);
         }
         ;

assign : ident TOK_ASSIGN expr { $$ = nim_ast_stmt_new_assign ($1, $3, &@$); }
       ;

expr : expr TOK_OR expr2  { $$ = nim_ast_expr_new_binop (NIM_BINOP_OR, $1, $3, &@$); }
     | expr TOK_AND expr2 { $$ = nim_ast_expr_new_binop (NIM_BINOP_AND, $1, $3, &@$); }
     | expr TOK_GT expr2  { $$ = nim_ast_expr_new_binop (NIM_BINOP_GT, $1, $3, &@$); }
     | expr TOK_GTE expr2  { $$ = nim_ast_expr_new_binop (NIM_BINOP_GTE, $1, $3, &@$); }
     | expr TOK_LT expr2  { $$ = nim_ast_expr_new_binop (NIM_BINOP_LT, $1, $3, &@$); }
     | expr TOK_LTE expr2  { $$ = nim_ast_expr_new_binop (NIM_BINOP_LTE, $1, $3, &@$); }
     | expr TOK_EQ expr2  { $$ = nim_ast_expr_new_binop (NIM_BINOP_EQ, $1, $3, &@$); }
     | expr TOK_NEQ expr2 { $$ = nim_ast_expr_new_binop (NIM_BINOP_NEQ, $1, $3, &@$); }
     | expr TOK_PLUS expr2     { $$ = nim_ast_expr_new_binop (NIM_BINOP_ADD, $1, $3, &@$); }
     | expr TOK_MINUS expr2    { $$ = nim_ast_expr_new_binop (NIM_BINOP_SUB, $1, $3, &@$); }
     | expr TOK_ASTERISK expr2 { $$ = nim_ast_expr_new_binop (NIM_BINOP_MUL, $1, $3, &@$); }
     | expr TOK_SLASH expr2    { $$ = nim_ast_expr_new_binop (NIM_BINOP_DIV, $1, $3, &@$); }
     | expr2 { $$ = $1; }
     ;

expr2: expr2 simple_tail {
        $$ = $2;
        NIM_AST_EXPR_TARGETABLE_INNER($$, $1);
     }
     | expr3 { $$ = $1; }
     ;

expr3: TOK_NOT expr2 { $$ = nim_ast_expr_new_not ($2, &@$); }
     | TOK_FN TOK_LBRACE TOK_PIPE opt_params TOK_PIPE TOK_NEWLINE opt_stmts TOK_RBRACE {
        $$ = nim_ast_expr_new_fn ($4, $7, &@$);
    }
     | TOK_FN TOK_LBRACE TOK_PIPE opt_params TOK_PIPE single_stmt TOK_RBRACE {
        $$ = nim_ast_expr_new_fn ($4, nim_array_new_var ($6, NULL), &@$);
     }
     | TOK_FN TOK_LBRACE TOK_NEWLINE opt_stmts TOK_RBRACE {
        $$ = nim_ast_expr_new_fn (nim_array_new (), $4, &@$);
     }
     | TOK_FN TOK_LBRACE single_stmt TOK_RBRACE {
        $$ = nim_ast_expr_new_fn (nim_array_new (), nim_array_new_var ($3, NULL), &@$);
     }
     | TOK_SPAWN expr2 {
        NimRef *target = NIM_AST_EXPR($2)->call.target;
        NimRef *args = NIM_AST_EXPR($2)->call.args;
        $$ = nim_ast_expr_new_spawn(target, args, &@$);
     }
     | expr4 { $$ = $1; };
     ;

expr4 : expr5 { $$ = $1; }
       | array { $$ = $1; }
       | hash { $$ = $1; }
       ;

expr5: nil { $$ = $1; }
       | str { $$ = $1; }
       | int { $$ = $1; }
       | float { $$ = $1; }
       | ident { $$ = $1; }
       | bool { $$ = $1; }
       | TOK_LBRACKET expr TOK_RBRACKET { $$ = $2; }
       ;

opt_patterns: pattern opt_patterns { $$ = $2; nim_array_unshift ($$, $1); }
            | /* empty */ { $$ = nim_array_new (); }
            ;

pattern: pattern_test block TOK_NEWLINE { $$ = nim_ast_stmt_new_pattern ($1, $2, &@$); }
       ;

pattern_test: expr5 { $$ = $1; }
            | TOK_UNDERSCORE { $$ = nim_ast_expr_new_wildcard (&@$); }
            | TOK_LSQBRACKET opt_pattern_array_elements TOK_RSQBRACKET {
                $$ = nim_ast_expr_new_array ($2, &@$);
            }
            | TOK_LBRACE opt_pattern_hash_elements TOK_RBRACE {
                $$ = nim_ast_expr_new_hash ($2, &@$);
            }
            ;

opt_pattern_array_elements : pattern_array_elements { $$ = $1; }
                           | /* empty */ { $$ = nim_array_new (); }
                           ;

pattern_array_elements: pattern_test opt_pattern_array_elements_tail {
                          $$ = $2; nim_array_unshift ($$, $1);
                      }
                      ;

opt_pattern_array_elements_tail:
            TOK_COMMA pattern_test opt_pattern_array_elements_tail {
              $$ = $3; nim_array_unshift ($$, $2);
            }
            | /* empty */ { $$ = nim_array_new (); }
            ;

opt_pattern_hash_elements : pattern_hash_elements { $$ = $1; }
                          | /* empty */ { $$ = nim_array_new (); }
                          ;

pattern_hash_elements :
    pattern_test TOK_COLON pattern_test opt_pattern_hash_elements_tail {
        $$ = $4; nim_array_unshift ($$, $3); nim_array_unshift ($$, $1);
    }
    ;

opt_pattern_hash_elements_tail :
        TOK_COMMA pattern_test TOK_COLON pattern_test
            opt_pattern_hash_elements_tail
        {
            $$ = $5; nim_array_unshift ($$, $4); nim_array_unshift ($$, $2);
        }
        | /* empty */ { $$ = nim_array_new (); }
        ;

simple_tail : TOK_LBRACKET opt_args TOK_RBRACKET {
                $$ = nim_ast_expr_new_call (NULL, $2, &@$);
            }
            | TOK_LSQBRACKET expr TOK_RSQBRACKET {
                $$ = nim_ast_expr_new_getitem (NULL, $2, &@$);
            }
            | TOK_FULLSTOP ident {
                $$ = nim_ast_expr_new_getattr (NULL, NIM_AST_EXPR($2)->ident.id, &@$);
            }
            ;

opt_args : args        { $$ = $1; }
         | /* empty */ { $$ = nim_array_new (); }
         ;

args : expr opt_args_tail { $$ = $2; nim_array_unshift ($$, $1); }
     ;

opt_args_tail : TOK_COMMA expr opt_args_tail { $$ = $3; nim_array_unshift ($$, $2); }
              | /* empty */ { $$ = nim_array_new (); }
              ;

func_ident : TOK_IDENT { $$ = nim_ast_expr_new_ident ($1, &@$); }
           ;

ident : TOK_IDENT { $$ = nim_ast_expr_new_ident ($1, &@$); }
      | TOK_CLASS { $$ = nim_ast_expr_new_ident (NIM_STR_NEW("class"), &@$); }
      ;

str : TOK_STR { $$ = nim_ast_expr_new_str ($1, &@$); }
    ;

int : TOK_INT { $$ = nim_ast_expr_new_int_ ($1, &@$); }
    ;

float : TOK_FLOAT { $$ = nim_ast_expr_new_float_ ($1, &@$); }
    ;

nil : TOK_NIL { $$ = nim_ast_expr_new_nil (&@$); }
    ;

bool : TOK_TRUE { $$ = nim_ast_expr_new_bool (nim_true, &@$); }
     | TOK_FALSE { $$ = nim_ast_expr_new_bool (nim_false, &@$); }
     ;

hash : TOK_LBRACE hash_elements TOK_RBRACE { $$ = nim_ast_expr_new_hash ($2, &@$); }
     | TOK_LBRACE TOK_RBRACE { $$ = nim_ast_expr_new_hash (nim_array_new (), &@$); }
     ;

hash_elements : expr TOK_COLON expr opt_hash_elements_tail { $$ = $4; nim_array_unshift ($$, $3); nim_array_unshift ($$, $1); }
              ;

opt_hash_elements_tail : TOK_COMMA expr TOK_COLON expr opt_hash_elements_tail { $$ = $5; nim_array_unshift ($$, $4); nim_array_unshift ($$, $2); }
                       | /* empty */ { $$ = nim_array_new (); }
                       ;

array : TOK_LSQBRACKET opt_array_elements TOK_RSQBRACKET { $$ = nim_ast_expr_new_array ($2, &@$); }
      | TOK_LSQBRACKET TOK_NEWLINE opt_array_elements TOK_NEWLINE TOK_RSQBRACKET { $$ = nim_ast_expr_new_array ($3, &@$); }
      ;

opt_array_elements : array_elements { $$ = $1; }
                   | /* empty */ { $$ = nim_array_new (); }
                   ;

array_elements : expr opt_array_elements_tail { $$ = $2; nim_array_unshift ($$, $1); }
               ;

opt_array_elements_tail : TOK_COMMA expr opt_array_elements_tail { $$ = $3; nim_array_unshift ($$, $2); }
                        | /* empty */ { $$ = nim_array_new (); }
                        ;

opt_expr : expr { $$ = $1; }
         | /* empty */ { $$ = NULL; }
         ;

break : TOK_BREAK { $$ = nim_ast_stmt_new_break_ (&@$); }
      ;

ret : TOK_RET opt_expr { $$ = nim_ast_stmt_new_ret ($2, &@$); }
    ;

%%

void
yyerror (const NimAstNodeLocation *loc, void *scanner, NimRef *filename, NimRef **mod, const char *format, ...)
{
    va_list args;

    *mod = NULL;

    fprintf (stderr, "error:%s:%zu:%zu: ",
                        NIM_STR_DATA(loc->filename),
                        (intmax_t) loc->first_line, (intmax_t) loc->first_column);
    va_start (args, format);
    vfprintf (stderr, format, args);
    va_end (args);
    fprintf (stderr, "\n");
    exit (1);
}

