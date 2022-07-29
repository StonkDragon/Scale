#ifndef SCALE_H
#define SCALE_H

/* Macro for calling Scale functions */
#define scall(func)  scale_func_##func()
/* Macro for function headers of custom Scale Functions */
#define sfunc_head(func) void scale_func_##func()

// Stacktrace functions
void stacktrace_push(char* ptr, int inc);
void stacktrace_pop(int dec);
void stacktrace_print();

// Stack push functions
void scale_push(void* n);
void scale_push_double(double n);
void scale_push_long(long long n);
void scale_push_string(const char* c);

// Stack pop functions
void*        scale_pop();
long long    scale_pop_long();
double       scale_pop_double();
char*        scale_pop_string();

// Heap management functions
void scale_heap_collect();
void scale_heap_collect_all(int print);
void scale_heap_add(void* ptr, int isFile);
void scale_heap_remove(void* ptr, int isFile);

void scale_op_add();
void scale_op_sub();
void scale_op_mul();
void scale_op_div();
void scale_op_mod();
void scale_op_land();
void scale_op_lor();
void scale_op_lxor();
void scale_op_lnot();
void scale_op_lsh();
void scale_op_rsh();
void scale_op_pow();

void scale_extern_trace();
void scale_extern_dumpstack();
void scale_extern_read();
void scale_extern_exit();
void scale_extern_sleep();
void scale_extern_getenv();
void scale_extern_less();
void scale_extern_more();
void scale_extern_equal();
void scale_extern_dup();
void scale_extern_over();
void scale_extern_swap();
void scale_extern_drop();
void scale_extern_sizeof_stack();
void scale_extern_concat();
void scale_extern_dadd();
void scale_extern_dsub();
void scale_extern_dmul();
void scale_extern_ddiv();
void scale_extern_dtoi();
void scale_extern_itod();
void scale_extern_random();
void scale_extern_crash();
void scale_extern_and();
void scale_extern_system();
void scale_extern_getstack();
void scale_extern_not();
void scale_extern_or();
void scale_extern_sprintf();
void scale_extern_strlen();
void scale_extern_strcmp();
void scale_extern_strncmp();
void scale_extern_fopen();
void scale_extern_fclose();
void scale_extern_fread();
void scale_extern_fseekstart();
void scale_extern_fseekend();
void scale_extern_fseekcur();
void scale_extern_ftell();
void scale_extern_fileno();
void scale_extern_sizeof();
void scale_extern_raise();
void scale_extern_abort();
void scale_extern_write();
void scale_extern_malloc();
void scale_extern_free();
void scale_extern_breakpoint();
void scale_extern_memset();
void scale_extern_memcpy();
void scale_extern_gc_collect();

#endif // SCALE_H
