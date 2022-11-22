#include "runtime.h"

extern const char* const scl_internal_frameworks[];
extern const size_t scl_internal_frameworks_size;

extern const scl_method scl_internal_function_ptrs[];
extern const unsigned long long scl_internal_function_names[];
extern const size_t scl_internal_functions_size;

typedef unsigned long long hash;

hash hash1(char* data) {
    hash h = 7;
    for (int i = 0; i < strlen(data); i++) {
        h = h * 31 + data[i];
    }
    return h;
}

sclDefFunc(Runtime_hasFramework, scl_int) {
    char* name = ctrl_pop_string();
    for (size_t i = 0; i < scl_internal_frameworks_size; i++) {
        if (strcmp(name, scl_internal_frameworks[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

sclDefFunc(Runtime_listFrameworks, void) {
    printf("Frameworks:\n");
    for (size_t i = 0; i < scl_internal_frameworks_size; i++) {
        printf(" %s\n", scl_internal_frameworks[i]);
    }
}

sclDefFunc(Runtime_getFunctionID, scl_int) {
    char* name = ctrl_pop_string();
    hash h = hash1(name);
    
    for (size_t i = 0; i < scl_internal_functions_size; i++) {
        if (h == scl_internal_function_names[i]) {
            return (scl_int) i;
        }
    }
    return -1;
}

sclDefFunc(Runtime_callByID, void) {
    int id = ctrl_pop_long();
    if (id == -1) {
        fprintf(stderr, "Error: Method not found");
        return;
    }
    scl_internal_function_ptrs[id]();
}

struct Array {
    scl_int $__type__;
    scl_str $__type_name__;
    scl_value values;
    scl_value count;
    scl_value capacity;
};

extern scl_value alloced_structs[];
extern size_t alloced_structs_count;

sclDefFunc(Runtime_objectsByType, struct Array*) {
    scl_str type = ctrl_pop_string();
    struct Array* array = scl_alloc_struct(sizeof(struct Array), "Array");
    array->capacity = (scl_value) alloced_structs_count;
    array->count = 0;
    array->values = scl_alloc(alloced_structs_count);
    for (size_t i = 0; i < alloced_structs_count; i++) {
        if (!alloced_structs[i]) continue;
        if (((struct Array*) (alloced_structs[i]))->$__type_name__ == 0) continue;
        if (strcmp(((struct Array*) (alloced_structs[i]))->$__type_name__, type) == 0) {
            ((scl_value*) array->values)[(scl_int) array->count++] = alloced_structs[i];
        }
    }
    return array;
}
