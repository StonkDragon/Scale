#ifndef ARRAY_H
#define ARRAY_H

#include <scale_internal.h>

struct Array {
    scl_int $__type__;
    scl_str $__type_name__;
    scl_value $__lock__;
    scl_value values;
    scl_value count;
    scl_value capacity;
};

void Method_Array_sort(struct Array* array) __asm("mthd_Array_fnct_sort_sIargs__sItype_none");

#endif
