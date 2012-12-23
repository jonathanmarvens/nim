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

#include "chimp/frame.h"
#include "chimp/class.h"
#include "chimp/object.h"
#include "chimp/str.h"

#define CHIMP_FRAME_INIT(ref) \
    do { \
        CHIMP_ANY(ref)->type = CHIMP_VALUE_TYPE_FRAME; \
        CHIMP_ANY(ref)->klass = chimp_frame_class; \
    } while (0)

ChimpRef *chimp_frame_class = NULL;

chimp_bool_t
chimp_frame_class_bootstrap (void)
{
    chimp_frame_class =
        chimp_class_new (CHIMP_STR_NEW("frame"), chimp_object_class);
    if (chimp_frame_class == NULL) {
        return CHIMP_FALSE;
    }
    chimp_gc_make_root (NULL, chimp_frame_class);
    return CHIMP_TRUE;
}

ChimpRef *
chimp_frame_new (ChimpRef *method)
{
    ChimpRef *locals;
    ChimpRef *ref = chimp_gc_new_object (NULL);
    if (ref == NULL) {
        return NULL;
    }
    CHIMP_FRAME_INIT(ref);
    locals = chimp_hash_new ();
    if (locals == NULL) {
        return NULL;
    }
    CHIMP_FRAME(ref)->method = method;
    CHIMP_FRAME(ref)->locals = locals;
    return ref;
}

