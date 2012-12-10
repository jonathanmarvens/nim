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
    ChimpRef *data = args;
    for (i = 0; i < CHIMP_ARRAY_SIZE(data); i++) {
        ChimpValueType type = CHIMP_ANY_TYPE(CHIMP_ARRAY_ITEM(data, i));
        switch (type) {
            case CHIMP_VALUE_TYPE_INT:
            case CHIMP_VALUE_TYPE_STR:
                {
                    /* acceptable (immutable values) */
                    break;
                }
            default:
                return NULL;
        };
    }
    CHIMP_MSG(self)->data = data;
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
chimp_msg_dup (ChimpRef *self)
{
    size_t i;
    ChimpRef *arr = CHIMP_MSG(self)->data;
    ChimpRef *data =
        chimp_array_new_with_capacity (CHIMP_ARRAY_SIZE(arr));
    for (i = 0; i < CHIMP_ARRAY_SIZE(arr); i++) {
        ChimpRef *ref = CHIMP_ARRAY_ITEM(arr, i);
        switch (CHIMP_ANY_TYPE(ref)) {
            case CHIMP_VALUE_TYPE_STR:
                {
                    ChimpRef *str =
                        chimp_str_new (
                            CHIMP_STR_DATA(ref), CHIMP_STR_SIZE(ref));
                    if (str == NULL) {
                        return NULL;
                    }
                    CHIMP_ARRAY(data)->items[i] = str;
                    break;
                }
            case CHIMP_VALUE_TYPE_INT:
                {
                    ChimpRef *n = chimp_int_new (CHIMP_INT(ref)->value);
                    CHIMP_ARRAY(data)->items[i] = n;
                    break;
                }
            default:
                chimp_bug (__FILE__, __LINE__,
                    "invalid type passed to msg() ctor: %s",
                    CHIMP_CLASS_NAME(CHIMP_ANY_CLASS(ref)));
                return NULL;
        }
    }
    CHIMP_ARRAY(data)->size = CHIMP_ARRAY_SIZE(arr);
    return chimp_msg_new (data);
}

