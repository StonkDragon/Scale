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
#define sclDefFunc(name)   	\
  void fn_ ## name ()

/* Call a function with the given name and arguments. */
#define sclCallFunc(name)  	\
  fn_ ## name ()

#define operator(a) \
  void op_ ## a (void)

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
typedef void* scl_value;
typedef union {
	scl_value ptr;
	double floating;
} scl_frame_t;

typedef struct {
	ssize_t     ptr;
	scl_frame_t data[STACK_SIZE];
	ssize_t     offset[STACK_SIZE];
} scl_stack_t;

void		scl_security_throw(int code, char* msg);
void		scl_security_required_arg_count(ssize_t n, char* func);
void		scl_security_check_null(scl_value n);
void 		scl_security_safe_exit(int code);
void		ctrl_set_file(char* file);
void        ctrl_set_pos(size_t line, size_t col);
void 		process_signal(int sig_num);

void		ctrl_fn_start(char* name);
void		ctrl_fn_end(void);
void		ctrl_fn_end_with_return(void);
void		ctrl_fn_fn_start(char* name);
void		ctrl_fn_fn_end(void);
void		ctrl_fn_nps_start(char* name);
void		ctrl_fn_nps_end(void);
void 		ctrl_trace(void);
void 		print_stacktrace(void);

void		ctrl_push_string(const char* c);
void		ctrl_push_long(long long n);
void		ctrl_push_double(double d);
void		ctrl_push(scl_value n);
char*		ctrl_pop_string(void);
double		ctrl_pop_double(void);
long long	ctrl_pop_long(void);
scl_value	ctrl_pop(void);
ssize_t	 	ctrl_stack_size(void);
void 		sap_open(void);
void		sap_close(void);

int		 	scl_is_complex(void* p);
scl_value	scl_alloc_complex(size_t size);
void		scl_dealloc_complex(scl_value ptr);

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
