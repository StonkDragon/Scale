#ifndef _SCALE_H_
#define _SCALE_H_

#include <scale_internal.h>

sclDefFunc(dumpstack, void);
sclDefFunc(sleep, void, scl_int c);
sclDefFunc(getenv, scl_str, scl_str c);
sclDefFunc(sizeof_stack, scl_int);
sclDefFunc(concat, scl_str, scl_str s2, scl_str s1);
sclDefFunc(random, scl_int);
sclDefFunc(crash, void);
sclDefFunc(system, scl_int, scl_str cmd);
sclDefFunc(strlen, scl_int, scl_str s);
sclDefFunc(strcmp, scl_int, scl_str s2, scl_str s1);
sclDefFunc(strdup, scl_str, scl_str s);
sclDefFunc(strncmp, scl_int, scl_int n, scl_str s1, scl_str s2);
sclDefFunc(raise, void, scl_int n);
sclDefFunc(strrev, scl_str, scl_str s);
sclDefFunc(strsplit, struct Array*, scl_str sep, scl_str string);
sclDefFunc(malloc, scl_value, scl_int n);
sclDefFunc(realloc, scl_value, scl_value s, scl_int n);
sclDefFunc(free, void, scl_value s);
sclDefFunc(breakpoint, void);
sclDefFunc(memset, void, scl_int c, scl_int n, scl_value s);
sclDefFunc(memcpy, void, scl_int n, scl_value s1, scl_value s2);
sclDefFunc(time, scl_float);
sclDefFunc(trace, void);
sclDefFunc(sqrt, scl_float, scl_float n);
sclDefFunc(sin, scl_float, scl_float n);
sclDefFunc(cos, scl_float, scl_float n);
sclDefFunc(tan, scl_float, scl_float n);
sclDefFunc(asin, scl_float, scl_float n);
sclDefFunc(acos, scl_float, scl_float n);
sclDefFunc(atan, scl_float, scl_float n);
sclDefFunc(atan2, scl_float, scl_float n1, scl_float n2);
sclDefFunc(sinh, scl_float, scl_float n);
sclDefFunc(cosh, scl_float, scl_float n);
sclDefFunc(tanh, scl_float, scl_float n);
sclDefFunc(asinh, scl_float, scl_float n);
sclDefFunc(acosh, scl_float, scl_float n);
sclDefFunc(atanh, scl_float, scl_float n);
sclDefFunc(exp, scl_float, scl_float n);
sclDefFunc(log, scl_float, scl_float n);
sclDefFunc(log10, scl_float, scl_float n);
sclDefFunc(longToString, scl_str, scl_int a);
sclDefFunc(stringToLong, scl_int, scl_str s);
sclDefFunc(stringToDouble, scl_float, scl_str s);
sclDefFunc(doubleToString, scl_str, scl_float a);

#endif // _SCALE_H_
