#include "runtime.h"

extern const char* const __scl_internal__frameworks[];
extern const size_t __scl_internal__frameworks_size;

extern const void* __scl_internal__function_ptrs[];
extern const unsigned long long __scl_internal__function_names[];
extern const size_t __scl_internal__function_names_size;

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
    for (size_t i = 0; i < __scl_internal__frameworks_size; i++) {
        if (strcmp(name, __scl_internal__frameworks[i]) == 0) {
            ctrl_push_long(1);
            return;
        }
    }
    ctrl_push_long(0);
}

sclDefFunc(Runtime_listFrameworks) {
    printf("Frameworks:\n");
    for (size_t i = 0; i < __scl_internal__frameworks_size; i++) {
        printf(" %s\n", __scl_internal__frameworks[i]);
    }
}

sclDefFunc(Runtime_getFunctionID) {
    char* name = ctrl_pop_string();
    hash h = hash1(name);
    
    for (size_t i = 0; i < __scl_internal__function_names_size; i++) {
        if (h == __scl_internal__function_names[i]) {
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
    const void* func = __scl_internal__function_ptrs[id];
    ((void (*)(void)) func)();
}
