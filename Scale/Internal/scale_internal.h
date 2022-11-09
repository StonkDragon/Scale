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

/* Function header */
#define sclDefFunc(name) \
  void fn_ ## name (void)

/* Call a function with the given name. */
#define sclCallFunc(name)  	\
  fn_ ## name ()

#define operator(a) \
  void op_ ## a (void) asm(#a)

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

#define ssize_t signed long
#define scl_value void*
#define scl_int long long
#define scl_str char*
#define scl_float double

typedef struct {
	scl_str type;
	union {
		scl_value 	i;
		scl_float 	f;
	} value;
} scl_frame_t;

typedef struct {
	ssize_t     ptr;
	scl_frame_t data[STACK_SIZE];
} scl_stack_t;

void		scl_security_throw(int code, scl_str msg) asm("throw");
void		scl_security_required_arg_count(ssize_t n, scl_str func) asm("requires");
void		scl_security_check_null(scl_value n) asm("is_null");
void 		scl_security_safe_exit(int code) asm("safe_exit");
void		ctrl_set_file(scl_str file) asm("set_file");
void        ctrl_set_pos(size_t line, size_t col) asm("set_pos");
void 		process_signal(int sig_num) asm("proc_signal");

void		ctrl_fn_start(scl_str name) asm("function_begin");
void		ctrl_fn_end(void) asm("function_end");
void 		ctrl_trace(void) asm("trace");
void 		print_stacktrace(void) asm("print_trace");

void		ctrl_push_string(const scl_str c) asm("push_str");
void		ctrl_push_long(long long n) asm("push_int");
void		ctrl_push_double(scl_float d) asm("push_float");
void		ctrl_push(scl_value n) asm("push_any");
scl_str		ctrl_pop_string(void) asm("pop_str");
scl_float	ctrl_pop_double(void) asm("pop_float");
long long	ctrl_pop_long(void) asm("pop_int");
scl_value	ctrl_pop(void) asm("pop_any");
ssize_t	 	ctrl_stack_size(void) asm("stack_size");
void 		sap_open(void) asm("sap_push");
void		sap_close(void) asm("sap_pop");

int		 	scl_is_struct(void* p) asm("is_struct");
scl_value	scl_alloc_struct(size_t size) asm("create_struct");
void		scl_dealloc_struct(scl_value ptr) asm("destroy_struct");

operator(add);
operator(sub);
operator(mul);
operator(div);
operator(mod);
operator(land);
operator(lor);
operator(lxor);
operator(lnot);
operator(lsh);
operator(rsh);
operator(pow);
operator(dadd);
operator(dsub);
operator(dmul);
operator(ddiv);

#endif
