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

#include "chimp/msg.h"
#include "chimp/class.h"
#include "chimp/object.h"
#include "chimp/array.h"
#include "chimp/task.h"

#define chimp_msg_int_cell_size(ref) sizeof(ChimpMsgCell)
#define chimp_msg_str_cell_size(ref) \
    (sizeof(ChimpMsgCell) + CHIMP_STR_SIZE(ref) + 1)

static chimp_bool_t
chimp_msg_value_cell_encode (char **buf_ptr, ChimpRef *ref);

static chimp_bool_t
chimp_msg_value_cell_decode (char **buf_ptr, ChimpRef **ref);

static size_t
chimp_msg_value_cell_size (ChimpRef *ref);

static size_t
chimp_msg_array_cell_size (ChimpRef *array)
{
    size_t i;
    size_t size = sizeof(ChimpMsgCell);
    for (i = 0; i < CHIMP_ARRAY_SIZE(array); i++) {
        size += chimp_msg_value_cell_size (CHIMP_ARRAY_ITEM(array, i));
    }
    return size;
}

static size_t
chimp_msg_task_cell_size (ChimpRef *ref)
{
    return sizeof(ChimpMsgCell) + sizeof(ChimpTaskInternal *);
}

static size_t
chimp_msg_value_cell_size (ChimpRef *ref)
{
    ChimpRef *klass = CHIMP_ANY_CLASS(ref);

    if (klass == chimp_int_class) {
        return chimp_msg_int_cell_size(ref);
    }
    else if (klass == chimp_str_class) {
        return chimp_msg_str_cell_size(ref);
    }
    else if (klass == chimp_array_class) {
        return chimp_msg_array_cell_size (ref);
    }
    else if (klass == chimp_task_class) {
        return chimp_msg_task_cell_size (ref);
    }
    else {
        CHIMP_BUG ("unsupported message type in array encode: %s",
                CHIMP_STR_DATA(CHIMP_CLASS_NAME(CHIMP_ANY_CLASS(ref))));
        return 0;
    }
}

static chimp_bool_t
chimp_msg_int_cell_encode (char **buf_ptr, ChimpRef *ref)
{
    char *buf = *buf_ptr;
    ChimpMsgCell *cell = (ChimpMsgCell *)buf;
    cell->type = CHIMP_MSG_CELL_INT;
    cell->int_ = CHIMP_INT(ref)->value;
    buf += chimp_msg_int_cell_size (ref);
    *buf_ptr = buf;
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_msg_str_cell_encode (char **buf_ptr, ChimpRef *ref)
{
    char *buf = *buf_ptr;
    ChimpMsgCell *cell = (ChimpMsgCell *)buf;
    cell->type = CHIMP_MSG_CELL_STR;
    cell->str.data = buf + sizeof(ChimpMsgCell);
    memcpy (cell->str.data, CHIMP_STR_DATA(ref), CHIMP_STR_SIZE(ref));
    cell->str.size = CHIMP_STR_SIZE(ref);
    buf += chimp_msg_str_cell_size (ref);
    *buf_ptr = buf;
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_msg_array_cell_encode (char **buf_ptr, ChimpRef *ref)
{
    size_t i;
    char *buf = *buf_ptr;
    ChimpMsgCell *cell = (ChimpMsgCell *)buf;
    cell->type = CHIMP_MSG_CELL_ARRAY;
    cell->array.items = ((ChimpMsgCell *)(buf + sizeof(ChimpMsgCell)));
    cell->array.size  = CHIMP_ARRAY_SIZE(ref);
    buf += sizeof(ChimpMsgCell);
    for (i = 0; i < CHIMP_ARRAY_SIZE(ref); i++) {
        if (!chimp_msg_value_cell_encode (&buf, CHIMP_ARRAY_ITEM(ref, i))) {
            return CHIMP_FALSE;
        }
    }
    *buf_ptr = buf;
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_msg_task_cell_encode (char **buf_ptr, ChimpRef *ref)
{
    char *buf = *buf_ptr;
    ChimpMsgCell *cell = (ChimpMsgCell *)buf;
    cell->type = CHIMP_MSG_CELL_TASK;
    cell->task = CHIMP_TASK(ref)->priv;
    /* bump the reference count on the task in case existing refs die
     * before the message arrives
     */
    chimp_task_ref (cell->task);
    buf += sizeof(ChimpMsgCell);
    *buf_ptr = buf;
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_msg_value_cell_encode (char **buf_ptr, ChimpRef *ref)
{
    ChimpRef *klass = CHIMP_ANY_CLASS(ref);

    if (klass == chimp_int_class) {
        if (!chimp_msg_int_cell_encode (buf_ptr, ref)) {
            return CHIMP_FALSE;
        }
    }
    else if (klass == chimp_str_class) {
        if (!chimp_msg_str_cell_encode (buf_ptr, ref)) {
            return CHIMP_FALSE;
        }
    }
    else if (klass == chimp_array_class) {
        if (!chimp_msg_array_cell_encode (buf_ptr, ref)) {
            return CHIMP_FALSE;
        }
    }
    else if (klass == chimp_task_class) {
        if (!chimp_msg_task_cell_encode (buf_ptr, ref)) {
            return CHIMP_FALSE;
        }
    }
    else {
        CHIMP_BUG ("unsupported message type in encode: %s",
                CHIMP_STR_DATA(CHIMP_CLASS_NAME(CHIMP_ANY_CLASS(ref))));
        return CHIMP_FALSE;
    }

    return CHIMP_TRUE;
}

ChimpMsgInternal *
chimp_msg_pack (ChimpRef *value)
{
    char *buf;
    ChimpMsgInternal *temp;
    size_t size;
    size_t cell_size;
    
    /* compute the full message size up-front */
    size = sizeof(ChimpMsgInternal);
    cell_size = chimp_msg_value_cell_size (value);
    if (cell_size == 0) return NULL;
    size += cell_size;

    /* allocate & initialize the internal message */
    buf = malloc (size);
    if (buf == NULL) {
        return NULL;
    }
    temp = (ChimpMsgInternal *) buf;
    temp->size = size;
    temp->next = NULL;
    buf += sizeof(ChimpMsgInternal);
    temp->cell = (ChimpMsgCell *) buf;

    if (!chimp_msg_value_cell_encode (&buf, value)) {
        free (buf);
        return NULL;
    }

    return temp;
}

static chimp_bool_t
chimp_msg_array_cell_decode (char **buf_ptr, ChimpRef **ref)
{
    size_t i;
    char *buf = *buf_ptr;
    ChimpMsgCell *cell = (ChimpMsgCell *)buf;
    ChimpRef *array = chimp_array_new_with_capacity (cell->array.size);

    if (array == NULL) {
        return CHIMP_FALSE;
    }

    buf += sizeof(ChimpMsgCell);

    for (i = 0; i < cell->array.size; i++) {
        ChimpRef *item;
        if (!chimp_msg_value_cell_decode (&buf, &item)) {
            return CHIMP_FALSE;
        }
        if (!chimp_array_push (array, item)) {
            return CHIMP_FALSE;
        }
    }

    *ref = array;
    *buf_ptr = buf;
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_msg_task_cell_decode (char **buf_ptr, ChimpRef **ref)
{
    char *buf = *buf_ptr;
    ChimpRef *taskobj;
    ChimpMsgCell *cell = (ChimpMsgCell *)buf;
    ChimpTaskInternal *task = cell->task;
    taskobj = chimp_task_new_from_internal (task);
    buf += sizeof(ChimpMsgCell);
    chimp_task_unref (task);
    *ref = taskobj;
    *buf_ptr = buf;
    return CHIMP_TRUE;
}

static chimp_bool_t
chimp_msg_value_cell_decode (char **buf_ptr, ChimpRef **ref)
{
    char *buf = *buf_ptr;
    ChimpMsgCell *cell = (ChimpMsgCell *)buf;
    switch (cell->type) {
        case CHIMP_MSG_CELL_INT:
            {
                *ref = chimp_int_new (cell->int_);
                if (*ref == NULL) {
                    return CHIMP_FALSE;
                }
                buf += sizeof(ChimpMsgCell);
                *buf_ptr = buf;
                break;
            }
        case CHIMP_MSG_CELL_STR:
            {
                *ref = chimp_str_new (cell->str.data, cell->str.size);
                if (*ref == NULL) {
                    return CHIMP_FALSE;
                }
                buf += sizeof(ChimpMsgCell) + cell->str.size + 1;
                *buf_ptr = buf;
                break;
            }
        case CHIMP_MSG_CELL_ARRAY:
            {
                if (!chimp_msg_array_cell_decode (buf_ptr, ref)) {
                    return CHIMP_FALSE;
                }
                break;
            }
        case CHIMP_MSG_CELL_TASK:
            {
                if (!chimp_msg_task_cell_decode (buf_ptr, ref)) {
                    return CHIMP_FALSE;
                }
                break;
            }
        default:
            CHIMP_BUG ("unknown message cell type in decode: %d\n",
                        cell->type);
            return CHIMP_FALSE;
    };
    return CHIMP_TRUE;
}

ChimpRef *
chimp_msg_unpack (ChimpMsgInternal *msg)
{
    char *buf = (char *)msg->cell;
    ChimpRef *value;
    
    if (!chimp_msg_value_cell_decode (&buf, &value)) {
        return NULL;
    }

    return value;
}

