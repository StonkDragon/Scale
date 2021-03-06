#ifndef SCALE_H
#define SCALE_H

/* Macro for calling Scale functions */
#define scall(func)  scale_func_##func()
/* Macro for function headers of custom Scale Functions */
#define sfunc_head(func) void scale_func_##func()

void scale_load_at(const char* key);
void scale_store_at(const char* key);
void stacktrace_push(char* ptr);
void stacktrace_pop();
void stacktrace_print();
void scale_extern_trace();
void scale_push_string(const char* c);
void scale_push_long(long long n);
void scale_push_double(double n);
long long scale_pop_long();
double scale_pop_double();
void scale_push(void* n);
char* scale_pop_string();
void* scale_pop();
void scale_extern_dumpstack();
void scale_extern_printf();
void scale_extern_read();
void scale_extern_tochars();
void scale_extern_exit();
void scale_extern_sleep();
void scale_extern_getproperty();
void scale_extern_less();
void scale_extern_more();
void scale_extern_equal();
void scale_extern_dup();
void scale_extern_over();
void scale_extern_swap();
void scale_extern_drop();
void scale_extern_sizeof_stack();
void scale_extern_concat();
void scale_extern_strstarts();
void scale_extern_add();
void scale_extern_sub();
void scale_extern_mul();
void scale_extern_div();
void scale_extern_dadd();
void scale_extern_dsub();
void scale_extern_dmul();
void scale_extern_ddiv();
void scale_extern_dtoi();
void scale_extern_itod();
void scale_extern_mod();
void scale_extern_lshift();
void scale_extern_rshift();
void scale_extern_random();
void scale_extern_crash();
void scale_extern_and();
void scale_extern_eval();
void scale_extern_getstack();
void scale_extern_not();
void scale_extern_or();
void scale_extern_sprintf();
void scale_extern_strlen();
void scale_extern_strcmp();
void scale_extern_strncmp();
void scale_extern_fprintf();
void scale_extern_fopen();
void scale_extern_fclose();
void scale_extern_fread();
void scale_extern_fseekstart();
void scale_extern_fseekend();
void scale_extern_fseekcur();
void scale_extern_ftell();
void scale_extern_fwrite();
void scale_extern_sizeof();
void scale_extern_raise();
void scale_extern_abort();

#endif // SCALE_H
