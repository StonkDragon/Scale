#include <scale_internal.h>

#define scl_export(func_name, ...) \
    void func_name (__VA_ARGS__) __asm("_Function_" #func_name); \
    void func_name (__VA_ARGS__)

#define ssize_t signed long
typedef void* scl_any;
typedef long long scl_int;
typedef char* scl_str;
typedef double scl_float;

extern scl_stack_t stack;

void bar(scl_str a);
