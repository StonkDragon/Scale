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
#include <time.h>
#include <sys/time.h>

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
#include <pthread.h>
#define sleep(s) do { struct timespec __ts = {((s) / 1000), ((s) % 1000) * 1000000}; nanosleep(&__ts, NULL); } while (0)
#endif

#define nullable __nullable
#define nonnull  __nonnull

// Scale expects this function
#define expect

// This function was declared in Scale code
#define export

#if __SIZEOF_POINTER__ < 8
#error "Scale is not supported on this platform"
#endif

#ifndef STACK_SIZE
#define STACK_SIZE			4096
#endif

// Define scale-specific signals
#define EX_BAD_PTR			128
#define EX_STACK_OVERFLOW	129
#define EX_STACK_UNDERFLOW	130
#define EX_UNHANDLED_DATA	131
#define EX_IO_ERROR			132
#define EX_INVALID_ARGUMENT	133
#define EX_CAST_ERROR		134
#define EX_THREAD_ERROR		136

#define ssize_t signed long

typedef void* 		scl_any;
typedef long long 	scl_int;
typedef char* 		scl_str;
typedef double 		scl_float;

typedef union {
	scl_int 	i;
	scl_str		s;
	scl_float 	f;
	scl_any	v;
} scl_frame_t;

typedef struct {
	ssize_t     ptr;
	scl_frame_t data[STACK_SIZE];
} scl_stack_t;

void		scl_security_throw(int code, scl_str msg);
void		scl_security_safe_exit(int code);
void		process_signal(int sig_num);

void		print_stacktrace(void);

void		ctrl_push_args(scl_int argc, scl_str argv[]);
void		ctrl_push_string(scl_str c);
void		ctrl_push_long(scl_int n);
void		ctrl_push_double(scl_float d);
void		ctrl_push(scl_any n);
scl_str		ctrl_pop_string(void);
scl_float	ctrl_pop_double(void);
scl_int		ctrl_pop_long(void);
scl_any		ctrl_pop(void);
ssize_t		ctrl_stack_size(void);

scl_any		scl_realloc(scl_any ptr, size_t size);
scl_any		scl_alloc(size_t size);
void		scl_free(scl_any ptr);

scl_any	scl_alloc_struct(size_t size, scl_str type_name);

#endif
