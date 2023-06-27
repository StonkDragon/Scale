#if !defined(_SCALE_RUNTIME_H_)
#define _SCALE_RUNTIME_H_

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
#include <stdarg.h>
#include <dlfcn.h>

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
#define sleep(s) do { struct timespec __ts = {((s) / 1000), ((s) % 1000) * 1000000}; nanosleep(&__ts, NULL); } while (0)
#endif

#define GC_DL
#define GC_THREADS
#if __has_include(<gc/gc.h>)
#include <gc/gc.h>
#else
#error <gc/gc.h> not found, which is required for Scale!
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

#if __GNUC__ >= 4
#define _scl_symbol_hidden __attribute__((visibility("hidden")))
#else
#define _scl_symbol_hidden
#endif

#define nullable __nullable
#define nonnull  __nonnull
#define tls __thread

#define _scl_macro_to_string_(x) # x
#define _scl_macro_to_string(x) _scl_macro_to_string_(x)

#define system_allocate(size)				({_callstack.func[_callstack.ptr++] = "<libc malloc()>"; scl_any tmp = malloc(size); _callstack.func[--_callstack.ptr] = NULL; tmp; })
#define system_free(_ptr)					({_callstack.func[_callstack.ptr++] = "<libc free()>"; free(_ptr); _callstack.func[--_callstack.ptr] = NULL; })
#define system_realloc(_ptr, size)			({_callstack.func[_callstack.ptr++] = "<libc realloc()>"; scl_any tmp = realloc(_ptr, size); _callstack.func[--_callstack.ptr] = NULL; tmp; })

// Scale expects this function
#define expect

// This function was declared in Scale code
#define export

#define nil NULL

#define str_of(_cstr) _scl_create_string((_cstr))
#define cstr_of(Struct_str) ((Struct_str)->_data)

#if !defined(STACK_SIZE)
#define STACK_SIZE			131072
#endif

#if !defined(EXCEPTION_DEPTH)
#define EXCEPTION_DEPTH		1024
#endif

#if __has_attribute(aligned)
#define _scl_align __attribute__((aligned(16)))
#else
#define _scl_align
#endif

#if __has_builtin(__builtin_expect)
#define _scl_expect(expr, c) __builtin_expect((expr), (c))
#else
#define _scl_expect(expr, c) (expr)
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

#if defined(__LITTLE_ENDIAN__)
#if __LITTLE_ENDIAN__
#define SCL_KNOWN_LITTLE_ENDIAN
#endif
#endif

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
#endif

#define SCL_int_MAX			INT64_MAX
#define SCL_int_MIN			INT64_MIN
#define SCL_uint_MAX		UINT64_MAX
#define SCL_uint_MIN		((scl_uint) 0)

typedef void*				scl_any;

#define SCL_INT_HEX_FMT		"%lx"
#define SCL_PTR_HEX_FMT		"0x%016lx"
#define SCL_INT_FMT			"%ld"
#define SCL_UINT_FMT		"%lu"

typedef long				scl_int;
typedef size_t				scl_uint;
typedef scl_int				scl_bool;
typedef struct scaleString*	scl_str;
typedef double 				scl_float;
typedef pthread_mutex_t*	mutex_t;

#define SCL_int64_MAX		INT64_MAX
#define SCL_int64_MIN		INT64_MIN
#define SCL_int32_MAX		INT32_MAX
#define SCL_int32_MIN		INT32_MIN
#define SCL_int16_MAX		INT16_MAX
#define SCL_int16_MIN		INT16_MIN
#define SCL_int8_MAX		INT8_MAX
#define SCL_int8_MIN		INT8_MIN
#define SCL_uint64_MAX		UINT64_MAX
#define SCL_uint64_MIN		((scl_uint64) 0)
#define SCL_uint32_MAX		UINT32_MAX
#define SCL_uint32_MIN		((scl_uint32) 0)
#define SCL_uint16_MAX		UINT16_MAX
#define SCL_uint16_MIN		((scl_uint16) 0)
#define SCL_uint8_MAX		UINT8_MAX
#define SCL_uint8_MIN		((scl_uint8) 0)

typedef long long			scl_int64;
typedef int					scl_int32;
typedef short				scl_int16;
typedef char				scl_int8;
typedef unsigned long long	scl_uint64;
typedef unsigned int		scl_uint32;
typedef unsigned short		scl_uint16;
typedef unsigned char		scl_uint8;

struct scaleString {
	scl_int		$__type__;
	scl_int8*	$__type_name__;
	scl_int		$__super__;
	scl_int		$__size__;
	mutex_t		$__mutex__;
	scl_int8*	_data;
	scl_int		_len;
	scl_int		_iter;
	scl_int		_hash;
};

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
} _scl_frame_t;

typedef struct {
	_scl_frame_t*	data;
	scl_int			ptr;
	scl_int			cap;
} _scl_stack_t;

typedef struct {
	scl_int8*		func[EXCEPTION_DEPTH];
	scl_int			ptr;
} _scl_callstack_t;

typedef void(*_scl_lambda)(void);

// Create a new instance of a struct that does not directly extend SclObject
#define NEW(_type, _super)		_scl_alloc_struct(sizeof(struct Struct_ ## _type), (#_type), hash1(# _super))
// Create a new instance of a struct
#define NEW0(_type)				_scl_alloc_struct(sizeof(struct Struct_ ## _type), (#_type), SclObjectHash)
// Create a new instance of SclObject
#define NEWOBJ()				_scl_alloc_struct(sizeof(struct Struct_SclObject), "SclObject", 0)

#define _scl_offsetof(type, member) ((scl_int)&((type*)0)->member)

#undef NEW_

_scl_no_return void	_scl_security_throw(int code, scl_int8* msg, ...);
_scl_no_return void	_scl_security_safe_exit(int code);
_scl_no_return void _scl_callstack_overflow(scl_int8* func);
_scl_no_return void	_scl_unreachable(scl_int8* msg);

void				_scl_default_signal_handler(scl_int sig_num);
void				print_stacktrace(void);

scl_int				_scl_stack_size(void);
_scl_frame_t*		_scl_push();
_scl_frame_t*		_scl_pop();
_scl_frame_t*		_scl_top();
_scl_frame_t*		_scl_offset(scl_int offset);
_scl_frame_t*		_scl_positive_offset(scl_int offset);
void				_scl_popn(scl_int n);
scl_str				_scl_create_string(scl_int8* data);
void				_scl_stack_new();
void				_scl_stack_free();

void				_scl_remove_ptr(scl_any ptr);
scl_int				_scl_get_index_of_ptr(scl_any ptr);
void				_scl_remove_ptr_at_index(scl_int index);
void				_scl_add_ptr(scl_any ptr, scl_int size);
scl_int				_scl_check_allocated(scl_any ptr);
scl_any				_scl_realloc(scl_any ptr, scl_int size);
scl_any				_scl_alloc(scl_int size);
void				_scl_free(scl_any ptr);
scl_int				_scl_sizeof(scl_any ptr);
void				_scl_assert(scl_int b, scl_int8* msg, ...);
void				_scl_check_layout_size(scl_any ptr, scl_int layoutSize, scl_int8* layout);
void				_scl_check_not_nil_argument(scl_int val, scl_int8* name);
void				_scl_checked_cast(scl_any instance, hash target_type, scl_int8* target_type_name);
void				_scl_not_nil_cast(scl_int val, scl_int8* name);
void				_scl_struct_allocation_failure(scl_int val, scl_int8* name);
void				_scl_nil_ptr_dereference(scl_int val, scl_int8* name);
void				_scl_check_not_nil_store(scl_int val, scl_int8* name);
void				_scl_not_nil_return(scl_int val, scl_int8* name);
void				_scl_exception_push();
void				_scl_throw(void* ex);

const hash			hash1(const scl_int8* data);
const hash			hash1len(const scl_int8* data, size_t len);
scl_int				_scl_identity_hash(scl_any obj);
scl_any				_scl_alloc_struct(scl_int size, scl_int8* type_name, hash super);
void				_scl_free_struct(scl_any ptr);
scl_any				_scl_add_struct(scl_any ptr);
scl_int				_scl_is_instance_of(scl_any ptr, hash typeId);
scl_any				_scl_get_method_on_type(hash type, hash method, hash signature);
scl_any				_scl_get_method_handle(hash type, hash method, hash signature);
void				_scl_call_method_or_throw(scl_any instance, hash method, hash signature, int on_super, scl_int8* method_name);
scl_int				_scl_find_index_of_struct(scl_any ptr);
void				_scl_free_struct_no_finalize(scl_any ptr);
void				_scl_remove_stack(_scl_stack_t* stack);
scl_int				_scl_stack_index(_scl_stack_t* stack);
void				_scl_remove_stack_at(scl_int index);

scl_any				_scl_get_struct_by_id(scl_int id);
scl_any				_scl_typeinfo_of(hash type);
scl_int				_scl_binary_search(scl_any* arr, scl_int count, scl_any val);
scl_int				_scl_binary_search_method_index(void** methods, scl_int count, hash id, hash sig);

void				Process$lock(volatile scl_any obj);
void				Process$unlock(volatile scl_any obj);
mutex_t				_scl_mutex_new();

inline void _scl_swap() {
	scl_any b = _scl_pop()->v;
	scl_any a = _scl_pop()->v;
	_scl_push()->v = b;
	_scl_push()->v = a;
}

inline void _scl_over() {
	scl_any c = _scl_pop()->v;
	scl_any b = _scl_pop()->v;
	scl_any a = _scl_pop()->v;
	_scl_push()->v = c;
	_scl_push()->v = b;
	_scl_push()->v = a;
}

inline void _scl_sdup2() {
	scl_any b = _scl_pop()->v;
	scl_any a = _scl_pop()->v;
	_scl_push()->v = a;
	_scl_push()->v = b;
	_scl_push()->v = a;
}

inline void _scl_swap2() {
	scl_any c = _scl_pop()->v;
	scl_any b = _scl_pop()->v;
	scl_any a = _scl_pop()->v;
	_scl_push()->v = b;
	_scl_push()->v = a;
	_scl_push()->v = c;
}

#endif // __SCALE_RUNTIME_H__
