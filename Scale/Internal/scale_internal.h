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

#ifndef STACK_SIZE
#define STACK_SIZE			131072
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
#define EX_ASSERTION_FAIL	137
#define EX_REFLECT_ERROR	138

#define ssize_t signed long

#if __SIZEOF_POINTER__ < 8
// WASM or 32-bit system
#ifdef __wasm__
#define SCL_SYSTEM "WASM32"
#define SCL_WASM32 1
#elif __arm__
#define SCL_SYSTEM "aarch32"
#define SCL_ARM32  1
#elif __i386__
#define SCL_SYSTEM "x86"
#define SCL_X86    1
#else
#define SCL_SYSTEM  "unknown 32-bit"
#define SCL_UNKNOWN_ARCH 1
#endif
#define SCL_INT_HEX_FMT 	"%x"
#define SCL_INT_FMT		 	"%d"
typedef void*				scl_any;
typedef int		 			scl_int;
typedef char* 				scl_str;
typedef float 				scl_float;
#else
// 64-bit system
#ifdef __wasm__
#define SCL_SYSTEM  "WASM64"
#define SCL_WASM64  1
#elif __aarch64__
#define SCL_SYSTEM  "aarch64"
#define SCL_AARCH64 1
#define SCL_ARM64   1
#elif __x86_64__
#define SCL_SYSTEM  "x86_64"
#define SCL_X64     1
#define SCL_X86_64  1
#else
#define SCL_SYSTEM  "unknown 64-bit"
#define SCL_UNKNOWN_ARCH 1
#endif
#define SCL_INT_HEX_FMT 	"%llx"
#define SCL_INT_FMT		 	"%lld"
typedef void*				scl_any;
typedef long long 			scl_int;
typedef char* 				scl_str;
typedef double 				scl_float;
#endif

#ifdef __ANDROID__
#define SCL_OS_NAME "Android"
#elif __amigaos__
#define SCL_OS_NAME "AmigaOS"
#elif __bsdi__
#define SCL_OS_NAME "BSD-like"
#elif __CYGWIN__
#define SCL_OS_NAME "Windows (Cygwin)"
#define SCL_CYGWIN
#elif __DragonFly__
#define SCL_OS_NAME "DragonFly"
#elif __FreeBSD__
#define SCL_OS_NAME "FreeBSD"
#elif __gnu_linux__
#define SCL_OS_NAME "GNU/Linux"
#elif __GNU__
#define SCL_OS_NAME "GNU"
#elif macintosh
#define SCL_OS_NAME "Classic Mac OS"
#elif __APPLE__ && __MACH__
#define SCL_OS_NAME "macOS"
#elif __minix
#define SCL_OS_NAME "MINIX"
#elif __MSDOS__
#define SCL_OS_NAME "MS-DOS"
#elif __NetBSD__
#define SCL_OS_NAME "NetBSD"
#elif __OpenBSD__
#define SCL_OS_NAME "OpenBSD"
#elif __OS2__
#define SCL_OS_NAME "IBM OS/2"
#elif __unix__
#define SCL_OS_NAME "UNIX"
#elif _WIN32 || _WIN32_WCE
#define SCL_OS_NAME "Windows"
#else
#define SCL_OS_NAME "Unknown OS"
#endif

typedef unsigned int		hash;

typedef union {
	scl_int		i;
	scl_str		s;
	scl_float	f;
	scl_any		v;
} scl_frame_t;

typedef struct {
	ssize_t		ptr;
	scl_frame_t	data[STACK_SIZE];
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
void		scl_assert(scl_int b, scl_str msg);
void		scl_finalize();

hash		hash1(char* data);
scl_any		scl_alloc_struct(size_t size, scl_str type_name, hash super);
void		scl_free_struct(scl_any ptr);
scl_any		scl_add_struct(scl_any ptr);
scl_int		scl_struct_is_type(scl_any ptr, hash typeId);
scl_any		scl_get_method_on_type(hash type, hash method);
size_t		scl_find_index_of_struct(scl_any ptr);
void		scl_free_struct_no_finalize(scl_any ptr);

void		scl_reflect_call(hash func);
void		scl_reflect_call_method(hash func);
scl_any		scl_typeinfo_of(hash type);
scl_int		scl_reflect_find(hash func);
scl_int		scl_reflect_find_method(hash func);

#endif
