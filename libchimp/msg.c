#include "chimp/msg.h"
#include "chimp/class.h"
#include "chimp/object.h"
#include "chimp/array.h"

#define CHIMP_MSG_INIT(ref) \
    CHIMP_ANY(ref)->type = CHIMP_VALUE_TYPE_MSG; \
    CHIMP_ANY(ref)->klass = chimp_msg_class;

ChimpRef *chimp_msg_class = NULL;

static ChimpRef *
_chimp_msg_init (ChimpRef *self, ChimpRef *args)
{
    size_t i;
    void *buf;
    ChimpMsgInternal *temp;
    ChimpRef *data = args;
    size_t size;
    
    /* compute the full message size up-front */
    size = sizeof(ChimpMsgInternal);
    for (i = 0; i < CHIMP_ARRAY_SIZE(data); i++) {
        ChimpRef *ref = CHIMP_ARRAY_ITEM(data, i);
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
    temp->num_cells = CHIMP_ARRAY_SIZE(data);
    temp->next = NULL;
    temp->cells = (ChimpMsgCell *)(buf + sizeof(ChimpMsgInternal));
    buf += sizeof(ChimpMsgInternal);

    /* map array elements to message cells */
    for (i = 0; i < CHIMP_ARRAY_SIZE(data); i++) {
        ChimpRef *ref = CHIMP_ARRAY_ITEM(data, i);
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
                    ((ChimpMsgCell*)buf)->str.data =
                        (buf + sizeof(ChimpMsgCell));
                    memcpy (
                        ((ChimpMsgCell*)buf)->str.data,
                        CHIMP_STR_DATA(ref),
                        CHIMP_STR_SIZE(ref)
                    );
                    ((ChimpMsgCell*)buf)->str.data[CHIMP_STR_SIZE(ref)] = '0';
                    ((ChimpMsgCell*)buf)->str.size = CHIMP_STR_SIZE(ref);
                    buf += sizeof(ChimpMsgCell) + CHIMP_STR_SIZE(ref) + 1;
                    break;
                }
            default:
                free (temp);
                return NULL;
        };
    }
    CHIMP_MSG(self)->impl = temp;
    return self;
}

chimp_bool_t
chimp_msg_class_bootstrap (void)
{
    chimp_msg_class =
        chimp_class_new (CHIMP_STR_NEW("msg"), chimp_object_class);
    if (chimp_msg_class == NULL) {
        return CHIMP_FALSE;
    }
    CHIMP_CLASS(chimp_msg_class)->init = _chimp_msg_init;
    CHIMP_CLASS(chimp_msg_class)->inst_type = CHIMP_VALUE_TYPE_MSG;
    return CHIMP_TRUE;
}

ChimpRef *
chimp_msg_new (ChimpRef *data)
{
    return chimp_class_new_instance (chimp_msg_class, data, NULL);
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

