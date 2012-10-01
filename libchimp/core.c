#include "chimp/core.h"
#include "chimp/gc.h"
#include "chimp/object.h"
#include "chimp/lwhash.h"
#include "chimp/class.h"
#include "chimp/str.h"
#include "chimp/array.h"
#include "chimp/method.h"

static ChimpGC *gc = NULL;

#define CHIMP_BOOTSTRAP_CLASS_L1(gc, c, n, sup) \
    do { \
        CHIMP_ANY(c)->type = CHIMP_VALUE_TYPE_CLASS; \
        CHIMP_ANY(c)->klass = chimp_class_class; \
        CHIMP_CLASS(c)->super = (sup); \
        CHIMP_CLASS(c)->name = chimp_gc_new_object ((gc)); \
        CHIMP_ANY(CHIMP_CLASS(c)->name)->type = CHIMP_VALUE_TYPE_STR; \
        CHIMP_ANY(CHIMP_CLASS(c)->name)->klass = chimp_str_class; \
        CHIMP_STR(CHIMP_CLASS(c)->name)->data = strndup ((n), (sizeof(n)-1)); \
        if (CHIMP_STR(CHIMP_CLASS(c)->name)->data == NULL) { \
            chimp_gc_delete (gc); \
            gc = NULL; \
            return CHIMP_FALSE; \
        } \
        CHIMP_STR(CHIMP_CLASS(c)->name)->size = sizeof(n)-1; \
        chimp_gc_make_root ((gc), (c)); \
    } while (0)

#define CHIMP_BOOTSTRAP_CLASS_L2(gc, c) \
    do { \
        CHIMP_CLASS(c)->methods = chimp_lwhash_new (); \
    } while (0)

ChimpRef *chimp_object_class = NULL;
ChimpRef *chimp_class_class = NULL;
ChimpRef *chimp_str_class = NULL;
ChimpRef *chimp_nil_class = NULL;

ChimpRef *chimp_nil = NULL;

static ChimpCmpResult
chimp_str_cmp (ChimpRef *a, ChimpRef *b)
{
    ChimpStr *as;
    ChimpStr *bs;
    ChimpValueType at, bt;

    if (a == b) {
        return CHIMP_CMP_EQ;
    }

    at = CHIMP_REF_TYPE(a);
    bt = CHIMP_REF_TYPE(b);

    if (at != CHIMP_VALUE_TYPE_STR) {
        /* TODO this is almost certainly a bug */
        return CHIMP_CMP_ERROR;
    }

    /* TODO should probably be a subtype check? */
    if (at != bt) {
        return CHIMP_CMP_NOT_IMPL;
    }

    as = CHIMP_STR(a);
    bs = CHIMP_STR(b);

    if (as->size != bs->size) {
        return strcmp (as->data, bs->data);
    }

    return memcmp (CHIMP_STR(a)->data, CHIMP_STR(b)->data, as->size);
}

static ChimpRef *
_chimp_object_str (ChimpGC *gc, ChimpRef *self)
{
    char buf[32];
    ChimpRef *name = CHIMP_CLASS_NAME(CHIMP_ANY_CLASS(self));
    ChimpRef *str;
    
    snprintf (buf, sizeof(buf), " @ %p>", self);
    str = chimp_str_new_concat (gc, "<", CHIMP_STR_DATA(name), buf, NULL);
    if (str == NULL) {
        return NULL;
    }
    return str;
}

static ChimpRef *
chimp_str_str (ChimpGC *gc, ChimpRef *self)
{
    return self;
}

static ChimpRef *
_chimp_object_getattr (ChimpRef *self, ChimpRef *name)
{
    /* TODO check CHIMP_ANY(self)->attributes ? */
    ChimpLWHash *methods = CHIMP_CLASS(CHIMP_ANY_CLASS(self))->methods;
    ChimpRef *method;
    if (!chimp_lwhash_get (methods, name, &method)) {
        return NULL;
    }
    /* XXX binding on every call is probably dumb/slow */
    return chimp_method_new_bound (method, self);
}

static ChimpRef *
chimp_class_getattr (ChimpRef *self, ChimpRef *name)
{
    ChimpLWHash *methods = CHIMP_CLASS(self)->methods;
    ChimpRef *method;
    if (!chimp_lwhash_get (methods, name, &method)) {
        return NULL;
    }
    return method;
}

static ChimpRef *
chimp_nil_str (ChimpGC *gc, ChimpRef *self)
{
    return CHIMP_CLASS_NAME(CHIMP_ANY_CLASS(self));
}

chimp_bool_t
chimp_core_startup (void)
{
    gc = chimp_gc_new ();
    if (gc == NULL) {
        return CHIMP_FALSE;
    }

    chimp_object_class = chimp_gc_new_object (gc);
    chimp_class_class  = chimp_gc_new_object (gc);
    chimp_str_class    = chimp_gc_new_object (gc);

    CHIMP_BOOTSTRAP_CLASS_L1(gc, chimp_object_class, "object", NULL);
    CHIMP_CLASS(chimp_object_class)->str = _chimp_object_str;
    CHIMP_CLASS(chimp_object_class)->getattr = _chimp_object_getattr;
    CHIMP_BOOTSTRAP_CLASS_L1(gc, chimp_class_class, "class", chimp_object_class);
    CHIMP_CLASS(chimp_class_class)->getattr = chimp_class_getattr;
    CHIMP_BOOTSTRAP_CLASS_L1(gc, chimp_str_class, "str", chimp_object_class);
    CHIMP_CLASS(chimp_str_class)->cmp = chimp_str_cmp;
    CHIMP_CLASS(chimp_str_class)->str = chimp_str_str;

    CHIMP_BOOTSTRAP_CLASS_L2(gc, chimp_object_class);
    CHIMP_BOOTSTRAP_CLASS_L2(gc, chimp_class_class);
    CHIMP_BOOTSTRAP_CLASS_L2(gc, chimp_str_class);

    chimp_nil_class = chimp_class_new (gc, CHIMP_STR_NEW(gc, "nil"), chimp_object_class);
    if (chimp_nil_class == NULL) {
        return CHIMP_FALSE;
    }
    chimp_gc_make_root (gc, chimp_nil_class);
    CHIMP_CLASS(chimp_nil_class)->str = chimp_nil_str;
    chimp_nil = chimp_object_new (gc, chimp_nil_class);
    if (chimp_nil == NULL) {
        return CHIMP_FALSE;
    }
    chimp_gc_make_root (gc, chimp_nil);

    if (!chimp_array_class_bootstrap (gc)) {
        return CHIMP_FALSE;
    }
    if (!chimp_method_class_bootstrap (gc)) {
        return CHIMP_FALSE;
    }

    return CHIMP_TRUE;
}

void
chimp_core_shutdown (void)
{
    if (gc != NULL) {
        chimp_gc_delete (gc);
        gc = NULL;
        chimp_object_class = NULL;
        chimp_class_class = NULL;
        chimp_str_class = NULL;
    }
}

void
chimp_bug (const char *filename, int lineno, const char *format, ...)
{
    va_list args;

    fprintf (stderr, "bug: %s:%d: ", filename, lineno);

    va_start (args, format);
    vfprintf (stderr, format, args);
    va_end (args);

    fprintf (stderr, ". aborting ...\n");

    abort ();
}

