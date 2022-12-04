#include <scale_internal.h>

extern scl_stack_t stack;

struct Array {
    scl_int $__type__;
    scl_str $__type_name__;
    scl_value values;
    scl_value count;
    scl_value capacity;
};

void Method_Array_sort(struct Array* array) {
    scl_int i = 0;
    while (i < (scl_int) array->capacity) {
        scl_int speicher = *(scl_int*)(array->values + (i * 8));
        scl_int j = i - 1;
        while (j >= 0) {
            if (speicher < *(scl_int*)(array->values + (j * 8))) {
                *(scl_int*)(array->values + ((j+1) * 8)) = *(scl_int*)(array->values + (j * 8));
                *(scl_int*)(array->values + (j * 8)) = speicher;
            } else {
                *(scl_int*)(array->values + ((j+1) * 8)) = speicher;
                break;
            }
            j--;
        }
        i++;
    }
}
