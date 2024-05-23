#if !defined(_SCALE_RUNTIME_H_)
#define _SCALE_RUNTIME_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
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
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <io.h>
#define sleep(s) Sleep(s)
#define read(fd, buf, n) _read(fd, buf, n)
#define write(fd, buf, n) _write(fd, buf, n)
#if !defined(WINDOWS)
#define WINDOWS
#endif
#else
// #include <unistd.h>
#define sleep(s) do { struct timespec __ts = {((s) / 1000), ((s) % 1000) * 1000000}; nanosleep(&__ts, NULL); } while (0)
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
#error "Scale requires a compiler with __attribute__((constructor)) support"
#endif

#if __has_attribute(destructor)
#define _scl_destructor __attribute__((destructor))
#else
#error "Scale requires a compiler with __attribute__((destructor)) support"
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

#define cstr_of(_str)		((_str)->data)

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

#define AKA(_sym)				asm(_scl_macro_to_string(__USER_LABEL_PREFIX__) # _sym)

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

#define SCL_int_MAX			 (1L << (sizeof(scl_int) * 8 - 1))
#define SCL_int_MASK		((scl_int) -1)
#define SCL_int_MIN			((1L << (sizeof(scl_int) * 8 - 1)) - 1)
#define SCL_uint_MAX		((scl_uint) -1)
#define SCL_uint_MASK		((scl_uint) -1)
#define SCL_uint_MIN		((scl_uint) 0)

typedef float				scl_float32;
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
#define SCL_int64_MASK		((scl_int64) -1)
#define SCL_int64_MIN		 (1LL << (sizeof(scl_int64) * 8 - 1))
#define SCL_int32_MAX		((1 << (sizeof(scl_int32) * 8 - 1)) - 1)
#define SCL_int32_MASK		((scl_int32) -1)
#define SCL_int32_MIN		 (1 << (sizeof(scl_int32) * 8 - 1))
#define SCL_int16_MAX		((1 << (sizeof(scl_int16) * 8 - 1)) - 1)
#define SCL_int16_MASK		((scl_int16) -1)
#define SCL_int16_MIN		 (1 << (sizeof(scl_int16) * 8 - 1))
#define SCL_int8_MAX		((1 << (sizeof(scl_int8) * 8 - 1)) - 1)
#define SCL_int8_MASK		((scl_int8) -1)
#define SCL_int8_MIN		 (1 << (sizeof(scl_int8) * 8 - 1))
#define SCL_uint64_MAX		((scl_uint64) -1)
#define SCL_uint64_MASK		((scl_uint64) -1)
#define SCL_uint64_MIN		((scl_uint64) 0)
#define SCL_uint32_MAX		((scl_uint32) -1)
#define SCL_uint32_MASK		((scl_uint32) -1)
#define SCL_uint32_MIN		((scl_uint32) 0)
#define SCL_uint16_MAX		((scl_uint16) -1)
#define SCL_uint16_MASK		((scl_uint16) -1)
#define SCL_uint16_MIN		((scl_uint16) 0)
#define SCL_uint8_MAX		((scl_uint8) -1)
#define SCL_uint8_MASK		((scl_uint8) -1)
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

typedef scl_uint ID_t;

#if defined(__ANDROID__)
#define SCL_OS_NAME "Android"
#elif defined(__amigaos__)
#define SCL_OS_NAME "AmigaOS"
#elif defined(__bsdi__)
#define SCL_OS_NAME "BSD-like"
#elif defined(__CYGWIN__)
#define SCL_OS_NAME "Windows (Cygwin)"
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

// Create a new instance of a struct
#define ALLOC(_type)	({ \
	extern const TypeInfo _scl_ti_ ## _type __asm("__T" #_type); \
	(scl_ ## _type) _scl_alloc_struct(&_scl_ti_ ## _type); \
})

// Try to cast the given instance to the given type
#define CAST0(_obj, _type, _type_hash) ((scl_ ## _type) _scl_checked_cast(_obj, _type_hash, #_type))
// Try to cast the given instance to the given type
#define CAST(_obj, _type) CAST0(_obj, _type, type_id(#_type))

struct _scl_exception_handler {
	scl_uint marker;
	scl_any exception;
	jmp_buf jmp;
	scl_any finalization_data;
	void (*finalizer)(scl_any);
};

struct _scl_backtrace {
	scl_uint marker;
	const scl_int8* func_name;
};

typedef struct {
	scl_uint flags;
	scl_int size;
	scl_int array_elem_size;
} memory_layout_t;

#define MEM_FLAG_INSTANCE	0b00000001
#define MEM_FLAG_ARRAY		0b00000010

struct _scl_methodinfo {
	const ID_t							pure_name;
	const ID_t							signature;
};

typedef struct {
	memory_layout_t layout;
	_scl_lambda funcs[];
} _scl_vtable;

typedef struct {
	memory_layout_t layout;
	struct _scl_methodinfo infos[];
} _scl_methodinfo_t;

typedef struct TypeInfo {
	const ID_t							type;
	const struct TypeInfo*				super;
	const scl_int8*						type_name;
	const size_t						size;
	const struct _scl_methodinfo* const	vtable_info;
	const _scl_lambda					vtable[];
} TypeInfo;

struct scale_string {
	const TypeInfo*						$type;
	scl_any								$mutex;
	scl_int8*							data;
	scl_int								length;
	scl_int								hash;
};

#define CONCAT(a, b) CONCAT_(a, b)
#define CONCAT_(a, b) a ## b

// Get the offset of a member in a struct
#define _scl_offsetof(type, member) ((scl_int)&((type*)0)->member)

#define REINTERPRET_CAST(_type, _value) ({ \
	typeof(_value) _tmp ## __LINE__ = (_value); \
	*(_type*) &_tmp ## __LINE__; \
})

#define _scl_async(x, at, ...) ({ \
	struct _args_ ## at* args = malloc(sizeof(struct _args_ ## at)); \
	struct _args_ ## at args_ = { __VA_ARGS__ }; \
	memcpy(args, &args_, sizeof(struct _args_ ## at)); \
	_scl_push(scl_any, cxx_async((x), args)); \
})
#define _scl_await(rtype) (_scl_top(rtype) = cxx_await(_scl_top(scl_any)))
#define _scl_await_void() cxx_await(_scl_pop(scl_any))

#define 			EXCEPTION_HANDLER_MARKER \
					0xF0E1D2C3B4A59687ULL

#define 			TRACE_MARKER \
					0xF7E8D9C0B1A28384ULL

#define 			TRY \
						struct _scl_exception_handler _scl_exception_handler = { .marker = EXCEPTION_HANDLER_MARKER }; \
						if (setjmp(_scl_exception_handler.jmp) != 666)
#define 			TRY_FINALLY(_then, _with) \
						struct _scl_exception_handler _scl_exception_handler = { .marker = EXCEPTION_HANDLER_MARKER, .finalizer = _then, .finalization_data = _with }; \
						if (setjmp(_scl_exception_handler.jmp) != 666)

#define				SCL_BACKTRACE(_func_name) \
						struct _scl_backtrace __scl_backtrace_cur __attribute__((cleanup(_scl_trace_remove))) = { .marker = TRACE_MARKER, .func_name = (_func_name) }

void				_scl_trace_remove(struct _scl_backtrace*);

#define				SCL_FUNCTION_LOCK(_obj) \
						const volatile scl_any __scl_lock_obj __attribute__((cleanup(_scl_unlock_ptr))) = _obj; _scl_lock(__scl_lock_obj)

void				_scl_unlock_ptr(void* lock_ptr);

#define				SCL_AUTO_DELETE \
						__attribute__((cleanup(_scl_delete_ptr)))

void				_scl_delete_ptr(void* ptr);

#define				SCL_ASSUME(val, what, ...) \
						if (_scl_expect(!(val), 0)) { \
							size_t msg_len = snprintf(NULL, 0, (what) __VA_OPT__(,) __VA_ARGS__); \
							scl_int8 msg[sizeof(scl_int8) * msg_len]; \
							snprintf(msg, msg_len, (what) __VA_OPT__(,) __VA_ARGS__); \
							_scl_assert(0, msg); \
						}

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
							scl_any _tmp ## __LINE__ = (instance); \
							(((scl_any(*)(scl_any __VA_OPT__(, _SCL_PREPROC_REPEAT_N(_SCL_PREPROC_NARG(__VA_ARGS__), scl_any)))) func_ptr_on(_tmp ## __LINE__, (methodIdentifier))))(_tmp ## __LINE__, ##__VA_ARGS__); \
						}) \
						_Pragma("clang diagnostic pop")
// call a method on an instance
#define				virtual_callf(instance, methodIdentifier, ...) \
						_Pragma("clang diagnostic push") \
						_Pragma("clang diagnostic ignored \"-Wincompatible-pointer-types-discards-qualifiers\"") \
						_Pragma("clang diagnostic ignored \"-Wint-conversion\"") \
						({ \
							scl_any _tmp ## __LINE__ = (instance); \
							(((scl_float(*)(scl_any __VA_OPT__(, _SCL_PREPROC_REPEAT_N(_SCL_PREPROC_NARG(__VA_ARGS__), scl_any)))) func_ptr_on(_tmp ## __LINE__, (methodIdentifier))))(_tmp ## __LINE__, ##__VA_ARGS__); \
						}) \
						_Pragma("clang diagnostic pop")
// call a method on the super class of an instance
// return value is undefined if the method return type is `none`
#define				virtual_call_super(instance, methodIdentifier, ...) \
						_Pragma("clang diagnostic push") \
						_Pragma("clang diagnostic ignored \"-Wincompatible-pointer-types-discards-qualifiers\"") \
						_Pragma("clang diagnostic ignored \"-Wint-conversion\"") \
						({ \
							scl_any _tmp ## __LINE__ = (instance); \
							(((scl_any(*)(scl_any __VA_OPT__(, _SCL_PREPROC_REPEAT_N(_SCL_PREPROC_NARG(__VA_ARGS__), scl_any)))) func_ptr_on_super(_tmp ## __LINE__, (methodIdentifier))))(_tmp ## __LINE__, ##__VA_ARGS__); \
						}) \
						_Pragma("clang diagnostic pop")
// call a method on the super class of an instance
#define				virtual_call_superf(instance, methodIdentifier, ...) \
						_Pragma("clang diagnostic push") \
						_Pragma("clang diagnostic ignored \"-Wincompatible-pointer-types-discards-qualifiers\"") \
						_Pragma("clang diagnostic ignored \"-Wint-conversion\"") \
						({ \
							scl_any _tmp ## __LINE__ = (instance); \
							(((scl_float(*)(scl_any __VA_OPT__(, _SCL_PREPROC_REPEAT_N(_SCL_PREPROC_NARG(__VA_ARGS__), scl_any)))) func_ptr_on_super(_tmp ## __LINE__, (methodIdentifier))))(_tmp ## __LINE__, ##__VA_ARGS__); \
						}) \
						_Pragma("clang diagnostic pop")

_scl_no_return void	_scl_runtime_error(int code, const scl_int8* msg, ...);
_scl_no_return void	_scl_runtime_catch(scl_any ex);
_scl_no_return void	_scl_throw(scl_any ex);

_scl_constructor
void				_scl_setup(void);

#define _scl_create_string(_data) ({ \
						const scl_int8* _data_ = (scl_int8*) (_data); \
						scl_str self = ALLOC(str); \
						self->length = strlen((_data_)); \
						self->data = (scl_int8*) _scl_migrate_foreign_array(_data_, self->length, sizeof(scl_int8)); \
						self->hash = type_id((_data_)); \
						self; \
					})

#define _scl_uninitialized_constant_ctype(_type) ({ \
						static _scl_symbol_hidden struct { \
							memory_layout_t layout; \
							_type data; \
						} _constant __asm__("l_scl_inline_constant" _scl_macro_to_string(__COUNTER__)) = { \
							.layout = { \
								.array_elem_size = 0, \
								.flags = MEM_FLAG_INSTANCE, \
								.size = sizeof(_type), \
							}, \
							.data = {}, \
						}; \
						&((typeof(_constant)*) _scl_mark_static(&(_constant.layout)))->data; \
					})

#define _scl_uninitialized_constant(_type) ({ \
						extern const TypeInfo _scl_ti_ ## _type __asm__("__T" #_type); \
						scl_ ## _type _t; \
						static _scl_symbol_hidden struct { \
							memory_layout_t layout; \
							typeof(*_t) data; \
						} _constant __asm__("l_scl_inline_constant" _scl_macro_to_string(__COUNTER__)) = { \
							.layout = { \
								.array_elem_size = 0, \
								.flags = MEM_FLAG_INSTANCE, \
								.size = sizeof(*_t), \
							}, \
							.data = { \
								.$type = &_scl_ti_ ## _type, \
							}, \
						}; \
						&((typeof(_constant)*) _scl_mark_static(&(_constant.layout)))->data; \
					})

#define _scl_stack_alloc(_type) ({ \
						extern const TypeInfo _scl_ti_ ## _type __asm__("__T" #_type); \
						scl_ ## _type _t; \
						struct { \
							memory_layout_t layout; \
							typeof(*_t) data; \
						}* tmp ## __LINE__ = _scl_scope_alloc(sizeof(*tmp ## __LINE__)); \
						tmp ## __LINE__->layout.size = sizeof(tmp ## __LINE__->data); \
						tmp ## __LINE__->layout.flags = MEM_FLAG_INSTANCE; \
						tmp ## __LINE__->data.$type = &_scl_ti_ ## _type; \
						&(tmp ## __LINE__->data); \
					})

#define _scl_stack_alloc_ctype(_type) ({ \
						struct { \
							memory_layout_t layout; \
							_type data; \
						}* tmp ## __LINE__ = _scl_scope_alloc(sizeof(*tmp ## __LINE__)); \
						tmp ## __LINE__->layout.size = sizeof(tmp ## __LINE__->data); \
						tmp ## __LINE__->layout.flags = MEM_FLAG_INSTANCE; \
						&(tmp ## __LINE__->data); \
					})
#define _scl_stack_alloc_array(_type, _size) ({ \
						struct { \
							memory_layout_t layout; \
							_type data[(_size)]; \
						}* tmp ## __LINE__ = _scl_scope_alloc(sizeof(*tmp ## __LINE__)); \
						tmp ## __LINE__->layout.size = (_size); \
						tmp ## __LINE__->layout.array_elem_size = sizeof(_type); \
						tmp ## __LINE__->layout.flags = MEM_FLAG_ARRAY; \
						(_type*) (tmp ## __LINE__->data); \
					})

#define _scl_static_cstring(_data, _len) ({ \
						static _scl_symbol_hidden struct { \
							memory_layout_t layout; \
							scl_int8 data[(_len) + 1]; \
						} CONCAT(_str_data, __LINE__) __asm__("l_scl_string_data" _scl_macro_to_string(__COUNTER__)) = { \
							.layout = { \
								.array_elem_size = sizeof(scl_int8), \
								.flags = MEM_FLAG_ARRAY, \
								.size = _len, \
							}, \
							.data = (_data), \
						}; \
						_scl_mark_static(&((CONCAT(_str_data, __LINE__)).layout)); \
						(scl_int8*) ((CONCAT(_str_data, __LINE__)).data); \
					})

#define _scl_static_string(_data, _hash, _len) ({ \
						extern const TypeInfo _scl_ti_str __asm("__Tstr"); \
						static _scl_symbol_hidden struct { \
							memory_layout_t layout; \
							scl_int8 data[(_len) + 1]; \
						} CONCAT(_str_data, __LINE__) __asm__("l_scl_string_data" _scl_macro_to_string(__COUNTER__)) = { \
							.layout = { \
								.array_elem_size = sizeof(scl_int8), \
								.flags = MEM_FLAG_ARRAY, \
								.size = _len, \
							}, \
							.data = (_data), \
						}; \
						static _scl_symbol_hidden struct { \
							memory_layout_t layout; \
							struct scale_string data; \
						} CONCAT(_str, __LINE__) __asm__("l_scl_string" _scl_macro_to_string(__COUNTER__)) = { \
							.layout = { \
								.array_elem_size = 0, \
								.flags = MEM_FLAG_INSTANCE, \
								.size = sizeof(struct scale_string), \
							}, \
							.data = { \
								.$type = &_scl_ti_str, \
								.hash = _hash, \
								.data = &((CONCAT(_str_data, __LINE__)).data), \
								.length = _len, \
							}, \
						}; \
						_scl_mark_static(&((CONCAT(_str_data, __LINE__)).layout)); \
						&(((typeof(CONCAT(_str, __LINE__))*) _scl_mark_static(&(CONCAT(_str, __LINE__).layout)))->data); \
					})

scl_any				_scl_realloc(scl_any ptr, scl_int size);
scl_any				_scl_alloc(scl_int size);
void				_scl_free(scl_any ptr);
scl_int				_scl_sizeof(scl_any ptr);

ID_t				type_id(const scl_int8* data);

scl_int				_scl_identity_hash(scl_any obj);
scl_any				_scl_alloc_struct(const TypeInfo* statics);
scl_any				_scl_init_struct(scl_any ptr, const TypeInfo* statics, memory_layout_t* layout);
scl_int				_scl_is_instance(scl_any ptr);
scl_any				_scl_mark_static(memory_layout_t* layout);
scl_int				_scl_is_instance_of(scl_any ptr, ID_t type_id);
scl_any				_scl_get_vtable_function(scl_int onSuper, scl_any instance, const scl_int8* methodIdentifier);
scl_any				_scl_checked_cast(scl_any instance, ID_t target_type, const scl_int8* target_type_name);
scl_int8*			_scl_typename_or_else(scl_any instance, const scl_int8* else_);
scl_any				_scl_cvarargs_to_array(va_list args, scl_int count);
void				_scl_lock(scl_any obj);
void				_scl_unlock(scl_any obj);
scl_any				_scl_copy_fields(scl_any dest, scl_any src, scl_int size);
char*				vstrformat(const char* fmt, va_list args);
char*				strformat(const char* fmt, ...);
void				_scl_assert(scl_int b, const scl_int8* msg, ...);
void				builtinUnreachable(void);
scl_int				builtinIsInstanceOf(scl_any obj, scl_str type);

scl_any				_scl_new_array_by_size(scl_int num_elems, scl_int elem_size);
scl_any				_scl_migrate_foreign_array(const void* const arr, scl_int num_elems, scl_int elem_size);
scl_int				_scl_is_array(scl_any* arr);
scl_any*			_scl_multi_new_array_by_size(scl_int dimensions, scl_int sizes[], scl_int elem_size);
scl_int				_scl_array_size(scl_any* arr);
scl_int				_scl_array_elem_size(scl_any* arr);
void				_scl_array_check_bounds_or_throw(scl_any* arr, scl_int index);
scl_any*			_scl_array_resize(scl_int new_size, scl_any* arr);
scl_str				_scl_array_to_string(scl_any* arr);

// BEGIN C++ Concurrency API wrappers
void				cxx_std_thread_join_and_delete(scl_any thread);
void				cxx_std_thread_detach(scl_any thread);
scl_any				cxx_std_thread_new(void);
scl_any				cxx_std_thread_new_with_args(scl_any args);
scl_any				cxx_async(scl_any func, scl_any args);
scl_any				cxx_await(scl_any t);
void				cxx_std_this_thread_yield(void);

scl_any				cxx_std_recursive_mutex_new(void);
void				cxx_std_recursive_mutex_delete(scl_any* mutex);
void				cxx_std_recursive_mutex_lock(scl_any* mutex);
void				cxx_std_recursive_mutex_unlock(scl_any* mutex);
// END C++ Concurrency API wrappers

#define _scl_scope_alloc(_size)				({ \
												scl_any tmp = &_local_scope_buffer[_local_scope_buffer_ptr]; \
												_local_scope_buffer_ptr += _size; \
												tmp; \
											})
#define _scl_scope(_size)					char _local_scope_buffer[(_size)]; scl_int _local_scope_buffer_ptr __attribute__((cleanup(_scl_reset_local_buffer))) = 0

static inline void _scl_reset_local_buffer(scl_int* ptr) {
	*ptr = 0;
}

#define _scl_push(_type, _value)			(*(_type*) _local_stack_ptr = (_value), _local_stack_ptr++)
#define _scl_push_value(_type, _flags, _value) ({ \
												struct { \
													memory_layout_t layout; \
													_type data; \
												}* tmp ## __LINE__ = _scl_scope_alloc(sizeof(*tmp ## __LINE__)); \
												tmp ## __LINE__->data = (_value); \
												tmp ## __LINE__->layout.size = sizeof(_type); \
												tmp ## __LINE__->layout.flags = (_flags); \
												_scl_push(_type*, &tmp ## __LINE__->data); \
											})
#define _scl_pop(_type)						(_local_stack_ptr--, *(_type*) _local_stack_ptr)
#define _scl_positive_offset(offset, _type)	(*(_type*) (_local_stack_ptr + offset))
#define _scl_top(_type)						(*(_type*) (_local_stack_ptr - 1))
#define _scl_cast_stack(_to, _from)			(_scl_top(_to) = (_to) _scl_top(_from))
#define _scl_popn(n)						(_local_stack_ptr -= (n))
#define _scl_dup()							_scl_push(scl_any, _scl_top(scl_any))
#define _scl_drop()							(--_local_stack_ptr)
#define _scl_cast_positive_offset(offset, _type, _other) \
											((_other) (*(_type*) (_local_stack_ptr + offset)))

#define _scl_swap() ({ \
		scl_int tmp ## __LINE__ = _local_stack_ptr[-1]; \
		_local_stack_ptr[-1] = _local_stack_ptr[-2]; \
		_local_stack_ptr[-2] = tmp ## __LINE__; \
	})

#define _scl_over() ({ \
		scl_int tmp ## __LINE__ = _local_stack_ptr[-1]; \
		_local_stack_ptr[-1] = _local_stack_ptr[-3]; \
		_local_stack_ptr[-3] = tmp ## __LINE__; \
	})

#define _scl_sdup2() ({ \
		_local_stack_ptr[0] = _local_stack_ptr[-2]; \
	})

#define _scl_swap2() ({ \
		scl_int tmp ## __LINE__ = _local_stack_ptr[-2]; \
		_local_stack_ptr[-2] = _local_stack_ptr[-3]; \
		_local_stack_ptr[-3] = tmp ## __LINE__; \
	})

#define _scl_rot() ({ \
		scl_int tmp ## __LINE__ = _local_stack_ptr[-3]; \
		_local_stack_ptr[-3] = _local_stack_ptr[-2]; \
		_local_stack_ptr[-2] = _local_stack_ptr[-1]; \
		_local_stack_ptr[-1] = tmp ## __LINE__; \
	})

#define _scl_unrot() ({ \
		scl_int tmp ## __LINE__ = _local_stack_ptr[-1]; \
		_local_stack_ptr[-1] = _local_stack_ptr[-2]; \
		_local_stack_ptr[-2] = _local_stack_ptr[-3]; \
		_local_stack_ptr[-3] = tmp ## __LINE__; \
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
#define _scl_asr(a, b)			(a) >> (b)
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
#define _scl_inc(a)				((a) + 1)
#define _scl_dec(a)				((a) - 1)
#define _scl_ann(a)				({ typeof((a)) _a = (a); _scl_assert(_a, "Expected non-nil value"); _a; })
#define _scl_elvis(a, b)		({ typeof((a)) _a = (a); _a ? _a : (b); })
#define _scl_checked_index(a, i) \
								({ typeof((a)) _a = (a); typeof((i)) _i = (i); _scl_array_check_bounds_or_throw((scl_any*) _a, _i); _a[_i]; })
#define _scl_checked_write(a, i, w) \
								({ typeof((a)) _a = (a); typeof((i)) _i = (i); _scl_array_check_bounds_or_throw((scl_any*) _a, _i); _a[_i] = (w); })
#define _scl_putlocal(a, w) 	((a) = (w))

static inline scl_uint8 _scl_ror8(scl_uint8 a, scl_int b) { return (a >> b) | (a << ((sizeof(scl_uint8) << 3) - b)); }
static inline scl_uint16 _scl_ror16(scl_uint16 a, scl_int b) { return (a >> b) | (a << ((sizeof(scl_uint16) << 3) - b)); }
static inline scl_uint32 _scl_ror32(scl_uint32 a, scl_int b) { return (a >> b) | (a << ((sizeof(scl_uint32) << 3) - b)); }
static inline scl_uint64 _scl_ror64(scl_uint64 a, scl_int b) { return (a >> b) | (a << ((sizeof(scl_uint64) << 3) - b)); }

static inline scl_uint8 _scl_rol8(scl_uint8 a, scl_int b) { return (a << b) | (a >> ((sizeof(scl_uint8) << 3) - b)); }
static inline scl_uint16 _scl_rol16(scl_uint16 a, scl_int b) { return (a << b) | (a >> ((sizeof(scl_uint16) << 3) - b)); }
static inline scl_uint32 _scl_rol32(scl_uint32 a, scl_int b) { return (a << b) | (a >> ((sizeof(scl_uint32) << 3) - b)); }
static inline scl_uint64 _scl_rol64(scl_uint64 a, scl_int b) { return (a << b) | (a >> ((sizeof(scl_uint64) << 3) - b)); }

#define _scl_ror(a, b)	(_Generic((a), \
		scl_int8: _scl_ror8, scl_uint8: _scl_ror8, \
		scl_int16: _scl_ror16, scl_uint16: _scl_ror16, \
		scl_int32: _scl_ror32, scl_uint32: _scl_ror32, \
		scl_int64: _scl_ror64, scl_uint64: _scl_ror64, \
		scl_int: _scl_ror64, scl_uint: _scl_ror64 \
	))((a), (b))
#define _scl_rol(a, b)	(_Generic((a), \
		scl_int8: _scl_ror8, scl_uint8: _scl_ror8, \
		scl_int16: _scl_ror16, scl_uint16: _scl_ror16, \
		scl_int32: _scl_ror32, scl_uint32: _scl_ror32, \
		scl_int64: _scl_ror64, scl_uint64: _scl_ror64, \
		scl_int: _scl_ror64, scl_uint: _scl_ror64 \
	))((a), (b))

#define likely(x) _scl_expect(!!(x), 1)
#define unlikely(x) _scl_expect(!!(x), 0)

#if defined(__cplusplus)
}
#endif

#endif // __SCALE_RUNTIME_H__
