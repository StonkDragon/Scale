#include <scale_internal.h>

#define scl_export(func_name) \
    void func_name (void); \
    void Function_ ## func_name (void) __asm("_Function_" #func_name); \
    void Function_ ## func_name () { func_name (); } \
    void func_name (void)

#define ssize_t signed long
typedef void* scl_any;
typedef long long scl_int;
typedef char* scl_str;
typedef double scl_float;

extern scl_stack_t stack;

void bar(scl_str a);
