/*****************************************************************************
 *                                                                           *
 * Copyright 2012 Thomas Lee                                                 *
 *                                                                           *
 * Licensed under the Apache License, Version 2.0 (the "License");           *
 * you may not use this file except in compliance with the License.          *
 * You may obtain a copy of the License at                                   *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 * Unless required by applicable law or agreed to in writing, software       *
 * distributed under the License is distributed on an "AS IS" BASIS,         *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
 * See the License for the specific language governing permissions and       *
 * limitations under the License.                                            *
 *                                                                           *
 *****************************************************************************/

#ifdef HAVE_VALGRIND
#include <valgrind/memcheck.h>
#else
#define VALGRIND_MAKE_MEM_DEFINED(p, s)
#endif

#include "nim/gc.h"
#include "nim/object.h"
#include "nim/core.h"
#include "nim/task.h"
#include "nim/_parser.h"

/* 64k default heap (NIM_VALUE_SIZE * DEFAULT_SLAB_SIZE) */
#define DEFAULT_SLAB_SIZE 256

struct _NimRef {
    nim_bool_t marked;
    struct _NimRef *next;
    char value[NIM_VALUE_SIZE];
};

#define NIM_FAST_ANY(ref) ((NimAny *)(ref)->value)

typedef struct _NimSlab {
    NimRef *refs;
    void *head;
} NimSlab;

typedef struct _NimHeap {
    NimSlab **slabs;
    size_t      slab_count;
    size_t      slab_size;
    size_t      used;
} NimHeap;

struct _NimGC {
    NimHeap  heap;

    NimRef  *live;
    NimRef  *free;

    NimRef **roots;
    size_t     num_roots;

    void      *stack_start;
    uint64_t   collection_count;
};

#define NIM_HEAP_CURRENT_SLAB(heap) \
    (heap)->slabs[(heap)->slab_count - 1]

#define NIM_HEAP_ALLOCATED(heap) \
    ((heap)->slab_count * ((heap)->slab_size * sizeof(NimRef)))

static NimSlab *
nim_slab_new (size_t size)
{
    NimRef *refs;
    size_t i;
    NimSlab *slab =
      NIM_MALLOC (NimSlab, sizeof (*slab) + sizeof(NimRef) * size);
    if (slab == NULL) {
        return NULL;
    }
    refs = (NimRef *)(((char *) slab) + sizeof (*slab));
    slab->head = refs;
    slab->refs = refs;
    /* TODO don't think we need the conditional check */
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

static nim_bool_t
nim_heap_init (NimHeap *heap, size_t slab_size)
{
    heap->slabs = NIM_MALLOC (NimSlab *, sizeof (*heap->slabs));
    if (heap->slabs == NULL) {
        return NIM_FALSE;
    }
    heap->slabs[0] = nim_slab_new (slab_size);
    if (heap->slabs[0] == NULL) {
        NIM_FREE (heap->slabs);
        return NIM_FALSE;
    }
    heap->slab_count = 1;
    heap->slab_size  = slab_size;
    heap->used       = 0;
    return NIM_TRUE;
}

static void
nim_heap_destroy (NimHeap *heap)
{
    if (heap != NULL) {
        size_t i;
        for (i = 0; i < heap->slab_count; i++) {
            NIM_FREE (heap->slabs[i]);
        }
        NIM_FREE(heap->slabs);
    }
}

static nim_bool_t
nim_heap_grow (NimHeap *heap)
{
    NimSlab **slabs;
    NimSlab *slab;

    slab = nim_slab_new (heap->slab_size);
    if (slab == NULL) {
        return NIM_FALSE;
    }
    slabs = NIM_REALLOC (
        NimSlab *, heap->slabs,
        sizeof (*heap->slabs) * (heap->slab_count + 1));
    if (slabs == NULL) {
        NIM_FREE (slab);
        return NIM_FALSE;
    }
    heap->slabs = slabs;
    slabs[heap->slab_count++] = slab;
    return NIM_TRUE;
}

static nim_bool_t
nim_heap_contains (NimHeap *heap, void *value)
{
    size_t i;
    for (i = 0; i < heap->slab_count; i++) {
        void *begin = heap->slabs[i]->head;
        void *end   = heap->slabs[i]->head + sizeof(NimRef) * heap->slab_size;
        
        if (value >= begin && value < end) {
            /* is this a pointer to the *start* of a value/ref?
             * (We don't want to corrupt random bytes in the heap during a mark)
             */
            if ((end - value) % sizeof(NimRef) == 0) {
                return NIM_TRUE;
            }
        }
    }
    return NIM_FALSE;
}

NimGC *
nim_gc_new (void *stack_start)
{
    NimGC *gc = NIM_MALLOC (NimGC, sizeof (*gc));
    if (gc == NULL) {
        return NULL;
    }

    memset (gc, 0, sizeof (*gc));

    gc->stack_start = stack_start;

    if (!nim_heap_init (&gc->heap, DEFAULT_SLAB_SIZE)) {
        NIM_FREE (gc);
        return NULL;
    }
    gc->free = gc->heap.slabs[0]->head;
    return gc;
}

static void
nim_gc_value_dtor (NimGC *gc, NimRef *ref)
{
    if (!nim_heap_contains (&gc->heap, ref)) {
        NIM_BUG ("destructor called on value that belongs to another GC: %s",
                NIM_STR(nim_object_str (ref))->data);
        return;
    }

    if (NIM_CLASS(NIM_ANY_CLASS(ref))->dtor) {
        NIM_CLASS(NIM_ANY_CLASS(ref))->dtor (ref);
    }
}

void
nim_gc_delete (NimGC *gc)
{
    if (gc != NULL) {
        NimRef *live = gc->live;
        while (live != NULL) {
            NimRef *next = live->next;
            nim_gc_value_dtor (gc, live);
            live = next;
        }
        gc->live = NULL;

        nim_heap_destroy (&gc->heap);
        NIM_FREE (gc->roots);
        NIM_FREE (gc);
    }
}

NimRef *
nim_gc_new_object (NimGC *gc)
{
    NimRef *ref;
    
    if (gc == NULL) {
        gc = NIM_CURRENT_GC;
    }

    if (gc->free == NULL) {
        if (!nim_gc_collect (gc)) {
            NimSlab *slab;
            if (!nim_heap_grow (&gc->heap)) {
                NIM_BUG ("out of memory");
                return NULL;
            }
            slab = NIM_HEAP_CURRENT_SLAB(&gc->heap);
            gc->free = slab->head;
        }
    }

    ref = gc->free;
    gc->free = gc->free->next;

    memset (ref, 0, sizeof(*ref));
    ref->next = gc->live;
    gc->live = ref;
    gc->heap.used++;
    return ref;
}

nim_bool_t
nim_gc_make_root (NimGC *gc, NimRef *ref)
{
    NimRef **roots;

    if (gc == NULL) {
        gc = NIM_CURRENT_GC;
    }
    
    roots = NIM_REALLOC (NimRef *, gc->roots, sizeof (*gc->roots) * (gc->num_roots + 1));
    if (roots == NULL) {
        return NIM_FALSE;
    }
    gc->roots = roots;
    gc->roots[gc->num_roots++] = ref;
    return NIM_TRUE;
}

void *
nim_gc_ref_check_cast (NimRef *ref, NimRef *klass)
{
    if (ref == NULL) {
        if (klass != NULL) {
            NIM_BUG ("expected ref type '%s', but got a null ref",
                        NIM_STR_DATA(NIM_CLASS (klass)->name));
        }
        else {
            NIM_BUG ("expected non-null ref");
        }
        return NULL;
    }

    if (klass == NULL) {
        return ref->value;
    }

    /* TODO need an 'instanceof' type check here */
    if (NIM_ANY_CLASS(ref) != klass) {
        NIM_BUG ("expected ref type '%s', but got '%s'",
                    NIM_STR_DATA(NIM_CLASS (klass)->name),
                    NIM_STR_DATA(NIM_CLASS (NIM_ANY_CLASS(ref))->name));
        return NULL;
    }

    return ref->value;
}

void
nim_gc_mark_ref (NimGC *gc, NimRef *ref)
{
    NimRef *klass;

    if (ref == NULL) return;

    if (gc == NULL) {
        gc = NIM_CURRENT_GC;
    }

    if (!nim_heap_contains (&gc->heap, ref)) {
        /* this ref belongs to another GC */
        return;
    }

    if (ref->marked) return;
    ref->marked = NIM_TRUE;

    nim_gc_mark_ref (gc, NIM_FAST_ANY(ref)->klass);
    klass = NIM_FAST_ANY(ref)->klass;

    if (NIM_CLASS(klass)->mark) {
        NIM_CLASS(klass)->mark (gc, ref);
    }
    else {
        NIM_BUG ("No mark for class %s", NIM_STR_DATA(NIM_CLASS(klass)->name));
    }
}

static void
_nim_gc_mark_lwhash_item (
    NimLWHash *self, NimRef *key, NimRef *value, void *data)
{
    NimGC *gc = (NimGC *) data;
    nim_gc_mark_ref (gc, key);
    nim_gc_mark_ref (gc, value);
}

void
nim_gc_mark_lwhash (NimGC *gc, NimLWHash *lwhash)
{
    nim_lwhash_foreach (lwhash, _nim_gc_mark_lwhash_item, gc);
}

static size_t
nim_gc_sweep (NimGC *gc)
{
    NimRef *ref = gc->live;
    NimRef *live = NULL;
    NimRef *free_ = NULL;
    size_t kept = 0;
    size_t freed = 0;

    while (ref != NULL) {
        NimRef *next = ref->next;
        if (!nim_heap_contains (&gc->heap, ref)) {
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
            nim_gc_value_dtor (gc, ref);
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

#if (defined NIM_ARCH_X86_64) && (defined __GNUC__)
#define NIM_GC_GET_STACK_END(ptr, guess) \
    __asm__("movq %%rsp, %0" : "=r" (ptr))
#elif (defined NIM_ARCH_X86_32) && (defined __GNUC__)
#define NIM_GC_GET_STACK_END(ptr, guess) \
    __asm__("movl %%esp, %0" : "=r" (ptr))
#else
#warning "Unknown or unsupported architecture: GC must guess at stack end"
#define NIM_GC_GET_STACK_END(ptr, guess) (ptr) = (guess)
#endif

nim_bool_t
nim_gc_collect (NimGC *gc)
{
    size_t i;
    NimRef *ref;
    NimRef *base;
    /* save registers to the stack */
#if (defined NIM_ARCH_X86_64) && (defined __GNUC__)
    void *regs[16];
    __asm__ volatile ("push %rax;");
    __asm__ volatile (
         "movq %%rbx, 8(%%rax);"
         "movq %%rcx, 16(%%rax);"
         "movq %%rdx, 24(%%rax);"
         "movq %%rsi, 32(%%rax);"
         "movq %%rdi, 40(%%rax);"
         "movq %%r8,  48(%%rax);"
         "movq %%r9,  56(%%rax);"
         "movq %%r10, 64(%%rax);"
         "movq %%r11, 72(%%rax);"
         "movq %%r12, 80(%%rax);"
         "movq %%r13, 88(%%rax);"
         "movq %%r14, 96(%%rax);"
         "movq %%r15, 104(%%rax);"
         "movq %%rbp, 112(%%rax);"
         "movq %%rsp, 120(%%rax);"
         "pop %%rbx;"
         "movq %%rbx, 128(%%rax);"
         :
         : "a" (regs)
         : "%rbx", "memory");
#elif (defined NIM_ARCH_X86_32) && (defined __GNUC__)
    /* XXX untested */
    void *regs[8];
    __asm__ volatile ("push %eax;");
    __asm__ volatile (
        "movl %%ebx, 4(%%eax);"
        "movl %%ecx, 8(%%eax);"
        "movl %%edx, 12(%%eax);"
        "movl %%esi, 16(%%eax);"
        "movl %%edi, 20(%%eax);"
        "movl %%ebp, 24(%%eax);"
        "movl %%esp, 28(%%eax);"
        "pop %%ebx;"
        "movl %%ebx, 32(%%eax);"
        :
        : "a" (regs)
        : "%ebx", "memory");
#else
#warning "Unknown or unsupported architecture: GC can't grok registers"
#endif
    
    base = NULL;
    base = base;

    if (gc == NULL) {
        gc = NIM_CURRENT_GC;
    }

    gc->collection_count++;

    ref = gc->live;
    while (ref != NULL) {
        ref->marked = NIM_FALSE;
        ref = ref->next;
    }

    for (i = 0; i < gc->num_roots; i++) {
        nim_gc_mark_ref (gc, gc->roots[i]);
    }

    nim_task_mark (gc, NIM_CURRENT_TASK);

    if (gc->stack_start != NULL) {
        void *ref_p;
        
        NIM_GC_GET_STACK_END(ref_p, &base);

        /* XXX stack may grow in the other direction on some archs. */
        while (ref_p <= gc->stack_start) {
            /* STFU valgrind. */
            VALGRIND_MAKE_MEM_DEFINED(ref_p, sizeof(NimRef *));

            if (nim_heap_contains (&gc->heap, *((NimRef **)ref_p))) {
                nim_gc_mark_ref (gc, *((NimRef **)ref_p));
            }
            ref_p += sizeof(ref_p);
        }
    }

    return nim_gc_sweep (gc) > 0;
}

uint64_t
nim_gc_collection_count (NimGC *gc)
{
    if (gc == NULL) {
        gc = NIM_CURRENT_GC;
    }

    return gc->collection_count;
}

uint64_t
nim_gc_num_live (NimGC *gc)
{
    if (gc == NULL) {
        gc = NIM_CURRENT_GC;
    }

    return gc->heap.used;
}

uint64_t
nim_gc_num_free (NimGC *gc)
{
    if (gc == NULL) {
        gc = NIM_CURRENT_GC;
    }

    return (gc->heap.slab_count * gc->heap.slab_size) - gc->heap.used;
}

