#ifndef _SCALE_INTERNAL_H_
#define _SCALE_INTERNAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>

#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#define sleep(s) Sleep(s)
#define read(fd, buf, n) _read(fd, buf, n)
#define write(fd, buf, n) _write(fd, buf, n)
#ifndef WINDOWS
#define WINDOWS
#endif
#else
#include <unistd.h>
#define sleep(s) do { struct timespec __ts = {((s) / 1000), ((s) % 1000) * 1000000}; nanosleep(&__ts, NULL); } while (0)
#endif

#if __has_attribute(always_inline)
#define scl_force_inline __attribute__((always_inline))
#else
#define scl_force_inline
#endif

#if __has_attribute(noinline)
#define scl_native __attribute__((noinline))
#else
#define scl_native
#endif

/* Function header */
#define sclDefFunc(name, ...)     	\
    void fn_ ## name ##(__VA_ARGS__)

/* Call a function with the given name and arguments. */
#define sclCallFunc(name, ...)    	\
    fn_ ## name ##(__VA_ARGS__)

/* Call a native function with the given name. */
#define sclCallNative(name)       	\
    ctrl_fn_native_start(#name);  	\
    native_ ## name ##();         	\
    ctrl_fn_native_end()

#define sclNativeImpl(name) \
	scl_native void native_ ## name (void)

#if __SIZEOF_POINTER__ < 8
#error "Scale is not supported on this platform"
#endif

#ifndef STACK_SIZE
#define STACK_SIZE 			4096
#endif

#define MALLOC_LIMIT 		1024
#define MAX_STRING_SIZE 	512
#define LONG_AS_STR_LEN 	22

// Define scale-specific signals
#define EX_BAD_PTR 			128
#define EX_STACK_OVERFLOW 	129
#define EX_STACK_UNDERFLOW 	130
#define EX_UNHANDLED_DATA 	131
#define EX_IO_ERROR 		132
#define EX_INVALID_ARGUMENT 133
#define EX_CAST_ERROR 		134
#define EX_SAP_ERROR 		135

typedef signed long ssize_t;
typedef void* scl_word;
typedef struct {
	scl_word ptr;
	ssize_t	 level;
	int 	 isFile;
} scl_memory_t;
typedef struct {
	ssize_t  ptr;
	scl_word data[STACK_SIZE];
	ssize_t  offset[STACK_SIZE];
} scl_stack_t;

scl_force_inline void 		throw(int code, char* msg);
scl_force_inline void 		ctrl_required(ssize_t n, char* func);
scl_force_inline void 		ctrl_where(char* file, size_t line, size_t col);
scl_force_inline void 		ctrl_fn_start(char* name);
scl_force_inline void 		ctrl_fn_end(void);
scl_force_inline void 		ctrl_fn_end_with_return(void);
scl_force_inline void 		ctrl_fn_native_start(char* name);
scl_force_inline void 		ctrl_fn_native_end(void);
scl_force_inline void 		ctrl_fn_nps_start(char* name);
scl_force_inline void 		ctrl_fn_nps_end(void);
scl_force_inline void 		ctrl_push_string(const char* c);
scl_force_inline void 		ctrl_push_double(double d);
scl_force_inline void 		ctrl_push_long(long long n);
scl_force_inline long long 	ctrl_pop_long(void);
scl_force_inline double 	ctrl_pop_double(void);
scl_force_inline void 		ctrl_push(scl_word n);
scl_force_inline char* 		ctrl_pop_string(void);
scl_force_inline scl_word 	ctrl_pop(void);
scl_force_inline scl_word 	ctrl_pop_word(void);
scl_force_inline void 		ctrl_push_word(scl_word n);
scl_force_inline ssize_t 	ctrl_stack_size(void);
scl_force_inline void 		scl_security_check_null(scl_word n);
scl_force_inline void 		sap_open(void);
scl_force_inline void 		sap_close(void);
scl_force_inline void 		op_add(void);
scl_force_inline void 		op_sub(void);
scl_force_inline void 		op_mul(void);
scl_force_inline void 		op_div(void);
scl_force_inline void 		op_mod(void);
scl_force_inline void 		op_land(void);
scl_force_inline void 		op_lor(void);
scl_force_inline void 		op_lxor(void);
scl_force_inline void 		op_lnot(void);
scl_force_inline void 		op_lsh(void);
scl_force_inline void 		op_rsh(void);
scl_force_inline void 		op_pow(void);
scl_force_inline void 		op_dadd(void);
scl_force_inline void 		op_dsub(void);
scl_force_inline void 		op_dmul(void);
scl_force_inline void 		op_ddiv(void);

void ctrl_trace(void);
void heap_collect(void);
void heap_add(scl_word ptr, int isFile);
int  heap_is_alloced(scl_word ptr);
void heap_remove(scl_word ptr);
void safe_exit(int code);
void print_stacktrace(void);
void process_signal(int sig_num);

#endif
