#ifndef SCALE_H
#define SCALE_H

#include <stdlib.h>

typedef void* scl_word;

/* Function header */
#define s_header(name, ...)            \
    void fun ## name ##(__VA_ARGS__)

/* Call a function with the given name and arguments. */
#define s_call(name, ...)              \
    fn_ ## name ##(__VA_ARGS__)

/* Call a native function with the given name. */
#define s_nativecall(name)             \
    ctrl_fn_native_start(#name);       \
    fn_ ## name ##();                  \
    ctrl_fn_native_end()

// Stacktrace functions
void ctrl_trace();
void ctrl_fn_native_start(char* ptr);
void ctrl_fn_nps_start(char* ptr);
void ctrl_fn_start(char* ptr);
void ctrl_fn_native_end();
void ctrl_fn_nps_end();
void ctrl_fn_end();

// Stack ctrl_push functions
void ctrl_push(scl_word n);
void ctrl_push_double(double n);
void ctrl_push_long(long long n);
void ctrl_push_string(const char* c);

// Stack ctrl_pop functions
scl_word  ctrl_pop();
long long ctrl_pop_long();
double    ctrl_pop_double();
char*     ctrl_pop_string();
void      ctrl_required(size_t n, char* msg);

// Heap management functions
void heap_collect();
void heap_collect_all();
void heap_add(scl_word ptr, int isFile);
void heap_remove(scl_word ptr);

void op_add();
void op_sub();
void op_mul();
void op_div();
void op_mod();
void op_land();
void op_lor();
void op_lxor();
void op_lnot();
void op_lsh();
void op_rsh();
void op_pow();

#endif // SCALE_H
