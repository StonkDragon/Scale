#ifndef ARRAY_H
#define ARRAY_H

#include <scale_internal.h>

extern scl_stack_t* stack;

struct Array {
    scl_int $__type__;
    scl_str $__type_name__;
    scl_any $__lock__;
    scl_any* values;
    scl_int count;
    scl_int capacity;
};

void Method_Array_sort(struct Array* array) __asm("mthd_Array_fnct_sort_sIargs__sItype_none");
scl_any Method_Array_get(struct Array* array, scl_int index) __asm("mthd_Array_fnct_get_sIargs_int_sItype_any");
void Method_Array_set(struct Array* array, scl_any value, scl_int index) __asm("mthd_Array_fnct_set_sIargs_int_any_sItype_none");

#endif
