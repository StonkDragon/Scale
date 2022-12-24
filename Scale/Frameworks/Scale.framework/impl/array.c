#include "array.h"

void Method_Array_sort(struct Array* array) {
    scl_int i = 0;
    while (i < (scl_int) array->capacity) {
        scl_int speicher = (scl_int) Method_Array_get(array, i);
        scl_int j = i - 1;
        while (j >= 0) {
            if (speicher < (scl_int) Method_Array_get(array, j)) {
                Method_Array_set(array, Method_Array_get(array, j), j + 1);
                Method_Array_set(array, (scl_any) speicher, j);
            } else {
                Method_Array_set(array, (scl_any) speicher, j + 1);
                break;
            }
            j--;
        }
        i++;
    }
}
