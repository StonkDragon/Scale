#if !defined(_SCALE_RUNTIME_H_)
#define _SCALE_RUNTIME_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <setjmp.h>

#define GC_THREADS
#include <gc/gc.h>

#if defined(_WIN32)
#include <Windows.h>
#include <io.h>
#define sleep(s) Sleep(s)
#define read(fd, buf, n) _read(fd, buf, n)
#define write(fd, buf, n) _write(fd, buf, n)
#if !defined(WINDOWS)
#define WINDOWS
#endif
#endif

#if !defined(_WIN32) && !defined(__wasm__)
#include <execinfo.h>
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

#if __has_attribute(noinline)
#define _scl_noinline __attribute__((noinline))
#else
#define _scl_noinline
#endif

#define nullable __nullable
#define nonnull  __nonnull
#define tls __thread

#define _scl_macro_to_string_(x) # x
#define _scl_macro_to_string(x) _scl_macro_to_string_(x)

// Scale expects this function
#define expect

// This function was declared in Scale code
#define export

#undef nil
#define nil NULL

// Creates a new string from a C string
// The given C string is not copied
#define str_of_exact(_cstr)	_scl_create_string((_cstr))

#define cstr_of(Struct_str)	((Struct_str)->data)

#if !defined(STACK_SIZE)
#define STACK_SIZE			131072
#endif

#if __has_builtin(__builtin_expect)
#define _scl_expect(expr, c) __builtin_expect((expr), (c))
#else
#define _scl_expect(expr, c) (expr)
#endif

#if __has_attribute(always_inline)
#define _scl_always_inline __attribute__((always_inline))
#else
#define _scl_always_inline
#endif

#define AKA(_sym) __asm(_scl_macro_to_string(__USER_LABEL_PREFIX__) # _sym)

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

typedef void*				scl_any;

#if __SIZEOF_LONG__ == 8
typedef long				scl_int;
typedef unsigned long		scl_uint;
#define SCL_INT_FMT			"%ld"
#define SCL_UINT_FMT		"%lu"
#define SCL_INT_HEX_FMT		"%lx"
#define SCL_PTR_HEX_FMT		"0x%016lx"
#elif __SIZEOF_LONG_LONG__ == 8
typedef long long			scl_int;
typedef unsigned long long	scl_uint;
#define SCL_INT_FMT			"%lld"
#define SCL_UINT_FMT		"%llu"
#define SCL_INT_HEX_FMT		"%llx"
#define SCL_PTR_HEX_FMT		"0x%016llx"
#else
#error "Can't find a 64-bit integer type"
#endif

#define SCL_int_MIN			((1L << (sizeof(scl_int) * 8 - 1)) - 1)
#define SCL_int_MAX			 (1L << (sizeof(scl_int) * 8 - 1))
#define SCL_uint_MIN		((scl_uint) 0)
#define SCL_uint_MAX		((scl_uint) -1)

#if __SIZEOF_DOUBLE__ == 8
typedef double 				scl_float;
#elif __SIZEOF_LONG_DOUBLE__ == 8
typedef long double			scl_float;
#elif __SIZEOF_FLOAT__ == 8
typedef float				scl_float;
#else
#error "Can't find a 64-bit floating point type"
#endif

typedef scl_int				scl_bool;

typedef struct scale_string* scl_str;

#define SCL_int64_MAX		((1LL << (sizeof(scl_int64) * 8 - 1)) - 1)
#define SCL_int64_MIN		 (1LL << (sizeof(scl_int64) * 8 - 1))
#define SCL_int32_MAX		((1 << (sizeof(scl_int32) * 8 - 1)) - 1)
#define SCL_int32_MIN		 (1 << (sizeof(scl_int32) * 8 - 1))
#define SCL_int16_MAX		((1 << (sizeof(scl_int16) * 8 - 1)) - 1)
#define SCL_int16_MIN		 (1 << (sizeof(scl_int16) * 8 - 1))
#define SCL_int8_MAX		((1 << (sizeof(scl_int8) * 8 - 1)) - 1)
#define SCL_int8_MIN		 (1 << (sizeof(scl_int8) * 8 - 1))
#define SCL_uint64_MAX		((scl_uint64) -1)
#define SCL_uint64_MIN		((scl_uint64) 0)
#define SCL_uint32_MAX		((scl_uint32) -1)
#define SCL_uint32_MIN		((scl_uint32) 0)
#define SCL_uint16_MAX		((scl_uint16) -1)
#define SCL_uint16_MIN		((scl_uint16) 0)
#define SCL_uint8_MAX		((scl_uint8) -1)
#define SCL_uint8_MIN		((scl_uint8) 0)

typedef long long			scl_int64;
typedef int					scl_int32;
typedef short				scl_int16;
typedef char				scl_int8;
typedef unsigned long long	scl_uint64;
typedef unsigned int		scl_uint32;
typedef unsigned short		scl_uint16;
typedef unsigned char		scl_uint8;

typedef void*(*_scl_lambda)();

typedef unsigned int ID_t;

struct _scl_methodinfo {
	const ID_t							pure_name;
	const ID_t							signature;
};

typedef struct TypeInfo {
	const ID_t							type;
	const _scl_lambda*					vtable;
	const struct TypeInfo*				super;
	const scl_int8*						type_name;
	const size_t						size;
	const struct _scl_methodinfo* const	vtable_info;
} TypeInfo;

struct scale_string {
	const _scl_lambda* const			$fast;
	const TypeInfo* const				$statics;
	const scl_any						$mutex;
	scl_int8*							data;
	scl_int								length;
	scl_int								hash;
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
#elif defined(__APPLE__) && defined(__IOS__)
#define SCL_OS_NAME "iOS"
#elif defined(__APPLE__) && defined(__TVOS__)
#define SCL_OS_NAME "tvOS"
#elif defined(__APPLE__) && defined(__WATCHOS__)
#define SCL_OS_NAME "watchOS"
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

typedef union _scl_frame {
	scl_int				i;
	scl_str				s;
	scl_float			f;
	scl_any				v;
	struct {
		_scl_lambda*	$fast;
		TypeInfo*		$statics;
		scl_any			$mutex;
	}*					o;
	struct {
		_scl_lambda*	$fast;
		TypeInfo*		$statics;
		scl_any			$mutex;
		scl_int			__tag;
		scl_any			__value;
	}*					u;
	const scl_int8*		cs;
} _scl_frame_t;

// Create a new instance of a struct
#define ALLOC(_type)	({ \
	extern const TypeInfo _scl_ti_ ## _type __asm("typeinfo for " #_type); \
	(scl_ ## _type) _scl_alloc_struct(sizeof(struct Struct_ ## _type), &_scl_ti_ ## _type); \
})

// Try to cast the given instance to the given type
#define CAST0(_obj, _type, _type_hash) ((scl_ ## _type) _scl_checked_cast(_obj, _type_hash, #_type))
// Try to cast the given instance to the given type
#define CAST(_obj, _type) CAST0(_obj, _type, type_id(#_type))

struct _scl_exception_handler {
	scl_int marker;
	scl_any exception;
	jmp_buf jmp;
};

struct _scl_backtrace {
	scl_int marker;
	const scl_int8* func_name;
};

struct memory_layout {
	scl_int32 marker;
	scl_int allocation_size;
	scl_int8 is_instance:4;
	scl_int8 is_array:4;
};

typedef struct memory_layout memory_layout_t;

#define CONCAT(a, b) CONCAT_(a, b)
#define CONCAT_(a, b) a ## b

// Get the offset of a member in a struct
#define _scl_offsetof(type, member) ((scl_int)&((type*)0)->member)

#define REINTERPRET_CAST(_type, _value) (*(_type*)&(_value))

#define EXCEPTION_HANDLER_MARKER \
					0xF0E1D2C3B4A59687ULL

#define TRACE_MARKER \
					0xF7E8D9C0B1A28384ULL

#define TRY			struct _scl_exception_handler _scl_exception_handler = { .marker = EXCEPTION_HANDLER_MARKER }; \
						if (setjmp(_scl_exception_handler.jmp) != 666)

#define				SCL_BACKTRACE(_func_name) \
						struct _scl_backtrace __scl_backtrace_cur __attribute__((cleanup(_scl_trace_remove))) = { .marker = TRACE_MARKER, .func_name = (_func_name) }; \
						__asm__ __volatile__("" : : "r" (__scl_backtrace_cur.func_name) : "memory")

void				_scl_trace_remove(const struct _scl_backtrace*);

#define				SCL_FUNCTION_LOCK(_obj) \
						const volatile scl_any __scl_lock_obj __attribute__((cleanup(_scl_unlock_ptr))) = _obj; _scl_lock(__scl_lock_obj)

void				_scl_unlock_ptr(void* lock_ptr);

#define				SCL_AUTO_DELETE \
						__attribute__((cleanup(_scl_delete_ptr)))

void				_scl_delete_ptr(void* ptr);

#define				SCL_ASSUME(val, what, ...) \
						if (_scl_expect(!(val), 0)) { \
							size_t msg_len = strlen((what)) + 256; \
							scl_int8* msg = (scl_int8*) GC_malloc(sizeof(scl_int8) * msg_len); \
							snprintf(msg, msg_len, (what) __VA_OPT__(,) __VA_ARGS__); \
							_scl_assert(0, msg); \
						}

typedef void(*mainFunc)(/* args: Array */ scl_any);

#include "preproc.h"

#define				func_ptr_on(instance, methodIdentifier) \
						_scl_get_vtable_function(0, (instance), (methodIdentifier))

#define				func_ptr_on_super(instance, methodIdentifier) \
						_scl_get_vtable_function(1, (instance), (methodIdentifier))

// call a method on an instance
// return value is undefined if the method return type is `none`
#define				virtual_call(instance, methodIdentifier, ...) \
						_Pragma("clang diagnostic push") \
						_Pragma("clang diagnostic ignored \"-Wincompatible-pointer-types-discards-qualifiers\"") \
						_Pragma("clang diagnostic ignored \"-Wint-conversion\"") \
						({ \
							scl_any _tmp = (instance); \
							(((scl_any(*)(scl_any __VA_OPT__(, _SCL_PREPROC_REPEAT_N(_SCL_PREPROC_NARG(__VA_ARGS__), scl_any)))) func_ptr_on(_tmp, (methodIdentifier))))(_tmp, ##__VA_ARGS__); \
						}) \
						_Pragma("clang diagnostic pop")
// call a method on an instance
#define				virtual_callf(instance, methodIdentifier, ...) \
						_Pragma("clang diagnostic push") \
						_Pragma("clang diagnostic ignored \"-Wincompatible-pointer-types-discards-qualifiers\"") \
						_Pragma("clang diagnostic ignored \"-Wint-conversion\"") \
						({ \
							scl_any _tmp = (instance); \
							(((scl_float(*)(scl_any __VA_OPT__(, _SCL_PREPROC_REPEAT_N(_SCL_PREPROC_NARG(__VA_ARGS__), scl_any)))) func_ptr_on(_tmp, (methodIdentifier))))(_tmp, ##__VA_ARGS__); \
						}) \
						_Pragma("clang diagnostic pop")
// call a method on the super class of an instance
// return value is undefined if the method return type is `none`
#define				virtual_call_super(instance, methodIdentifier, ...) \
						_Pragma("clang diagnostic push") \
						_Pragma("clang diagnostic ignored \"-Wincompatible-pointer-types-discards-qualifiers\"") \
						_Pragma("clang diagnostic ignored \"-Wint-conversion\"") \
						({ \
							scl_any _tmp = (instance); \
							(((scl_any(*)(scl_any __VA_OPT__(, _SCL_PREPROC_REPEAT_N(_SCL_PREPROC_NARG(__VA_ARGS__), scl_any)))) func_ptr_on_super(_tmp, (methodIdentifier))))(_tmp, ##__VA_ARGS__); \
						}) \
						_Pragma("clang diagnostic pop")
// call a method on the super class of an instance
#define				virtual_call_superf(instance, methodIdentifier, ...) \
						_Pragma("clang diagnostic push") \
						_Pragma("clang diagnostic ignored \"-Wincompatible-pointer-types-discards-qualifiers\"") \
						_Pragma("clang diagnostic ignored \"-Wint-conversion\"") \
						({ \
							scl_any _tmp = (instance); \
							(((scl_float(*)(scl_any __VA_OPT__(, _SCL_PREPROC_REPEAT_N(_SCL_PREPROC_NARG(__VA_ARGS__), scl_any)))) func_ptr_on_super(_tmp, (methodIdentifier))))(_tmp, ##__VA_ARGS__); \
						}) \
						_Pragma("clang diagnostic pop")

_scl_no_return void	_scl_runtime_error(int code, const scl_int8* msg, ...);
_scl_no_return void	_scl_runtime_catch(scl_any ex);
_scl_no_return void	_scl_throw(scl_any ex);

_scl_constructor
void				_scl_setup(void);

#define				_scl_create_string(_data) ({ \
							const scl_int8* _data_ = (scl_int8*) (_data); \
							extern const TypeInfo _scl_ti_str __asm("typeinfo for str"); \
							scl_str self = (scl_str) _scl_alloc_struct(sizeof(struct scale_string), &_scl_ti_str); \
							self->data = (scl_int8*) (_data_); \
							self->length = strlen((_data_)); \
							self->hash = type_id((_data_)); \
							self; \
						})

#define				_scl_string_with_hash_len(_data, _hash, _len) ({ \
							const scl_int8* _data_ = (scl_int8*) (_data); \
							scl_int _len_ = (scl_int) (_len); \
							scl_int _hash_ = (scl_int) (_hash); \
							extern const TypeInfo _scl_ti_str __asm("typeinfo for str"); \
							scl_str self = (scl_str) _scl_alloc_struct(sizeof(struct scale_string), &_scl_ti_str); \
							self->data = (scl_int8*) (_data_); \
							self->length = (_len_); \
							self->hash = (_hash_); \
							self; \
						})

scl_any				_scl_realloc(scl_any ptr, scl_int size);
scl_any				_scl_alloc(scl_int size);
void				_scl_free(scl_any ptr);
scl_int				_scl_sizeof(scl_any ptr);
void				_scl_assert(scl_int b, const scl_int8* msg, ...);

const ID_t			type_id(const scl_int8* data);

scl_int				_scl_identity_hash(scl_any obj);
scl_any				_scl_alloc_struct(scl_int size, const TypeInfo* statics);
scl_int				_scl_is_instance(scl_any ptr);
scl_int				_scl_is_instance_of(scl_any ptr, ID_t type_id);
_scl_lambda			_scl_get_method_on_type(scl_any type, ID_t method, ID_t signature, int onSuper);
scl_any				_scl_get_vtable_function(scl_int onSuper, scl_any instance, const scl_int8* methodIdentifier);
scl_any				_scl_checked_cast(scl_any instance, ID_t target_type, const scl_int8* target_type_name);
scl_int8*			_scl_typename_or_else(scl_any instance, const scl_int8* else_);
scl_any				_scl_cvarargs_to_array(va_list args, scl_int count);
void				_scl_lock(scl_any obj);
void				_scl_unlock(scl_any obj);

scl_any				_scl_new_array_by_size(scl_int num_elems, scl_int elem_size);
scl_int				_scl_is_array(scl_any* arr);
scl_any*			_scl_multi_new_array_by_size(scl_int dimensions, scl_int sizes[], scl_int elem_size);
scl_int				_scl_array_size(scl_any* arr);
void				_scl_array_check_bounds_or_throw(scl_any* arr, scl_int index);
scl_any*			_scl_array_resize(scl_any* arr, scl_int new_size);
scl_any*			_scl_array_sort(scl_any* arr);
scl_any*			_scl_array_reverse(scl_any* arr);
scl_str				_scl_array_to_string(scl_any* arr);

// BEGIN C++ Concurrency API wrappers
void				cxx_std_thread_join(scl_any thread);
void				cxx_std_thread_delete(scl_any thread);
void				cxx_std_thread_detach(scl_any thread);
scl_any				cxx_std_thread_new(void);
scl_any				cxx_std_thread_new_with_args(scl_any Thread$run, scl_any args);
void				cxx_std_this_thread_yield(void);

scl_any				cxx_std_recursive_mutex_new(void);
void				cxx_std_recursive_mutex_delete(scl_any mutex);
void				cxx_std_recursive_mutex_lock(scl_any mutex);
void				cxx_std_recursive_mutex_unlock(scl_any mutex);
// END C++ Concurrency API wrappers

#define _scl_push()						(localstack++)
#define _scl_pop()						(--localstack)
#define _scl_positive_offset(offset)	(localstack + offset)
#define _scl_top()						(localstack - 1)
#define _scl_popn(n)					(localstack -= (n))

#define _scl_swap() \
	({ \
		_scl_frame_t tmp = *(localstack - 1); \
		*(localstack - 1) = *(localstack - 2); \
		*(localstack - 2) = tmp; \
	})

#define _scl_over() \
	({ \
		_scl_frame_t tmp = *(localstack - 1); \
		*(localstack - 1) = *(localstack - 3); \
		*(localstack - 3) = tmp; \
	})

#define _scl_sdup2() \
	({ \
		*(localstack) = *(localstack - 2); \
		localstack++; \
	})

#define _scl_swap2() \
	({ \
		_scl_frame_t tmp = *(localstack - 2); \
		*(localstack - 2) = *(localstack - 3); \
		*(localstack - 3) = tmp; \
	})

#define _scl_rot() \
	({ \
		_scl_frame_t tmp = *(localstack - 3); \
		*(localstack - 3) = *(localstack - 2); \
		*(localstack - 2) = *(localstack - 1); \
		*(localstack - 1) = tmp; \
	})

#define _scl_unrot() \
	({ \
		_scl_frame_t tmp = *(localstack - 1); \
		*(localstack - 1) = *(localstack - 2); \
		*(localstack - 2) = *(localstack - 3); \
		*(localstack - 3) = tmp; \
	})

#define _scl_add(a, b)			(a) + (b)
#define _scl_sub(a, b)			(a) - (b)
#define _scl_mul(a, b)			(a) * (b)
#define _scl_div(a, b)			(a) / (b)
#define _scl_pow(a, b)			pow((a), (b))
#define _scl_mod(a, b)			(a) % (b)
#define _scl_land(a, b)			(a) & (b)
#define _scl_lor(a, b)			(a) | (b)
#define _scl_lxor(a, b)			(a) ^ (b)
#define _scl_lnot(a)			~((a))
#define _scl_lsr(a, b)			(a) >> (b)
#define _scl_lsl(a, b)			(a) << (b)
#define _scl_eq(a, b)			(a) == (b)
#define _scl_ne(a, b)			(a) != (b)
#define _scl_gt(a, b)			(a) > (b)
#define _scl_ge(a, b)			(a) >= (b)
#define _scl_lt(a, b)			(a) < (b)
#define _scl_le(a, b)			(a) <= (b)
#define _scl_and(a, b)			(a) && (b)
#define _scl_or(a, b)			(a) || (b)
#define _scl_not(a)				!(a)
#define _scl_at(a)				(*((a)))
#define _scl_inc(a)				(++(a))
#define _scl_dec(a)				(--(a))


#if !defined(__CHAR_BIT__)
#define __CHAR_BIT__ 8
#endif

#define _scl_ror(a, b)			({ scl_int _a = (a); scl_int _b = (b); ((_a) >> (_b)) | ((_a) << ((sizeof(scl_int) << 3) - (_b))); })
#define _scl_rol(a, b)			({ scl_int _a = (a); scl_int _b = (b); ((_a) << (_b)) | ((_a) >> ((sizeof(scl_int) << 3) - (_b))); })

#if defined(__cplusplus)
}
#endif

#endif // __SCALE_RUNTIME_H__
