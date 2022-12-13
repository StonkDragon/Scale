#ifndef _SCALE_H_
#define _SCALE_H_

#include <scale_internal.h>

sclDefFuncHeader(dumpstack,         "dumpstack_sIargs__sItype_none", void);
sclDefFuncHeader(sleep,             "sleep_sIargs_int_sItype_none", void, scl_int c);
sclDefFuncHeader(getenv,            "getenv_sIargs_str_sItype_str", scl_str, scl_str c);
sclDefFuncHeader(sizeof_stack,      "sizeof_stack_sIargs__sItype_int", scl_int);
sclDefFuncHeader(concat,            "concat_sIargs_str_str_sItype_str", scl_str, scl_str s2, scl_str s1);
sclDefFuncHeader(random,            "random_sIargs__sItype_int", scl_int);
sclDefFuncHeader(crash,             "crash_sIargs__sItype_none", void);
sclDefFuncHeader(system,            "system_sIargs_str_sItype_int", scl_int, scl_str cmd);
sclDefFuncHeader(strlen,            "strlen_sIargs_str_sItype_int", scl_int, scl_str s);
sclDefFuncHeader(strcmp,            "strcmp_sIargs_str_str_sItype_int", scl_int, scl_str s2, scl_str s1);
sclDefFuncHeader(strdup,            "strdup_sIargs_str_sItype_str", scl_str, scl_str s);
sclDefFuncHeader(strncmp,           "strncmp_sIargs_str_str_int_sItype_int", scl_int, scl_int n, scl_str s1, scl_str s2);
sclDefFuncHeader(raise,             "raise_sIargs_int_sItype_none", void, scl_int n);
sclDefFuncHeader(strrev,            "strrev_sIargs_str_sItype_str", scl_str, scl_str s);
sclDefFuncHeader(strsplit,          "strsplit_sIargs_str_str_sItype_Array", struct Array*, scl_str sep, scl_str string);
sclDefFuncHeader(malloc,            "malloc_sIargs_int_sItype__sIptrType_any", scl_value, scl_int n);
sclDefFuncHeader(realloc,           "realloc_sIargs_int__sIptrType_any_sItype__sIptrType_any", scl_value, scl_value s, scl_int n);
sclDefFuncHeader(free,              "free_sIargs__sIptrType_any_sItype_none", void, scl_value s);
sclDefFuncHeader(breakpoint,        "breakpoint_sIargs__sItype_none", void);
sclDefFuncHeader(memset,            "memset_sIargs_any_int_int_sItype_none", void, scl_int c, scl_int n, scl_value s);
sclDefFuncHeader(memcpy,            "memcpy_sIargs_any_any_int_sItype_none", void, scl_int n, scl_value s1, scl_value s2);
sclDefFuncHeader(time,              "time_sIargs__sItype_float", scl_float);
sclDefFuncHeader(trace,             "trace_sIargs__sItype_none", void);
sclDefFuncHeader(sqrt,              "sqrt_sIargs_float_sItype_float", scl_float, scl_float n);
sclDefFuncHeader(sin,               "sin_sIargs_float_sItype_float", scl_float, scl_float n);
sclDefFuncHeader(cos,               "cos_sIargs_float_sItype_float", scl_float, scl_float n);
sclDefFuncHeader(tan,               "tan_sIargs_float_sItype_float", scl_float, scl_float n);
sclDefFuncHeader(asin,              "asin_sIargs_float_sItype_float", scl_float, scl_float n);
sclDefFuncHeader(acos,              "acos_sIargs_float_sItype_float", scl_float, scl_float n);
sclDefFuncHeader(atan,              "atan_sIargs_float_sItype_float", scl_float, scl_float n);
sclDefFuncHeader(atan2,             "atan2_sIargs_float_float_sItype_float", scl_float, scl_float n1, scl_float n2);
sclDefFuncHeader(sinh,              "sinh_sIargs_float_sItype_float", scl_float, scl_float n);
sclDefFuncHeader(cosh,              "cosh_sIargs_float_sItype_float", scl_float, scl_float n);
sclDefFuncHeader(tanh,              "tanh_sIargs_float_sItype_float", scl_float, scl_float n);
sclDefFuncHeader(asinh,             "asinh_sIargs_float_sItype_float", scl_float, scl_float n);
sclDefFuncHeader(acosh,             "acosh_sIargs_float_sItype_float", scl_float, scl_float n);
sclDefFuncHeader(atanh,             "atanh_sIargs_float_sItype_float", scl_float, scl_float n);
sclDefFuncHeader(exp,               "exp_sIargs_float_sItype_float", scl_float, scl_float n);
sclDefFuncHeader(log,               "log_sIargs_float_sItype_float", scl_float, scl_float n);
sclDefFuncHeader(log10,             "log10_sIargs_float_sItype_float", scl_float, scl_float n);
sclDefFuncHeader(longToString,      "longToString_sIargs_int_sItype_str", scl_str, scl_int a);
sclDefFuncHeader(stringToLong,      "stringToLong_sIargs_str_sItype_int", scl_int, scl_str s);
sclDefFuncHeader(stringToDouble,    "stringToDouble_sIargs_str_sItype_float", scl_float, scl_str s);
sclDefFuncHeader(doubleToString,    "doubleToString_sIargs_float_sItype_str", scl_str, scl_float a);

#endif // _SCALE_H_
