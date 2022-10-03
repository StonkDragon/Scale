#include "runtime.h"

extern const char* const __scl_internal__frameworks[];
extern const size_t __scl_internal__frameworks_size;

extern const char __scl_internal__function_args[];
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

sclNativeImpl(Runtime_hasFramework) {
    char* name = ctrl_pop_string();
    for (size_t i = 0; i < __scl_internal__frameworks_size; i++) {
        if (strcmp(name, __scl_internal__frameworks[i]) == 0) {
            ctrl_push_long(1);
            return;
        }
    }
    ctrl_push_long(0);
}

sclNativeImpl(Runtime_listFrameworks) {
    printf("Frameworks:\n");
    for (size_t i = 0; i < __scl_internal__frameworks_size; i++) {
        printf(" %s\n", __scl_internal__frameworks[i]);
    }
}

sclNativeImpl(Runtime_getFunctionID) {
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

sclNativeImpl(Runtime_callByID) {
    int id = ctrl_pop_long();
    if (id == -1) {
        fprintf(stderr, "Error: Method not found");
        return;
    }
    char argCount = __scl_internal__function_args[id];
    const void* func = __scl_internal__function_ptrs[id];
    switch (argCount) {
        case 0: ((void (*)()) func)(); break;
        case 1: ((void (*)(scl_value, ...)) func)(ctrl_pop()); break;
        case 2: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop()); break;
        case 3: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 4: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 5: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 6: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 7: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 8: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 9: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 10: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 11: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 12: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 13: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 14: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 15: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 16: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 17: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 18: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 19: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 20: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 21: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 22: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 23: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 24: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 25: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 26: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 27: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 28: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 29: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 30: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        case 31: ((void (*)(scl_value, ...)) func)(ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop(), ctrl_pop()); break;
        default: fprintf(stderr, "Invalid Function\n"); break;
    }
}
