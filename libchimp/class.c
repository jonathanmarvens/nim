#include "chimp/gc.h"
#include "chimp/object.h"
#include "chimp/class.h"
#include "chimp/lwhash.h"

#define CHIMP_CLASS_INIT(ref) \
    CHIMP_ANY(ref)->type = CHIMP_VALUE_TYPE_CLASS; \
    CHIMP_ANY(ref)->klass = chimp_class_class;

ChimpRef *
chimp_class_new (ChimpGC *gc, ChimpRef *name, ChimpRef *super)
{
    ChimpRef *ref = chimp_gc_new_object (gc);
    if (ref == NULL) {
        return NULL;
    }
    CHIMP_CLASS_INIT(ref);
    CHIMP_CLASS(ref)->name = name;
    CHIMP_CLASS(ref)->super = super;
    CHIMP_CLASS(ref)->methods = chimp_lwhash_new ();
    return ref;
}

