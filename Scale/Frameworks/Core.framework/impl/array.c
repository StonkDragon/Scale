#include <scale_internal.h>

extern scl_stack_t stack;

struct Array {
    scl_int $__type__;
    scl_str $__type_name__;
    scl_value values;
    scl_value count;
    scl_value capacity;
};

struct Array* Method_Array_sort(void) {
    struct Array* array = stack.data[--stack.ptr].v;
    scl_int i = 0;
    while (i < (scl_int) array->capacity) {
        scl_int speicher = *(scl_int*)(array->values + (i * 8));
        scl_int smallestIndex = i;
        scl_int j = i + 1;
        while (j < (scl_int) array->capacity) {
            if (*(scl_int*)(array->values + (j * 8)) < speicher) {
                speicher = *(scl_int*)(array->values + (j * 8));
                smallestIndex = j;
            }
            j++;
        }
        scl_int tmp = *(scl_int*)(array->values + (i * 8));
        *(scl_int*)(array->values + (i * 8)) = speicher;
        *(scl_int*)(array->values + (smallestIndex * 8)) = tmp;
        i++;
    }
    return array;
}
