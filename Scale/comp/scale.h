#ifndef SCALE_H
#define SCALE_H

#include <stdlib.h>

typedef void* scl_word;

/* Function header */
#define s_header(name, ...)             \
    void fun ## name ##(__VA_ARGS__)

/* Call a function with the given name and arguments. */
#define s_call(name, ...)               \
    fun_ ## name ##(__VA_ARGS__)

/* Call a native function with the given name. */
#define s_nativecall(name)              \
    function_native_start(#name);       \
    fun_ ## name ##();                  \
    function_native_end()

// Stacktrace functions
void stacktrace_print();
void function_native_start(char* ptr);
void function_nps_start(char* ptr);
void function_start(char* ptr);
void function_native_end();
void function_nps_end();
void function_end();

// Stack push functions
void push(scl_word n);
void push_double(double n);
void push_long(long long n);
void push_string(const char* c);

// Stack pop functions
scl_word  pop();
long long pop_long();
double    pop_double();
char*     pop_string();
void      require_elements(size_t n, char* msg);

// Heap management functions
void heap_collect();
void heap_collect_all(int print);
void heap_add(void* ptr, int isFile);
void heap_remove(void* ptr);

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
