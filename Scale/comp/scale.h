#ifndef _SCALE_H_
#define _SCALE_H_

#if __has_attribute(always_inline)
#define scl_force_inline __attribute__((always_inline))
#else
#define scl_force_inline
#endif

#include <stdlib.h>

#define INITIAL_SIZE (1024)
#define MALLOC_LIMIT (1024)
#define MAX_STRING_SIZE (2048)
#define LONG_AS_STR_LEN (22)

// Define scale-specific signals
#define EX_BAD_PTR (128)
#define EX_STACK_OVERFLOW (129)
#define EX_STACK_UNDERFLOW (130)
#define EX_UNHANDLED_DATA (131)
#define EX_IO_ERROR (132)
#define EX_INVALID_ARGUMENT (133)
#define EX_CAST_ERROR (134)

typedef void* scl_word;
typedef struct {
	scl_word ptr;
	size_t	 level;
	int 	 isFile;
} scl_memory_t;
typedef struct {
	size_t 	 ptr;
	scl_word data[INITIAL_SIZE];
} scl_stack_t;

scl_force_inline void throw(int code, char* msg);
scl_force_inline void ctrl_required(size_t n, char* func);
scl_force_inline void ctrl_where(char* file, size_t line, size_t col);
scl_force_inline void ctrl_fn_start(char* name);
scl_force_inline void ctrl_fn_end(void);
scl_force_inline void ctrl_fn_native_start(char* name);
scl_force_inline void ctrl_fn_native_end(void);
scl_force_inline void ctrl_fn_nps_start(char* name);
scl_force_inline void ctrl_fn_nps_end(void);
scl_force_inline void ctrl_push_string(const char* c);
scl_force_inline void ctrl_push_double(double d);
scl_force_inline void ctrl_push_long(long long n);
scl_force_inline long long ctrl_pop_long(void);
scl_force_inline double ctrl_pop_double(void);
scl_force_inline void ctrl_push(scl_word n);
scl_force_inline char* ctrl_pop_string(void);
scl_force_inline scl_word ctrl_pop(void);
scl_force_inline scl_word ctrl_pop_word(void);
scl_force_inline void ctrl_push_word(scl_word n);
scl_force_inline void op_add(void);
scl_force_inline void op_sub(void);
scl_force_inline void op_mul(void);
scl_force_inline void op_div(void);
scl_force_inline void op_mod(void);
scl_force_inline void op_land(void);
scl_force_inline void op_lor(void);
scl_force_inline void op_lxor(void);
scl_force_inline void op_lnot(void);
scl_force_inline void op_lsh(void);
scl_force_inline void op_rsh(void);
scl_force_inline void op_pow(void);
scl_force_inline void op_dadd(void);
scl_force_inline void op_dsub(void);
scl_force_inline void op_dmul(void);
scl_force_inline void op_ddiv(void);

void ctrl_trace(void);
void heap_collect(void);
void heap_collect_all(void);
void heap_add(scl_word ptr, int isFile);
int heap_is_alloced(scl_word ptr);
void heap_remove(scl_word ptr);
void safe_exit(int code);
void print_stacktrace(void);
void process_signal(int sig_num);

void native_dumpstack(void);
void native_exit(void);
void native_sleep(void);
void native_getenv(void);
void native_less(void);
void native_more(void);
void native_equal(void);
void native_dup(void);
void native_over(void);
void native_swap(void);
void native_drop(void);
void native_sizeof_stack(void);
void native_concat(void);
void native_random(void);
void native_crash(void);
void native_and(void);
void native_system(void);
void native_not(void);
void native_or(void);
void native_sprintf(void);
void native_strlen(void);
void native_strcmp(void);
void native_strncmp(void);
void native_fopen(void);
void native_fclose(void);
void native_fseek(void);
void native_ftell(void);
void native_fileno(void);
void native_raise(void);
void native_abort(void);
void native_write(void);
void native_read(void);
void native_strrev(void);
void native_malloc(void);
void native_free(void);
void native_breakpoint(void);
void native_memset(void);
void native_memcpy(void);
void native_time(void);
void native_heap_collect(void);
void native_trace(void);
void native_sqrt(void);
void native_sin(void);
void native_cos(void);
void native_tan(void);
void native_asin(void);
void native_acos(void);
void native_atan(void);
void native_atan2(void);
void native_sinh(void);
void native_cosh(void);
void native_tanh(void);
void native_asinh(void);
void native_acosh(void);
void native_atanh(void);
void native_exp(void);
void native_log(void);
void native_log10(void);
void native_longToString(void);
void native_stringToLong(void);
void native_stringToDouble(void);
void native_doubleToString(void);

void fn_main(void);

#endif // _SCALE_H_
