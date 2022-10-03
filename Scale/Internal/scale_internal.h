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
#define sclDefFunc(name, ...)   	\
  void fn_ ## name ##(__VA_ARGS__)

/* Call a function with the given name and arguments. */
#define sclCallFunc(name, ...)  	\
  fn_ ## name ##(__VA_ARGS__)

/* Call a native function with the given name. */
#define sclCallNative(name)    	\
  ctrl_fn_native_start(#name); 	\
  native_ ## name ##();     	\
  ctrl_fn_native_end()

#define sclNativeImpl(name) \
	void native_ ## name (void)

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

// Types
#define TYPE_STR "str"
#define TYPE_LONG "int"
#define TYPE_DOUBLE "float"


typedef signed long ssize_t;
typedef void* scl_value;
typedef struct {
	char* type;
	scl_value value;
} scl_frame;
typedef scl_frame scl_var;
typedef struct {
	scl_value ptr;
	ssize_t   level;
	int       isFile;
} scl_memory_t;
typedef struct {
	ssize_t   ptr;
	scl_value data[STACK_SIZE];
	ssize_t   offset[STACK_SIZE];
} scl_stack_t;

void		scl_security_throw(int code, char* msg);
void		scl_security_required_arg_count(ssize_t n, char* func);
void		ctrl_where(char* file, size_t line, size_t col);
void		ctrl_fn_start(char* name);
void		ctrl_fn_end(void);
void		ctrl_fn_end_with_return(void);
void		ctrl_fn_native_start(char* name);
void		ctrl_fn_native_end(void);
void		ctrl_fn_nps_start(char* name);
void		ctrl_fn_nps_end(void);
void		ctrl_push_complex(scl_value complex);
void		ctrl_push_string(const char* c);
void		ctrl_push_word(scl_value n);
void		ctrl_push_long(long long n);
void		ctrl_push_double(double d);
void		ctrl_push(scl_value n);
scl_value 	ctrl_pop_complex(void);
char*		ctrl_pop_string(void);
double		ctrl_pop_double(void);
long long	ctrl_pop_long(void);
scl_value	ctrl_pop_word(void);
scl_value	ctrl_pop(void);
ssize_t	 	ctrl_stack_size(void);
void		scl_security_check_null(scl_value n);
void		scl_security_check_uaf(scl_value ptr);
void 		sap_open(void);
void		sap_close(void);
void		op_add(void);
void		op_sub(void);
void		op_mul(void);
void		op_div(void);
void		op_mod(void);
void		op_land(void);
void		op_lor(void);
void		op_lxor(void);
void		op_lnot(void);
void		op_lsh(void);
void		op_rsh(void);
void		op_pow(void);
void		op_dadd(void);
void		op_dsub(void);
void		op_dmul(void);
void		op_ddiv(void);

void ctrl_trace(void);
void scl_security_safe_exit(int code);
void print_stacktrace(void);
void process_signal(int sig_num);

#endif
