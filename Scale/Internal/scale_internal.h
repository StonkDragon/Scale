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
#include <pthread.h>
#define sleep(s) do { struct timespec __ts = {((s) / 1000), ((s) % 1000) * 1000000}; nanosleep(&__ts, NULL); } while (0)
#endif

#if __has_attribute(always_inline)
#define scl_inline __attribute__((always_inline))
#else
#define scl_inline
#endif

/* Function header */
#define sclDefFunc(name) \
  void fn_ ## name (void)

/* Call a function with the given name. */
#define sclCallFunc(name) \
  fn_ ## name ()

#define operator(a) \
  scl_inline void op_ ## a (void)

#define nullable __nullable
#define nonnull  __nonnull

#if __SIZEOF_POINTER__ < 8
#error "Scale is not supported on this platform"
#endif

#ifndef STACK_SIZE
#define STACK_SIZE			4096
#endif

#define MALLOC_LIMIT		1024
#define MAX_STRING_SIZE		512
#define LONG_AS_STR_LEN		22

// Define scale-specific signals
#define EX_BAD_PTR			128
#define EX_STACK_OVERFLOW	129
#define EX_STACK_UNDERFLOW	130
#define EX_UNHANDLED_DATA	131
#define EX_IO_ERROR			132
#define EX_INVALID_ARGUMENT	133
#define EX_CAST_ERROR		134
#define EX_SAP_ERROR		135
#define EX_THREAD_ERROR		136

#define ssize_t signed long
#define scl_value void*
#define scl_int long long
#define scl_str char*
#define scl_float double

typedef void (*scl_method)(void);

typedef union {
	scl_value 	i;
	scl_float 	f;
} scl_frame_t;

typedef struct {
	ssize_t     ptr;
	scl_frame_t data[STACK_SIZE];
} scl_stack_t;

void		scl_security_throw(int code, scl_str msg);
void		scl_security_required_arg_count(ssize_t n, scl_str func);
void		scl_security_check_null(scl_value n);
void		scl_security_safe_exit(int code);
void		ctrl_set_file(scl_str file);
void		ctrl_set_pos(size_t line, size_t col);
void		process_signal(int sig_num);

void		ctrl_fn_start(scl_str name);
void		ctrl_fn_end(void);
void		ctrl_trace(void);
void		print_stacktrace(void);
scl_method  scl_method_for_name(scl_int name_hash);

void		ctrl_push_string(const scl_str c);
void		ctrl_push_long(scl_int n);
void		ctrl_push_double(scl_float d);
void		ctrl_push(scl_value n);
scl_str		ctrl_pop_string(void);
scl_float	ctrl_pop_double(void);
scl_int		ctrl_pop_long(void);
scl_value	ctrl_pop(void);
ssize_t		ctrl_stack_size(void);
void		sap_open(void);
void		sap_close(void);

void		scl_gc_collect(void);
void		scl_gc_addlocal(scl_value* local);
void		scl_gc_removelocal(scl_value* local);
scl_value	scl_realloc(scl_value ptr, size_t size);
scl_value	scl_alloc(size_t size);
void		scl_free(scl_value ptr);

int			scl_is_struct(void* p);
scl_value	scl_alloc_struct(size_t size);
void		scl_dealloc_struct(scl_value ptr);

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
