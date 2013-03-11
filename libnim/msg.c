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

#include <inttypes.h>

#include "nim/msg.h"
#include "nim/class.h"
#include "nim/object.h"
#include "nim/array.h"
#include "nim/task.h"

#define nim_msg_int_cell_size(ref) sizeof(NimMsgCell)
#define nim_msg_nil_cell_size(ref) sizeof(NimMsgCell)
#define nim_msg_method_cell_size(ref) sizeof(NimMsgCell)
#define nim_msg_module_cell_size(ref) sizeof(NimMsgCell)
#define nim_msg_str_cell_size(ref) \
    (sizeof(NimMsgCell) + NIM_STR_SIZE(ref) + 1)

static nim_bool_t
nim_msg_value_cell_encode (char **buf_ptr, NimRef *ref);

static nim_bool_t
nim_msg_value_cell_decode (char **buf_ptr, NimRef **ref);

static size_t
nim_msg_value_cell_size (NimRef *ref);

static size_t
nim_msg_array_cell_size (NimRef *array)
{
    size_t i;
    size_t size = sizeof(NimMsgCell);
    for (i = 0; i < NIM_ARRAY_SIZE(array); i++) {
        size += nim_msg_value_cell_size (NIM_ARRAY_ITEM(array, i));
    }
    return size;
}

static size_t
nim_msg_task_cell_size (NimRef *ref)
{
    return sizeof(NimMsgCell) + sizeof(NimTaskInternal *);
}

static size_t
nim_msg_value_cell_size (NimRef *ref)
{
    NimRef *klass = NIM_ANY_CLASS(ref);

    if (klass == nim_int_class) {
        return nim_msg_int_cell_size(ref);
    }
    else if (klass == nim_str_class) {
        return nim_msg_str_cell_size(ref);
    }
    else if (klass == nim_array_class) {
        return nim_msg_array_cell_size (ref);
    }
    else if (klass == nim_nil_class) {
        return nim_msg_nil_cell_size (ref);
    }
    else if (klass == nim_module_class) {
        return nim_msg_module_cell_size (ref);
    }
    else if (klass == nim_method_class) {
        return nim_msg_method_cell_size (ref);
    }
    else if (klass == nim_task_class) {
        return nim_msg_task_cell_size (ref);
    }
    else {
        NIM_BUG ("unsupported message type in array encode: %s",
                NIM_STR_DATA(NIM_CLASS_NAME(NIM_ANY_CLASS(ref))));
        return 0;
    }
}

static nim_bool_t
nim_msg_nil_cell_encode (char **buf_ptr, NimRef *ref)
{
    char *buf = *buf_ptr;
    NimMsgCell *cell = (NimMsgCell *)buf;
    cell->type = NIM_MSG_CELL_NIL;
    *buf_ptr = buf;
    return NIM_TRUE;
}

static nim_bool_t
nim_msg_int_cell_encode (char **buf_ptr, NimRef *ref)
{
    char *buf = *buf_ptr;
    NimMsgCell *cell = (NimMsgCell *)buf;
    cell->type = NIM_MSG_CELL_INT;
    cell->int_ = NIM_INT(ref)->value;
    buf += nim_msg_int_cell_size (ref);
    *buf_ptr = buf;
    return NIM_TRUE;
}

static nim_bool_t
nim_msg_str_cell_encode (char **buf_ptr, NimRef *ref)
{
    char *buf = *buf_ptr;
    NimMsgCell *cell = (NimMsgCell *)buf;
    cell->type = NIM_MSG_CELL_STR;
    cell->str.data = buf + sizeof(NimMsgCell);
    memcpy (cell->str.data, NIM_STR_DATA(ref), NIM_STR_SIZE(ref));
    cell->str.size = NIM_STR_SIZE(ref);
    buf += nim_msg_str_cell_size (ref);
    *buf_ptr = buf;
    return NIM_TRUE;
}

static nim_bool_t
nim_msg_array_cell_encode (char **buf_ptr, NimRef *ref)
{
    size_t i;
    char *buf = *buf_ptr;
    NimMsgCell *cell = (NimMsgCell *)buf;
    cell->type = NIM_MSG_CELL_ARRAY;
    cell->array.items = ((NimMsgCell *)(buf + sizeof(NimMsgCell)));
    cell->array.size  = NIM_ARRAY_SIZE(ref);
    buf += sizeof(NimMsgCell);
    for (i = 0; i < NIM_ARRAY_SIZE(ref); i++) {
        if (!nim_msg_value_cell_encode (&buf, NIM_ARRAY_ITEM(ref, i))) {
            return NIM_FALSE;
        }
    }
    *buf_ptr = buf;
    return NIM_TRUE;
}

static nim_bool_t
nim_msg_module_cell_encode (char **buf_ptr, NimRef *ref)
{
    char *buf = *buf_ptr;
    NimMsgCell *cell = (NimMsgCell *) buf;
    cell->type = NIM_MSG_CELL_MODULE;
    cell->module = ref;
    buf += sizeof(NimMsgCell);
    *buf_ptr = buf;
    return NIM_TRUE;
}

static nim_bool_t
nim_msg_method_cell_encode (char **buf_ptr, NimRef *ref)
{
    char *buf = *buf_ptr;
    NimMsgCell *cell = (NimMsgCell *) buf;
    cell->type = NIM_MSG_CELL_METHOD;
    cell->method = ref;
    buf += sizeof(NimMsgCell);
    *buf_ptr = buf;
    return NIM_TRUE;
}

static nim_bool_t
nim_msg_task_cell_encode (char **buf_ptr, NimRef *ref)
{
    char *buf = *buf_ptr;
    NimMsgCell *cell = (NimMsgCell *)buf;
    cell->type = NIM_MSG_CELL_TASK;
    cell->task = NIM_TASK(ref)->priv;
    /* bump the reference count on the task in case existing refs die
     * before the message arrives
     */
    nim_task_ref (cell->task);
    buf += sizeof(NimMsgCell);
    *buf_ptr = buf;
    return NIM_TRUE;
}

static nim_bool_t
nim_msg_value_cell_encode (char **buf_ptr, NimRef *ref)
{
    NimRef *klass = NIM_ANY_CLASS(ref);

    if (klass == nim_int_class) {
        if (!nim_msg_int_cell_encode (buf_ptr, ref)) {
            return NIM_FALSE;
        }
    }
    else if (klass == nim_str_class) {
        if (!nim_msg_str_cell_encode (buf_ptr, ref)) {
            return NIM_FALSE;
        }
    }
    else if (klass == nim_nil_class) {
        if (!nim_msg_nil_cell_encode (buf_ptr, ref)) {
            return NIM_FALSE;
        }
    }
    else if (klass == nim_array_class) {
        if (!nim_msg_array_cell_encode (buf_ptr, ref)) {
            return NIM_FALSE;
        }
    }
    else if (klass == nim_module_class) {
        if (!nim_msg_module_cell_encode (buf_ptr, ref)) {
            return NIM_FALSE;
        }
    }
    else if (klass == nim_method_class) {
        if (NIM_METHOD(ref)->type == NIM_METHOD_TYPE_CLOSURE) {
            NIM_BUG ("closures cannot be serialized");
            return NIM_FALSE;
        }
        if (!nim_msg_method_cell_encode (buf_ptr, ref)) {
            return NIM_FALSE;
        }
    }
    else if (klass == nim_task_class) {
        if (!nim_msg_task_cell_encode (buf_ptr, ref)) {
            return NIM_FALSE;
        }
    }
    else {
        NIM_BUG ("unsupported message type in encode: %s",
                NIM_STR_DATA(NIM_CLASS_NAME(NIM_ANY_CLASS(ref))));
        return NIM_FALSE;
    }

    return NIM_TRUE;
}

NimMsgInternal *
nim_msg_pack (NimRef *value)
{
    char *buf;
    NimMsgInternal *temp;
    size_t size;
    size_t cell_size;
    
    /* compute the full message size up-front */
    size = sizeof(NimMsgInternal);
    cell_size = nim_msg_value_cell_size (value);
    if (cell_size == 0) return NULL;
    size += cell_size;

    /* allocate & initialize the internal message */
    buf = malloc (size);
    if (buf == NULL) {
        return NULL;
    }
    temp = (NimMsgInternal *) buf;
    temp->size = size;
    temp->next = NULL;
    buf += sizeof(NimMsgInternal);
    temp->cell = (NimMsgCell *) buf;

    if (!nim_msg_value_cell_encode (&buf, value)) {
        free (buf);
        return NULL;
    }

    return temp;
}

static nim_bool_t
nim_msg_array_cell_decode (char **buf_ptr, NimRef **ref)
{
    size_t i;
    char *buf = *buf_ptr;
    NimMsgCell *cell = (NimMsgCell *)buf;
    NimRef *array = nim_array_new_with_capacity (cell->array.size);

    if (array == NULL) {
        return NIM_FALSE;
    }

    buf += sizeof(NimMsgCell);

    for (i = 0; i < cell->array.size; i++) {
        NimRef *item;
        if (!nim_msg_value_cell_decode (&buf, &item)) {
            return NIM_FALSE;
        }
        if (!nim_array_push (array, item)) {
            return NIM_FALSE;
        }
    }

    *ref = array;
    *buf_ptr = buf;
    return NIM_TRUE;
}

static nim_bool_t
nim_msg_module_cell_decode (char **buf_ptr, NimRef **ref)
{
    char *buf = *buf_ptr;
    NimMsgCell *cell = (NimMsgCell *) buf;
    NimRef *module = cell->module;
    buf += sizeof(NimMsgCell);
    *ref = module;
    *buf_ptr = buf;
    return NIM_TRUE;
}

static nim_bool_t
nim_msg_method_cell_decode (char **buf_ptr, NimRef **ref)
{
    char *buf = *buf_ptr;
    NimMsgCell *cell = (NimMsgCell *) buf;
    NimRef *method = cell->method;
    buf += sizeof(NimMsgCell);
    *ref = method;
    *buf_ptr = buf;
    return NIM_TRUE;
}

static nim_bool_t
nim_msg_task_cell_decode (char **buf_ptr, NimRef **ref)
{
    char *buf = *buf_ptr;
    NimRef *taskobj;
    NimMsgCell *cell = (NimMsgCell *)buf;
    NimTaskInternal *task = cell->task;
    taskobj = nim_task_new_from_internal (task);
    buf += sizeof(NimMsgCell);
    nim_task_unref (task);
    *ref = taskobj;
    *buf_ptr = buf;
    return NIM_TRUE;
}

static nim_bool_t
nim_msg_value_cell_decode (char **buf_ptr, NimRef **ref)
{
    char *buf = *buf_ptr;
    NimMsgCell *cell = (NimMsgCell *)buf;
    switch (cell->type) {
        case NIM_MSG_CELL_NIL:
            {
                (*buf_ptr) += sizeof(NimMsgCell);
                *ref = nim_nil;
                break;
            }
        case NIM_MSG_CELL_INT:
            {
                *ref = nim_int_new (cell->int_);
                if (*ref == NULL) {
                    return NIM_FALSE;
                }
                buf += sizeof(NimMsgCell);
                *buf_ptr = buf;
                break;
            }
        case NIM_MSG_CELL_STR:
            {
                *ref = nim_str_new (cell->str.data, cell->str.size);
                if (*ref == NULL) {
                    return NIM_FALSE;
                }
                buf += sizeof(NimMsgCell) + cell->str.size + 1;
                *buf_ptr = buf;
                break;
            }
        case NIM_MSG_CELL_ARRAY:
            {
                if (!nim_msg_array_cell_decode (buf_ptr, ref)) {
                    return NIM_FALSE;
                }
                break;
            }
        case NIM_MSG_CELL_MODULE:
            {
                if (!nim_msg_module_cell_decode (buf_ptr, ref)) {
                    return NIM_FALSE;
                }
                break;
            }
        case NIM_MSG_CELL_METHOD:
            {
                if (!nim_msg_method_cell_decode (buf_ptr, ref)) {
                    return NIM_FALSE;
                }
                break;
            }
        case NIM_MSG_CELL_TASK:
            {
                if (!nim_msg_task_cell_decode (buf_ptr, ref)) {
                    return NIM_FALSE;
                }
                break;
            }
        default:
            NIM_BUG ("unknown message cell type in decode: %d\n",
                        cell->type);
            return NIM_FALSE;
    };
    return NIM_TRUE;
}

NimRef *
nim_msg_unpack (NimMsgInternal *msg)
{
    char *buf = (char *)msg->cell;
    NimRef *value;
    
    if (!nim_msg_value_cell_decode (&buf, &value)) {
        return NULL;
    }

    return value;
}

