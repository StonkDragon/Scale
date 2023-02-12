#if !defined(_SCALE_INTERNAL_H_)
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

#if defined(_WIN32)
#include <Windows.h>
#include <io.h>
#define sleep(s) Sleep(s)
#define read(fd, buf, n) _read(fd, buf, n)
#define write(fd, buf, n) _write(fd, buf, n)
#if !defined(WINDOWS)
#define WINDOWS
#endif
#else
#include <unistd.h>
#include <pthread.h>
#define sleep(s) do { struct timespec __ts = {((s) / 1000), ((s) % 1000) * 1000000}; nanosleep(&__ts, NULL); } while (0)
#endif

#if !defined(_WIN32) && !defined(__wasm__)
#include <execinfo.h>
#endif

#if __has_include(<setjmp.h>)
#include <setjmp.h>
#else
#error <setjmp.h> not found, which is required for Scale!
#endif

#if __has_attribute(__noreturn__)
#define _scl_no_return __attribute__((__noreturn__))
#else
#define _scl_no_return
#endif

#if __has_attribute(constructor)
#define _scl_constructor __attribute__((constructor))
#else
#if defined(SCL_COMPILER_NO_MAIN)
#error "Can't compile with --no-main"
#endif
#define _scl_constructor
#endif

#if __has_attribute(destructor)
#define _scl_destructor __attribute__((destructor))
#else
#if defined(SCL_COMPILER_NO_MAIN)
#error "Can't compile with --no-main"
#endif
#define _scl_destructor
#endif

#define nullable __nullable
#define nonnull  __nonnull

#define _scl_macro_to_string_(x) # x
#define _scl_macro_to_string(x) _scl_macro_to_string_(x)

// Scale expects this function
#define expect

// This function was declared in Scale code
#define export

#if !defined(STACK_SIZE)
#define STACK_SIZE			131072
#endif

#if !defined(EXCEPTION_DEPTH)
#define EXCEPTION_DEPTH		1024
#endif

#if !defined(SIGABRT)
#define SIGABRT 6
#endif

// Define scale-specific signals
#define EX_BAD_PTR				128
#define EX_STACK_OVERFLOW		129
#define EX_STACK_UNDERFLOW		130
#define EX_UNHANDLED_DATA		131
#define EX_IO_ERROR				132
#define EX_INVALID_ARGUMENT		133
#define EX_CAST_ERROR			134
#define EX_THREAD_ERROR			136
#define EX_ASSERTION_FAIL		137
#define EX_REFLECT_ERROR		138
#define EX_THROWN				139
#define EX_INVALID_BYTE_ORDER	140
#define EX_UNREACHABLE			141

#define ssize_t signed long

#if __SIZEOF_POINTER__ < 8
// WASM or 32-bit system
#if defined(__wasm__)
#define SCL_SYSTEM "WASM32"
#define SCL_WASM32 1
#elif defined(__arm__)
#define SCL_SYSTEM "aarch32"
#define SCL_ARM32  1
#elif defined(__i386__)
#define SCL_SYSTEM "x86"
#define SCL_X86    1
#else
#define SCL_SYSTEM "unknown 32-bit"
#define SCL_UNKNOWN_ARCH 1
#endif
#define SCL_INT_HEX_FMT 	"%x"
#define SCL_INT_FMT		 	"%d"
typedef void*				scl_any;
typedef int		 			scl_int;
typedef unsigned int		scl_uint;
typedef char* 				scl_str;
typedef float 				scl_float;
#else
// 64-bit system
#if defined(__wasm__)
#define SCL_SYSTEM  "WASM64"
#define SCL_WASM64  1
#define SCL_WASM32	1
#elif defined(__aarch64__)
#define SCL_SYSTEM  "aarch64"
#define SCL_AARCH64 1
#define SCL_ARM64   1
#define SCL_ARM32	1
#elif defined(__x86_64__)
#define SCL_SYSTEM  "x86_64"
#define SCL_X64     1
#define SCL_X86_64  1
#define SCL_X86		1
#else
#define SCL_SYSTEM  "unknown 64-bit"
#define SCL_UNKNOWN_ARCH 1
#endif
#define SCL_INT_HEX_FMT 	"%llx"
#define SCL_INT_FMT		 	"%lld"
typedef void*				scl_any;
typedef long long 			scl_int;
typedef unsigned long long	scl_uint;
typedef char* 				scl_str;
typedef double 				scl_float;
#endif

typedef int		 			scl_int32;
typedef float 				scl_float32;
typedef short		 		scl_int16;
typedef char		 		scl_int8;
typedef unsigned int 		scl_uint32;
typedef unsigned short 		scl_uint16;
typedef unsigned char 		scl_uint8;

#if defined(__ANDROID__)
#define SCL_OS_NAME "Android"
#elif defined(__amigaos__)
#define SCL_OS_NAME "AmigaOS"
#elif defined(__bsdi__)
#define SCL_OS_NAME "BSD-like"
#elif defined(__CYGWIN__)
#define SCL_OS_NAME "Windows (Cygwin)"
#define SCL_CYGWIN
#elif defined(__DragonFly__)
#define SCL_OS_NAME "DragonFly"
#elif defined(__FreeBSD__)
#define SCL_OS_NAME "FreeBSD"
#elif defined(__gnu_linux__)
#define SCL_OS_NAME "GNU/Linux"
#elif defined(__GNU__)
#define SCL_OS_NAME "GNU"
#elif defined(macintosh)
#define SCL_OS_NAME "Classic Mac OS"
#elif defined(__APPLE__) && defined(__MACH__)
#define SCL_OS_NAME "macOS"
#elif defined(__minix)
#define SCL_OS_NAME "MINIX"
#elif defined(__MSDOS__)
#define SCL_OS_NAME "MS-DOS"
#elif defined(__NetBSD__)
#define SCL_OS_NAME "NetBSD"
#elif defined(__OpenBSD__)
#define SCL_OS_NAME "OpenBSD"
#elif defined(__OS2__)
#define SCL_OS_NAME "IBM OS/2"
#elif defined(__unix__)
#define SCL_OS_NAME "UNIX"
#elif defined(_WIN32) || defined(_WIN32_WCE)
#define SCL_OS_NAME "Windows"
#else
#define SCL_OS_NAME "Unknown OS"
#endif

typedef unsigned int hash;

typedef union {
	scl_int		i;
	scl_str		s;
	scl_float	f;
	scl_any		v;

	scl_int32	i32;
	scl_float32	f32;
	scl_int16	i16;
	scl_int8	i8;
	scl_uint32	u32;
	scl_uint16	u16;
	scl_uint8	u8;
} _scl_frame_t;

typedef struct {
	scl_int			ptr;
	scl_int			cap;
	_scl_frame_t*	data;
} _scl_stack_t;

typedef struct {
	scl_str file;
	scl_str func;
	scl_int line;
	scl_int col;
	scl_int begin_stack_size;
	scl_int begin_var_count;
	scl_int sp;
} _scl_callframe_t;

typedef struct {
	ssize_t				ptr;
	_scl_callframe_t	data[STACK_SIZE];
} _scl_callstack_t;

typedef void(*_scl_lambda)(void);

_scl_no_return
void			_scl_security_throw(int code, scl_str msg);

_scl_no_return
void			_scl_security_safe_exit(int code);
void			_scl_catch_final(int sig_num);
void			print_stacktrace(void);
void			print_stacktrace_with_file(FILE* trace);

void			ctrl_push_string(scl_str c);
void			ctrl_push_long(scl_int n);
void			ctrl_push_double(scl_float d);
void			ctrl_push(scl_any n);
scl_str			ctrl_pop_string(void);
scl_float		ctrl_pop_double(void);
scl_int			ctrl_pop_long(void);
scl_any			ctrl_pop(void);
ssize_t			ctrl_stack_size(void);
_scl_frame_t*	_scl_push();
_scl_frame_t*	_scl_pop();
_scl_frame_t*	_scl_top();

void			_scl_remove_ptr(scl_any ptr);
scl_int			_scl_get_index_of_ptr(scl_any ptr);
void			_scl_remove_ptr_at_index(scl_int index);
void			_scl_add_ptr(scl_any ptr, size_t size);
scl_int			_scl_check_allocated(scl_any ptr);
scl_any			_scl_realloc(scl_any ptr, size_t size);
scl_any			_scl_alloc(size_t size);
void			_scl_free(scl_any ptr);
void			_scl_assert(scl_int b, scl_str msg);
void			_scl_finalize(void);
void			_scl_unreachable(char* msg);

hash			hash1(char* data);
void			_scl_cleanup_post_func(scl_int depth);
scl_any			_scl_alloc_struct(size_t size, scl_str type_name, hash super);
void			_scl_free_struct(scl_any ptr);
scl_any			_scl_add_struct(scl_any ptr);
scl_int			_scl_struct_is_type(scl_any ptr, hash typeId);
scl_any			_scl_get_method_on_type(hash type, hash method);
size_t			_scl_find_index_of_struct(scl_any ptr);
void			_scl_free_struct_no_finalize(scl_any ptr);

void			_scl_reflect_call(hash func);
void			_scl_reflect_call_method(hash func);
scl_any			_scl_typeinfo_of(hash type);
scl_int			_scl_reflect_find(hash func);
scl_int			_scl_reflect_find_method(hash func);
scl_int			_scl_binary_search(scl_any* arr, scl_int count, scl_any val);
scl_int			_scl_binary_search_method_index(void** methods, scl_int count, hash id);

#endif // __SCALE_INTERNAL_H__
