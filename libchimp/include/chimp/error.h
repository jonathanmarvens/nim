#ifndef _CHIMP_ERROR_H_INCLUDED_
#define _CHIMP_ERROR_H_INCLUDED_

#include <chimp/any.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ChimpError {
    ChimpAny base;
    ChimpRef *message;
    ChimpRef *backtrace;
    ChimpRef *cause;
} ChimpError;

chimp_bool_t
chimp_error_class_bootstrap (void);

ChimpRef *
chimp_error_new (ChimpRef *message);

ChimpRef *
chimp_error_new_with_format (const char *fmt, ...);

#define CHIMP_ERROR(ref)  CHIMP_CHECK_CAST(ChimpError, (ref), CHIMP_VALUE_TYPE_ERROR)
#define CHIMP_ERROR_MESSAGE(ref) CHIMP_ERROR(ref)->message
#define CHIMP_ERROR_BACKTRACE(ref) CHIMP_ERROR(ref)->backtrace
#define CHIMP_ERROR_CAUSE(ref) CHIMP_ERROR(ref)->cause

CHIMP_EXTERN_CLASS(error);

#ifdef __cplusplus
};
#endif

#endif

