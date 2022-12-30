#include <scale_internal.h>

#define ssize_t signed long
typedef void* scl_any;
typedef long long scl_int;
typedef char* scl_str;
typedef double scl_float;

extern scl_stack_t stack;

export void bar(scl_str Var_a) __asm("function_bar_sclArgs_str_sclType_none");
expect void foo() __asm("_Function_foo");
