#ifdef HAVE_VALGRIND
#include <valgrind/memcheck.h>
#else
#define VALGRIND_MAKE_MEM_DEFINED(p, s)
#endif

#include "chimp/gc.h"
#include "chimp/object.h"
#include "chimp/core.h"
#include "chimp/lwhash.h"
#include "chimp/task.h"
#include "chimp/_parser.h"

#define DEFAULT_SLAB_SIZE 1500

struct _ChimpRef {
    chimp_bool_t marked;
    ChimpValue value;
    struct _ChimpRef *next;
};

#define CHIMP_FAST_ANY(ref) (&((ref)->value.any))
#define CHIMP_FAST_CLASS(ref) (&((ref)->value.klass))
#define CHIMP_FAST_MODULE(ref) (&((ref)->value.module))
#define CHIMP_FAST_STR(ref) (&((ref)->value.str))
#define CHIMP_FAST_ARRAY(ref) (&((ref)->value.array))
#define CHIMP_FAST_HASH(ref) (&((ref)->value.hash))
#define CHIMP_FAST_METHOD(ref) (&((ref)->value.method))
#define CHIMP_FAST_CODE(ref) (&((ref)->value.code))
#define CHIMP_FAST_FRAME(ref) (&((ref)->value.frame))
#define CHIMP_FAST_SYMTABLE(ref) (&((ref)->value.symtable))
#define CHIMP_FAST_SYMTABLE_ENTRY(ref) (&((ref)->value.symtable_entry))

#define CHIMP_FAST_REF_TYPE(ref) ((ref)->value.any.type)

typedef struct _ChimpSlab {
    ChimpRef *refs;
    void *head;
} ChimpSlab;

typedef struct _ChimpHeap {
    ChimpSlab **slabs;
    size_t      slab_count;
    size_t      slab_size;
    size_t      used;
} ChimpHeap;

struct _ChimpGC {
    ChimpHeap  heap;

    ChimpRef  *live;
    ChimpRef  *free;

    ChimpRef **roots;
    size_t     num_roots;

    void      *stack_start;
    uint64_t   collection_count;
};

static const char *
type_name (ChimpValueType type)
{
    switch (type) {
        case CHIMP_VALUE_TYPE_CLASS:
            {
                return "class";
            }
        case CHIMP_VALUE_TYPE_OBJECT:
            {
                return "object";
            }
        case CHIMP_VALUE_TYPE_STR:
            {
                return "str";
            }
        case CHIMP_VALUE_TYPE_CODE:
            {
                return "code";
            }
        case CHIMP_VALUE_TYPE_MODULE:
            {
                return "module";
            }
        case CHIMP_VALUE_TYPE_INT:
            {
                return "int";
            }
        case CHIMP_VALUE_TYPE_ARRAY:
            {
                return "array";
            }
        case CHIMP_VALUE_TYPE_HASH:
            {
                return "hash";
            }
        case CHIMP_VALUE_TYPE_METHOD:
            {
                return "method";
            }
        case CHIMP_VALUE_TYPE_FRAME:
            {
                return "stackframe";
            }
        case CHIMP_VALUE_TYPE_AST_MOD:
            {
                return "ast.mod";
            }
        case CHIMP_VALUE_TYPE_AST_STMT:
            {
                return "ast.stmt";
            }
        case CHIMP_VALUE_TYPE_AST_EXPR:
            {
                return "ast.expr";
            }
        case CHIMP_VALUE_TYPE_AST_DECL:
            {
                return "ast.decl";
            }
        case CHIMP_VALUE_TYPE_SYMTABLE:
            {
                return "symtable";
            }
        case CHIMP_VALUE_TYPE_TASK:
            {
                return "task";
            }
        case CHIMP_VALUE_TYPE_SYMTABLE_ENTRY:
            {
                return "symtable.entry";
            }
        default:
            {
                return "<unknown>";
            }
    };
}

#define CHIMP_HEAP_CURRENT_SLAB(heap) \
    (heap)->slabs[(heap)->used / ((heap)->slab_size * sizeof(ChimpRef))]

#define CHIMP_HEAP_ALLOCATED(heap) \
    ((heap)->slab_count * ((heap)->slab_size * sizeof(ChimpRef)))

static ChimpSlab *
chimp_slab_new (size_t size)
{
    ChimpRef *refs;
    size_t i;
    ChimpSlab *slab = CHIMP_MALLOC (ChimpSlab, sizeof (*slab) + sizeof(ChimpRef) * size);
    if (slab == NULL) {
        return NULL;
    }
    refs = (ChimpRef *)(((char *) slab) + sizeof (*slab));
    slab->head = refs;
    slab->refs = refs;
    if (size > 1) {
        for (i = 1; i < size; i++) {
            refs[i-1].next = refs + i;
        }
        refs[size-1].next = NULL;
    }
    else if (size == 1) {
        refs->next = NULL;
    }
    return slab;
}

static chimp_bool_t
chimp_heap_init (ChimpHeap *heap, size_t slab_size)
{
    heap->slabs = CHIMP_MALLOC (ChimpSlab *, sizeof (*heap->slabs));
    if (heap->slabs == NULL) {
        return CHIMP_FALSE;
    }
    heap->slabs[0] = chimp_slab_new (slab_size);
    if (heap->slabs[0] == NULL) {
        CHIMP_FREE (heap->slabs);
        return CHIMP_FALSE;
    }
    heap->slab_count = 1;
    heap->slab_size  = slab_size;
    heap->used       = 0;
    return CHIMP_TRUE;
}

static void
chimp_heap_destroy (ChimpHeap *heap)
{
    if (heap != NULL) {
        size_t i;
        for (i = 0; i < heap->slab_count; i++) {
            CHIMP_FREE (heap->slabs[i]);
        }
        CHIMP_FREE(heap->slabs);
    }
}

static chimp_bool_t
chimp_heap_grow (ChimpHeap *heap)
{
    ChimpSlab **slabs;
    ChimpSlab *slab;

    slab = chimp_slab_new (heap->slab_size);
    if (slab == NULL) {
        return CHIMP_FALSE;
    }
    slabs = CHIMP_REALLOC (ChimpSlab *, heap->slabs, sizeof (*heap->slabs) * (heap->slab_count + 1));
    if (slabs == NULL) {
        CHIMP_FREE (slab);
        return CHIMP_FALSE;
    }
    heap->slabs = slabs;
    slabs[heap->slab_count++] = slab;
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_heap_contains (ChimpHeap *heap, void *value)
{
    size_t i;
    for (i = 0; i < heap->slab_count; i++) {
        void *begin = heap->slabs[i]->head;
        void *end   = heap->slabs[i]->head + sizeof(ChimpRef) * heap->slab_size;
        chimp_bool_t is_base_ptr;
        
        /* is this a pointer to the *start* of a value/ref?
         * (We don't want to corrupt random bytes in the heap during a mark)
         */ 
        is_base_ptr = (end - value) % sizeof(ChimpRef) == 0;

        if (value >= begin && value < end && is_base_ptr) {
            return CHIMP_TRUE;
        }
    }
    return CHIMP_FALSE;
}

ChimpGC *
chimp_gc_new (void *stack_start)
{
    ChimpGC *gc = CHIMP_MALLOC (ChimpGC, sizeof (*gc));
    if (gc == NULL) {
        return NULL;
    }

    gc->stack_start = stack_start;

    if (!chimp_heap_init (&gc->heap, DEFAULT_SLAB_SIZE)) {
        CHIMP_FREE (gc);
        return NULL;
    }
    gc->live = NULL;
    gc->free = gc->heap.slabs[0]->refs;
    gc->roots = NULL;
    gc->num_roots = 0;
    gc->collection_count = 0;
    return gc;
}

static void
chimp_gc_value_dtor (ChimpGC *gc, ChimpRef *ref)
{
    if (!chimp_heap_contains (&gc->heap, ref)) {
        chimp_bug (__FILE__, __LINE__, "destructor called on value that belongs to another GC: %s", CHIMP_STR(chimp_object_str (ref))->data);
        return;
    }

    if (CHIMP_CLASS(CHIMP_ANY_CLASS(ref))->dtor) {
        CHIMP_CLASS(CHIMP_ANY_CLASS(ref))->dtor (ref);
    }
}

void
chimp_gc_delete (ChimpGC *gc)
{
    if (gc != NULL) {
        ChimpRef *live = gc->live;
        while (live != NULL) {
            ChimpRef *next = live->next;
            chimp_gc_value_dtor (gc, live);
            live = next;
        }
        gc->live = NULL;

        chimp_heap_destroy (&gc->heap);
        CHIMP_FREE (gc->roots);
        CHIMP_FREE (gc);
    }
}

ChimpRef *
chimp_gc_new_object (ChimpGC *gc)
{
    ChimpRef *ref;
    
    if (gc == NULL) {
        gc = CHIMP_CURRENT_GC;
    }

    if (gc->free == NULL) {
        if (!chimp_gc_collect (gc)) {
            ChimpSlab *slab;
            if (!chimp_heap_grow (&gc->heap)) {
                fprintf (stderr, "out of memory\n");
                abort ();
                return NULL;
            }
            slab = CHIMP_HEAP_CURRENT_SLAB(&gc->heap);
            gc->free = slab->refs;
        }
    }

    ref = gc->free;
    gc->free = gc->free->next;

    memset (ref, 0, sizeof(*ref));
    ref->marked = CHIMP_FALSE;
    ref->next = gc->live;
    gc->live = ref;
    gc->heap.used++;
    return ref;
}

chimp_bool_t
chimp_gc_make_root (ChimpGC *gc, ChimpRef *ref)
{
    ChimpRef **roots;

    if (gc == NULL) {
        gc = CHIMP_CURRENT_GC;
    }
    
    roots = CHIMP_REALLOC (ChimpRef *, gc->roots, sizeof (*gc->roots) * (gc->num_roots + 1));
    if (roots == NULL) {
        return CHIMP_FALSE;
    }
    gc->roots = roots;
    gc->roots[gc->num_roots++] = ref;
    return CHIMP_TRUE;
}

ChimpValue *
chimp_gc_ref_check_cast (ChimpRef *ref, ChimpValueType type)
{
    if (ref == NULL) {
        chimp_bug (__FILE__, __LINE__, "expected ref type '%s', but got a null ref", type_name (type));
        return NULL;
    }

    if (type == CHIMP_VALUE_TYPE_ANY) {
        return &ref->value;
    }

    if (ref->value.any.type != type) {
        chimp_bug (__FILE__, __LINE__, "expected ref type '%s', but got '%s'", type_name (type), type_name (ref->value.any.type));
        return NULL;
    }

    return &ref->value;
}

ChimpValueType
chimp_gc_ref_type (ChimpRef *ref)
{
    return ref->value.any.type;
}

static void
chimp_gc_mark_lwhash_items (ChimpLWHash *self, ChimpRef *key, ChimpRef *value, void *data);

void
chimp_gc_mark_ref (ChimpGC *gc, ChimpRef *ref)
{
    if (ref == NULL) return;

    if (gc == NULL) {
        gc = CHIMP_CURRENT_GC;
    }

    if (!chimp_heap_contains (&gc->heap, ref)) {
        /* this ref belongs to another GC */
        return;
    }

    if (ref->marked) return;
    ref->marked = CHIMP_TRUE;

    chimp_gc_mark_ref (gc, CHIMP_FAST_ANY(ref)->klass);
    switch (CHIMP_FAST_REF_TYPE(ref)) {
        case CHIMP_VALUE_TYPE_CLASS:
            {
                chimp_gc_mark_ref (gc, CHIMP_FAST_CLASS(ref)->super);
                chimp_gc_mark_ref (gc, CHIMP_FAST_CLASS(ref)->name);
                chimp_lwhash_foreach (CHIMP_FAST_CLASS(ref)->methods, chimp_gc_mark_lwhash_items, gc);
                break;
            }
        case CHIMP_VALUE_TYPE_MODULE:
            {
                chimp_gc_mark_ref (gc, CHIMP_FAST_MODULE(ref)->name);
                chimp_gc_mark_ref (gc, CHIMP_FAST_MODULE(ref)->locals);
                break;
            }
        case CHIMP_VALUE_TYPE_ARRAY:
            {
                size_t i;
                for (i = 0; i < CHIMP_FAST_ARRAY(ref)->size; i++) {
                    chimp_gc_mark_ref (gc, CHIMP_FAST_ARRAY(ref)->items[i]);
                }
                break;
            }
        case CHIMP_VALUE_TYPE_HASH:
            {
                size_t i;
                for (i = 0; i < CHIMP_FAST_HASH(ref)->size; i++) {
                    chimp_gc_mark_ref (gc, CHIMP_FAST_HASH(ref)->keys[i]);
                    chimp_gc_mark_ref (gc, CHIMP_FAST_HASH(ref)->values[i]);
                }
                break;
            }
        case CHIMP_VALUE_TYPE_METHOD:
            {
                chimp_gc_mark_ref (gc, CHIMP_FAST_METHOD(ref)->self);
                if (CHIMP_METHOD_TYPE(ref) == CHIMP_METHOD_TYPE_BYTECODE) {
                    chimp_gc_mark_ref (gc, CHIMP_FAST_METHOD(ref)->bytecode.code);
                }
                chimp_gc_mark_ref (gc, CHIMP_FAST_METHOD(ref)->module);
                /* TODO mark code object ? */
                break;
            }
        case CHIMP_VALUE_TYPE_AST_MOD:
            {
                chimp_ast_mod_mark (ref);
                break;
            }
        case CHIMP_VALUE_TYPE_AST_DECL:
            {
                chimp_ast_decl_mark (ref);
                break;
            }
        case CHIMP_VALUE_TYPE_AST_STMT:
            {
                chimp_ast_stmt_mark (ref);
                break;
            }
        case CHIMP_VALUE_TYPE_AST_EXPR:
            {
                chimp_ast_expr_mark (ref);
                break;
            }
        case CHIMP_VALUE_TYPE_CODE:
            {
                chimp_gc_mark_ref (gc, CHIMP_FAST_CODE(ref)->constants);
                chimp_gc_mark_ref (gc, CHIMP_FAST_CODE(ref)->names);
                break;
            }
        case CHIMP_VALUE_TYPE_FRAME:
            {
                chimp_gc_mark_ref (gc, CHIMP_FAST_FRAME(ref)->method);
                chimp_gc_mark_ref (gc, CHIMP_FAST_FRAME(ref)->locals);
                break;
            }
        case CHIMP_VALUE_TYPE_SYMTABLE:
            {
                chimp_gc_mark_ref (gc, CHIMP_FAST_SYMTABLE(ref)->filename);
                chimp_gc_mark_ref (gc, CHIMP_FAST_SYMTABLE(ref)->lookup);
                chimp_gc_mark_ref (gc, CHIMP_FAST_SYMTABLE(ref)->stack);
                chimp_gc_mark_ref (gc, CHIMP_FAST_SYMTABLE(ref)->current);
                break;
            }
        case CHIMP_VALUE_TYPE_SYMTABLE_ENTRY:
            {
                chimp_gc_mark_ref (gc, CHIMP_FAST_SYMTABLE_ENTRY(ref)->symtable);
                chimp_gc_mark_ref (gc, CHIMP_FAST_SYMTABLE_ENTRY(ref)->scope);
                chimp_gc_mark_ref (gc, CHIMP_FAST_SYMTABLE_ENTRY(ref)->symbols);
                chimp_gc_mark_ref (gc, CHIMP_FAST_SYMTABLE_ENTRY(ref)->varnames);
                chimp_gc_mark_ref (gc, CHIMP_FAST_SYMTABLE_ENTRY(ref)->parent);
                chimp_gc_mark_ref (gc, CHIMP_FAST_SYMTABLE_ENTRY(ref)->children);
                break;
            }
        case CHIMP_VALUE_TYPE_TASK:
        case CHIMP_VALUE_TYPE_OBJECT:
        case CHIMP_VALUE_TYPE_STR:
        case CHIMP_VALUE_TYPE_INT:
            break;
        default:
            chimp_bug (__FILE__, __LINE__, "unknown ref type '%s'", type_name (ref->value.any.type));
    };
}

static void
chimp_gc_mark_lwhash_items (ChimpLWHash *self, ChimpRef *key, ChimpRef *value, void *data)
{
    ChimpGC *gc = (ChimpGC *) data;
    chimp_gc_mark_ref (gc, key);
    chimp_gc_mark_ref (gc, value);
}

static size_t
chimp_gc_sweep (ChimpGC *gc)
{
    ChimpRef *ref = gc->live;
    ChimpRef *live = NULL;
    ChimpRef *free_ = NULL;
    size_t kept = 0;
    size_t freed = 0;

    while (ref != NULL) {
        ChimpRef *next = ref->next;
        if (!chimp_heap_contains (&gc->heap, ref)) {
            /* this ref belongs to another GC*/
            ref = next;
            continue;
        }
        if (ref->marked) {
            ref->next = live;
            live = ref;
            kept++;
        }
        else {
            chimp_gc_value_dtor (gc, ref);
            ref->next = free_;
            free_ = ref;
            freed++;
        }
        ref = next;
    }

    gc->live = live;
    gc->free = free_;
    gc->heap.used -= freed;
    return freed;
}

chimp_bool_t
chimp_gc_collect (ChimpGC *gc)
{
    ChimpRef *base;
    size_t i;
    ChimpRef *ref;
    
    if (gc == NULL) {
        gc = CHIMP_CURRENT_GC;
    }

    gc->collection_count++;

    ref = gc->live;
    while (ref != NULL) {
        ref->marked = CHIMP_FALSE;
        ref = ref->next;
    }

    for (i = 0; i < gc->num_roots; i++) {
        chimp_gc_mark_ref (gc, gc->roots[i]);
    }

    chimp_task_mark (gc, CHIMP_CURRENT_TASK);

    if (gc->stack_start != NULL) {
        void *ref_p = &base;
        /* XXX stack may grow in the other direction on some archs. */
        while (ref_p <= gc->stack_start) {
            /* STFU valgrind. */
            VALGRIND_MAKE_MEM_DEFINED(ref_p, sizeof(ChimpRef *));

            if (chimp_heap_contains (&gc->heap, *((ChimpRef **)ref_p))) {
                chimp_gc_mark_ref (gc, *((ChimpRef **)ref_p));
            }
            ref_p += sizeof(ref_p);
        }
    }

    return chimp_gc_sweep (gc) > 0;
}

uint64_t
chimp_gc_collection_count (ChimpGC *gc)
{
    if (gc == NULL) {
        gc = CHIMP_CURRENT_GC;
    }

    return gc->collection_count;
}

uint64_t
chimp_gc_num_live (ChimpGC *gc)
{
    if (gc == NULL) {
        gc = CHIMP_CURRENT_GC;
    }

    return gc->heap.used;
}

uint64_t
chimp_gc_num_free (ChimpGC *gc)
{
    if (gc == NULL) {
        gc = CHIMP_CURRENT_GC;
    }

    return (gc->heap.slab_count * gc->heap.slab_size) - gc->heap.used;
}

