#ifndef _SCALE_H_
#define _SCALE_H_

#include <scale_internal.h>

sclDefFuncHeader(dumpstack,         "dumpstack(): none", void);
sclDefFuncHeader(sleep,             "sleep(int): none", void, scl_int c);
sclDefFuncHeader(getenv,            "getenv(str): str", scl_str, scl_str c);
sclDefFuncHeader(sizeof_stack,      "sizeof_stack(): int", scl_int);
sclDefFuncHeader(concat,            "concat(str, str): str", scl_str, scl_str s2, scl_str s1);
sclDefFuncHeader(random,            "random(): int", scl_int);
sclDefFuncHeader(crash,             "crash(): none", void);
sclDefFuncHeader(system,            "system(str): int", scl_int, scl_str cmd);
sclDefFuncHeader(strlen,            "strlen(str): int", scl_int, scl_str s);
sclDefFuncHeader(strcmp,            "strcmp(str, str): int", scl_int, scl_str s2, scl_str s1);
sclDefFuncHeader(strdup,            "strdup(str): str", scl_str, scl_str s);
sclDefFuncHeader(strncmp,           "strncmp(str, str, int): int", scl_int, scl_int n, scl_str s1, scl_str s2);
sclDefFuncHeader(raise,             "raise(int): none", void, scl_int n);
sclDefFuncHeader(strrev,            "strrev(str): str", scl_str, scl_str s);
sclDefFuncHeader(strsplit,          "strsplit(str, str): Array", struct Array*, scl_str sep, scl_str string);
sclDefFuncHeader(malloc,            "malloc(int): any", scl_value, scl_int n);
sclDefFuncHeader(realloc,           "realloc(int, any): any", scl_value, scl_value s, scl_int n);
sclDefFuncHeader(free,              "free(any): none", void, scl_value s);
sclDefFuncHeader(breakpoint,        "breakpoint(): none", void);
sclDefFuncHeader(memset,            "memset(any, int, int): none", void, scl_int c, scl_int n, scl_value s);
sclDefFuncHeader(memcpy,            "memcpy(any, any, int): none", void, scl_int n, scl_value s1, scl_value s2);
sclDefFuncHeader(time,              "time(): float", scl_float);
sclDefFuncHeader(trace,             "trace(): none", void);
sclDefFuncHeader(sqrt,              "sqrt(float): float", scl_float, scl_float n);
sclDefFuncHeader(sin,               "sin(float): float", scl_float, scl_float n);
sclDefFuncHeader(cos,               "cos(float): float", scl_float, scl_float n);
sclDefFuncHeader(tan,               "tan(float): float", scl_float, scl_float n);
sclDefFuncHeader(asin,              "asin(float): float", scl_float, scl_float n);
sclDefFuncHeader(acos,              "acos(float): float", scl_float, scl_float n);
sclDefFuncHeader(atan,              "atan(float): float", scl_float, scl_float n);
sclDefFuncHeader(atan2,             "atan2(float, float): float", scl_float, scl_float n1, scl_float n2);
sclDefFuncHeader(sinh,              "sinh(float): float", scl_float, scl_float n);
sclDefFuncHeader(cosh,              "cosh(float): float", scl_float, scl_float n);
sclDefFuncHeader(tanh,              "tanh(float): float", scl_float, scl_float n);
sclDefFuncHeader(asinh,             "asinh(float): float", scl_float, scl_float n);
sclDefFuncHeader(acosh,             "acosh(float): float", scl_float, scl_float n);
sclDefFuncHeader(atanh,             "atanh(float): float", scl_float, scl_float n);
sclDefFuncHeader(exp,               "exp(float): float", scl_float, scl_float n);
sclDefFuncHeader(log,               "log(float): float", scl_float, scl_float n);
sclDefFuncHeader(log10,             "log10(float): float", scl_float, scl_float n);
sclDefFuncHeader(longToString,      "longToString(int): str", scl_str, scl_int a);
sclDefFuncHeader(stringToLong,      "stringToLong(str): int", scl_int, scl_str s);
sclDefFuncHeader(stringToDouble,    "stringToDouble(str): float", scl_float, scl_str s);
sclDefFuncHeader(doubleToString,    "doubleToString(float): str", scl_str, scl_float a);

#endif // _SCALE_H_
