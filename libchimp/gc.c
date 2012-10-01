#include "chimp/gc.h"
#include "chimp/object.h"
#include "chimp/core.h"
#include "chimp/lwhash.h"

#define DEFAULT_SLAB_SIZE ((4 * 1024) / sizeof(ChimpValue))

struct _ChimpRef {
    chimp_bool_t marked;
    ChimpValue *value;
    struct _ChimpRef *next;
};

#define CHIMP_FAST_ANY(ref) (&((ref)->value->any))
#define CHIMP_FAST_CLASS(ref) (&((ref)->value->klass))
#define CHIMP_FAST_STR(ref) (&((ref)->value->str))
#define CHIMP_FAST_ARRAY(ref) (&((ref)->value->array))
#define CHIMP_FAST_METHOD(ref) (&((ref)->value->method))

#define CHIMP_FAST_REF_TYPE(ref) ((ref)->value->any.type)

typedef struct _ChimpSlab {
    ChimpValue *values;
    ChimpValue *head;
} ChimpSlab;

typedef struct _ChimpHeap {
    ChimpSlab **slabs;
    size_t      slab_count;
    size_t      slab_size;
    size_t      used;
    size_t      allocated;
} ChimpHeap;

struct _ChimpGC {
    ChimpHeap  heaps[2];
    ChimpHeap *heap;
    ChimpRef  *live;
    ChimpRef **roots;
    size_t     num_roots;
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
        case CHIMP_VALUE_TYPE_ARRAY:
            {
                return "array";
            }
        case CHIMP_VALUE_TYPE_METHOD:
            {
                return "method";
            }
        default:
            {
                return "<unknown>";
            }
    };
}

static ChimpSlab *
chimp_slab_new (size_t size)
{
    ChimpSlab *slab = CHIMP_MALLOC (ChimpSlab, sizeof (*slab) + sizeof(ChimpValue) * size);
    if (slab == NULL) {
        return NULL;
    }
    slab->values = (ChimpValue *)(((char *) slab) + sizeof (*slab));
    slab->head = slab->values;
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
    heap->allocated  = slab_size;
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

static void
chimp_heap_copy_value (ChimpHeap *heap, ChimpValue *value)
{
    ChimpSlab *slab = heap->slabs[heap->used / heap->slab_size];
    memcpy (slab->values, value, sizeof (*value));
    slab->values++;
    heap->used++;
}

static void
chimp_heap_reset (ChimpHeap *heap)
{
    size_t i;
    for (i = 0; i < heap->slab_count; i++) {
        heap->slabs[i]->values = heap->slabs[i]->head;
    }
    heap->used = 0;
}

static chimp_bool_t
chimp_heap_contains (ChimpHeap *heap, ChimpValue *value)
{
    size_t i;
    for (i = 0; i < heap->slab_count; i++) {
        ChimpValue *begin = heap->slabs[i]->head;
        ChimpValue *end   = heap->slabs[i]->head + heap->slab_size;
        if (value >= begin && value < end) {
            return CHIMP_TRUE;
        }
    }
    return CHIMP_FALSE;
}

ChimpGC *
chimp_gc_new (void)
{
    ChimpGC *gc = CHIMP_MALLOC (ChimpGC, sizeof (*gc));
    if (gc == NULL) {
        return NULL;
    }
    if (!chimp_heap_init (&gc->heaps[0], DEFAULT_SLAB_SIZE)) {
        CHIMP_FREE (gc);
        return NULL;
    }
    if (!chimp_heap_init (&gc->heaps[1], DEFAULT_SLAB_SIZE)) {
        chimp_heap_destroy (&gc->heaps[0]);
        CHIMP_FREE (gc);
        return NULL;
    }
    gc->heap = &gc->heaps[0];
    gc->live = NULL;
    gc->roots = NULL;
    gc->num_roots = 0;
    return gc;
}

static void
chimp_gc_value_dtor (ChimpRef *ref)
{
    switch (CHIMP_FAST_REF_TYPE(ref)) {
        case CHIMP_VALUE_TYPE_STR:
            {
                CHIMP_FREE (CHIMP_FAST_STR(ref)->data);
                break;
            }
        case CHIMP_VALUE_TYPE_CLASS:
            {
                chimp_lwhash_delete (CHIMP_FAST_CLASS(ref)->methods);
                break;
            }
        case CHIMP_VALUE_TYPE_ARRAY:
            {
                CHIMP_FREE (CHIMP_FAST_ARRAY(ref)->items);
                break;
            }
        case CHIMP_VALUE_TYPE_METHOD:
        case CHIMP_VALUE_TYPE_OBJECT:
            break;
        default:
            chimp_bug (__FILE__, __LINE__, "unknown ref type: %s", type_name (ref->value->any.type));
            return;
    };
}

void
chimp_gc_delete (ChimpGC *gc)
{
    if (gc != NULL) {
        ChimpRef *live = gc->live;
        while (live != NULL) {
            ChimpRef *next = live->next;
            chimp_gc_value_dtor (live);
            CHIMP_FREE (live);
            live = next;
        }

        chimp_heap_destroy (&gc->heaps[0]);
        chimp_heap_destroy (&gc->heaps[1]);
        CHIMP_FREE (gc->roots);
        CHIMP_FREE (gc);
    }
}

static ChimpRef *
chimp_gc_new_ref (ChimpGC *gc, ChimpValue *value)
{
    ChimpRef *ref = CHIMP_MALLOC(ChimpRef, sizeof(*ref));
    if (ref == NULL) {
        return NULL;
    }
    ref->next = gc->live;
    ref->value = value;
    ref->marked = CHIMP_FALSE;
    gc->live = ref;
    return ref;
}

ChimpRef *
chimp_gc_new_object (ChimpGC *gc)
{
    ChimpValue *value;
    ChimpSlab *slab;
    
    if (gc->heap->used == gc->heap->allocated) {
        if (!chimp_gc_collect (gc)) {
            if (!chimp_heap_grow (&gc->heaps[0])) {
                fprintf (stderr, "out of memory\n");
                abort ();
                return NULL;
            }
            if (!chimp_heap_grow (&gc->heaps[1])) {
                fprintf (stderr, "out of memory\n");
                abort ();
                return NULL;
            }
        }
    }

    slab = gc->heap->slabs[gc->heap->used / gc->heap->slab_size];
    value = slab->values;
    memset (value, 0, sizeof(*value));
    slab->values++;
    gc->heap->used++;
    return chimp_gc_new_ref (gc, value);
}

chimp_bool_t
chimp_gc_make_root (ChimpGC *gc, ChimpRef *ref)
{
    ChimpRef **roots;
    
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

    if (ref->value == NULL) {
        chimp_bug (__FILE__, __LINE__, "expected ref type '%s', but got a ref with a null value", type_name (type));
        return NULL;
    }

    if (type == CHIMP_VALUE_TYPE_ANY) {
        return ref->value;
    }

    if (ref->value->any.type != type) {
        chimp_bug (__FILE__, __LINE__, "expected ref type '%s', but got '%s'", type_name (type), type_name (ref->value->any.type));
        return NULL;
    }

    return ref->value;
}

ChimpValueType
chimp_gc_ref_type (ChimpRef *ref)
{
    return ref->value->any.type;
}

static void
chimp_gc_mark_lwhash_items (ChimpLWHash *self, ChimpRef *key, ChimpRef *value, void *data);

static void
chimp_gc_mark_ref (ChimpGC *gc, ChimpRef *ref)
{
    if (ref == NULL) return;
    if (ref->marked) return;

    if (!chimp_heap_contains (gc->heap, ref->value)) {
        /* this ref belongs to another GC */
        return;
    }

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
        case CHIMP_VALUE_TYPE_ARRAY:
            {
                size_t i;
                for (i = 0; i < CHIMP_FAST_ARRAY(ref)->size; i++) {
                    chimp_gc_mark_ref (gc, CHIMP_FAST_ARRAY(ref)->items[i]);
                }
                break;
            }
        case CHIMP_VALUE_TYPE_METHOD:
            {
                chimp_gc_mark_ref (gc, CHIMP_FAST_METHOD(ref)->self);
                /* TODO mark code object ? */
                break;
            }
        case CHIMP_VALUE_TYPE_OBJECT:
        case CHIMP_VALUE_TYPE_STR:
            break;
        default:
            chimp_bug (__FILE__, __LINE__, "unknown ref type '%s'", type_name (ref->value->any.type));
    };
}

static void
chimp_gc_mark_lwhash_items (ChimpLWHash *self, ChimpRef *key, ChimpRef *value, void *data)
{
    ChimpGC *gc = (ChimpGC *) data;
    chimp_gc_mark_ref (gc, key);
    chimp_gc_mark_ref (gc, value);
}

static void
chimp_gc_sweep (ChimpGC *gc)
{
    ChimpHeap *dest = (gc->heap == &gc->heaps[0] ? &gc->heaps[1] : &gc->heaps[0]);
    ChimpRef *prev = NULL;
    ChimpRef *ref = gc->live;
    size_t kept = 0;
    size_t freed = 0;

    chimp_heap_reset (dest);
    while (ref != NULL) {
        ChimpRef *next = ref->next;
        if (!chimp_heap_contains (gc->heap, ref->value)) {
            /* this ref belongs to another GC*/
            ref = next;
            continue;
        }
        if (ref->marked) {
            chimp_heap_copy_value (dest, ref->value);
            kept++;
        }
        else {
            if (prev != NULL) {
                prev->next = next;
            }
            else {
                gc->live = next;
            }
            chimp_gc_value_dtor (ref);
            CHIMP_FREE (ref);
            freed++;
        }
        ref = next;
    }

    fprintf (stderr, "sweep complete! freed %lu, kept %lu\n", freed, kept);
}

chimp_bool_t
chimp_gc_collect (ChimpGC *gc)
{
    size_t i;
    ChimpRef *ref = gc->live;
    while (ref != NULL) {
        ref->marked = CHIMP_FALSE;
        ref = ref->next;
    }

    for (i = 0; i < gc->num_roots; i++) {
        chimp_gc_mark_ref (gc, gc->roots[i]);
    }

    chimp_gc_sweep (gc);

    return CHIMP_TRUE;
}

