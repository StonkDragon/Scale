#include "runtime.h"

extern const char* const scl_internal_frameworks[];
extern const size_t scl_internal_frameworks_size;

extern const scl_method scl_internal_function_ptrs[];
extern const unsigned long long scl_internal_function_names[];
extern const size_t scl_internal_function_names_size;

typedef unsigned long long hash;

hash hash1(char* data) {
    hash h = 7;
    for (int i = 0; i < strlen(data); i++) {
        h = h * 31 + data[i];
    }
    return h;
}

sclDefFunc(Runtime_hasFramework) {
    char* name = ctrl_pop_string();
    for (size_t i = 0; i < scl_internal_frameworks_size; i++) {
        if (strcmp(name, scl_internal_frameworks[i]) == 0) {
            ctrl_push_long(1);
            return;
        }
    }
    ctrl_push_long(0);
}

sclDefFunc(Runtime_listFrameworks) {
    printf("Frameworks:\n");
    for (size_t i = 0; i < scl_internal_frameworks_size; i++) {
        printf(" %s\n", scl_internal_frameworks[i]);
    }
}

sclDefFunc(Runtime_getFunctionID) {
    char* name = ctrl_pop_string();
    hash h = hash1(name);
    
    for (size_t i = 0; i < scl_internal_function_names_size; i++) {
        if (h == scl_internal_function_names[i]) {
            ctrl_push_long(i);
            return;
        }
    }
    ctrl_push_long(-1);
}

sclDefFunc(Runtime_callByID) {
    int id = ctrl_pop_long();
    if (id == -1) {
        fprintf(stderr, "Error: Method not found");
        return;
    }
    scl_internal_function_ptrs[id]();
}
