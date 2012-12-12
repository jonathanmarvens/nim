#include <inttypes.h>

#include "chimp/msg.h"
#include "chimp/class.h"
#include "chimp/object.h"
#include "chimp/array.h"

ChimpMsgInternal *
chimp_msg_pack (ChimpRef *array)
{
    size_t i;
    void *buf;
    char *strbuf;
    ChimpMsgInternal *temp;
    size_t size;
    
    /* compute the full message size up-front */
    size = sizeof(ChimpMsgInternal);
    for (i = 0; i < CHIMP_ARRAY_SIZE(array); i++) {
        ChimpRef *ref = CHIMP_ARRAY_ITEM(array, i);
        ChimpValueType type = CHIMP_ANY_TYPE(ref);
        /* check acceptable (immutable values) */
        switch (type) {
            case CHIMP_VALUE_TYPE_INT:
                {
                    size += sizeof(ChimpMsgCell);
                    break;
                }
            case CHIMP_VALUE_TYPE_STR:
                {
                    size += sizeof(ChimpMsgCell) + CHIMP_STR_SIZE(ref) + 1;
                    break;
                }
            default:
                return NULL;
        };
    }

    /* allocate & initialize the internal message */
    buf = malloc (size);
    if (buf == NULL) {
        return NULL;
    }
    temp = (ChimpMsgInternal *) buf;
    temp->size = size;
    temp->num_cells = CHIMP_ARRAY_SIZE(array);
    temp->next = NULL;
    buf += sizeof(ChimpMsgInternal);
    temp->cells = buf;
    strbuf = buf + sizeof(ChimpMsgCell) * temp->num_cells;

    /* map array elements to message cells */
    for (i = 0; i < CHIMP_ARRAY_SIZE(array); i++) {
        ChimpRef *ref = CHIMP_ARRAY_ITEM(array, i);
        ChimpValueType type = CHIMP_ANY_TYPE(ref);
        switch (type) {
            case CHIMP_VALUE_TYPE_INT:
                {
                    ((ChimpMsgCell*)buf)->type = CHIMP_MSG_CELL_INT;
                    ((ChimpMsgCell*)buf)->int_ = CHIMP_INT(ref)->value;
                    buf += sizeof(ChimpMsgCell);
                    break;
                }
            case CHIMP_VALUE_TYPE_STR:
                {
                    ((ChimpMsgCell*)buf)->type = CHIMP_MSG_CELL_STR;
                    ((ChimpMsgCell*)buf)->str.data = strbuf;
                    memcpy (
                        ((ChimpMsgCell*)buf)->str.data,
                        CHIMP_STR_DATA(ref),
                        CHIMP_STR_SIZE(ref)
                    );
                    ((ChimpMsgCell*)buf)->str.data[CHIMP_STR_SIZE(ref)] = '0';
                    ((ChimpMsgCell*)buf)->str.size = CHIMP_STR_SIZE(ref);
                    buf += sizeof(ChimpMsgCell);
                    strbuf += CHIMP_STR_SIZE(ref) + 1;
                    break;
                }
            default:
                free (temp);
                return NULL;
        };
    }

    return temp;
}

ChimpRef *
chimp_msg_unpack (ChimpMsgInternal *msg)
{
    size_t i;
    ChimpRef *arr = chimp_array_new_with_capacity (msg->num_cells);
    for (i = 0; i < msg->num_cells; i++) {
        ChimpRef *ref;
        ChimpMsgCell *cell = msg->cells + i;
        switch (cell->type) {
            case CHIMP_MSG_CELL_INT:
                {
                    ref = chimp_int_new (cell->int_);
                    break;
                }
            case CHIMP_MSG_CELL_STR:
                {
                    ref = chimp_str_new (cell->str.data, cell->str.size);
                    break;
                }
            default:
                chimp_bug (__FILE__, __LINE__, "unexpected value type in packed msg: %d", cell->type);
                return NULL;
        };
        if (ref == NULL) {
            return NULL;
        }
        if (!chimp_array_push (arr, ref)) {
            return NULL;
        }
    }
    return arr;
}

