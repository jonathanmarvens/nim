#include "chimp/object.h"
#include "chimp/gc.h"

ChimpCmpResult
chimp_object_cmp (ChimpRef *a, ChimpRef *b)
{
    ChimpRef *a_class, *b_class;

    /* reference equality is nice & easy */
    if (a == b) {
        return CHIMP_CMP_EQ;
    }

    a_class = CHIMP_ANY_CLASS(a);
    b_class = CHIMP_ANY_CLASS(b);

    if (CHIMP_CLASS(a_class)->cmp != NULL) {
        return CHIMP_CLASS(a_class)->cmp (a, b);
    }
    else if (CHIMP_CLASS(b_class)->cmp != NULL) {
        return CHIMP_CLASS(b_class)->cmp (a, b);
    }
    else {
        return CHIMP_CMP_NOT_IMPL;
    }
}

