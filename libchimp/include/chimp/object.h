#ifndef _CHIMP_OBJECT_H_INCLUDED_
#define _CHIMP_OBJECT_H_INCLUDED_

#include <chimp/core.h>
#include <chimp/gc.h>
#include <chimp/class.h>
#include <chimp/str.h>
#include <chimp/int.h>
#include <chimp/array.h>
#include <chimp/frame.h>
#include <chimp/code.h>
#include <chimp/method.h>
#include <chimp/hash.h>
#include <chimp/ast.h>
#include <chimp/module.h>
#include <chimp/symtable.h>
#include <chimp/msg.h>
#include <chimp/task.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ChimpObject {
    ChimpAny     base;
} ChimpObject;

typedef union _ChimpValue {
    ChimpAny            any;
    ChimpClass          klass;
    ChimpObject         object;
    ChimpModule         module;
    ChimpStr            str;
    ChimpInt            integer;
    ChimpArray          array;
    ChimpHash           hash;
    ChimpCode           code;
    ChimpMethod         method;
    ChimpFrame          frame;
    ChimpSymtable       symtable;
    ChimpSymtableEntry  symtable_entry;
    ChimpAstMod         ast_mod;
    ChimpAstDecl        ast_decl;
    ChimpAstStmt        ast_stmt;
    ChimpAstExpr        ast_expr;
} ChimpValue;

ChimpCmpResult
chimp_object_cmp (ChimpRef *a, ChimpRef *b);

ChimpRef *
chimp_object_str (ChimpRef *self);

ChimpRef *
chimp_object_add (ChimpRef *left, ChimpRef *right);

ChimpRef *
chimp_object_sub (ChimpRef *left, ChimpRef *right);

ChimpRef *
chimp_object_mul (ChimpRef *left, ChimpRef *right);

ChimpRef *
chimp_object_div (ChimpRef *left, ChimpRef *right);

ChimpRef *
chimp_object_call (ChimpRef *target, ChimpRef *args);

ChimpRef *
chimp_object_getattr (ChimpRef *self, ChimpRef *name);

ChimpRef *
chimp_object_getitem (ChimpRef *self, ChimpRef *key);

ChimpRef *
chimp_object_getattr_str (ChimpRef *self, const char *name);

chimp_bool_t
chimp_object_instance_of (ChimpRef *object, ChimpRef *klass);

#define CHIMP_OBJECT(ref) CHIMP_CHECK_CAST(ChimpObject, (ref), CHIMP_VALUE_TYPE_OBJECT)

CHIMP_EXTERN_CLASS(object);
CHIMP_EXTERN_CLASS(bool);
CHIMP_EXTERN_CLASS(nil);

extern struct _ChimpRef *chimp_nil;
extern struct _ChimpRef *chimp_true;
extern struct _ChimpRef *chimp_false;
extern struct _ChimpRef *chimp_builtins;

#define CHIMP_BOOL_REF(b) ((b) ? chimp_true : chimp_false)

#define CHIMP_OBJECT_STR(ref) CHIMP_STR_DATA(chimp_object_str (NULL, (ref)))

#ifdef __cplusplus
};
#endif

#endif

