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
#include <string.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <setjmp.h>

#define GC_THREADS
#ifdef _WIN32
#define GC_WIN32_THREADS
#else
#define GC_PTHREADS
#endif
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
#define scale_no_return __attribute__((__noreturn__))
#else
#define scale_no_return
#endif

#if __has_attribute(constructor)
#define scale_constructor __attribute__((constructor))
#else
#error "Scale requires a compiler with __attribute__((constructor)) support"
#endif

#if __has_attribute(destructor)
#define scale_destructor __attribute__((destructor))
#else
#error "Scale requires a compiler with __attribute__((destructor)) support"
#endif

#if __GNUC__ >= 4
#define scale_symbol_hidden __attribute__((visibility("hidden")))
#else
#define scale_symbol_hidden
#endif

#if __has_attribute(noinline)
#define scale_noinline __attribute__((noinline))
#else
#define scale_noinline
#endif

#define nullable __nullable
#define nonnull  __nonnull
#define tls __thread

#define scale_macro_to_string_(x) # x
#define scale_macro_to_string(x) scale_macro_to_string_(x)

// Scale expects this function
#define expect

#undef nil
#define nil NULL

#pragma clang 

// Creates a new string from a C string
// The given C string is not copied
#define str_of_exact		scale_create_string

#define cstr_of(_str)		((_str)->data)

#if __has_builtin(__builtin_expect)
#define scale_expect(expr, c) __builtin_expect((expr), (c))
#else
#define scale_expect(expr, c) (expr)
#endif

#if __has_attribute(always_inline)
#define scale_always_inline __attribute__((always_inline))
#else
#define scale_always_inline
#endif

#define AKA(_sym)				asm(scale_macro_to_string(__USER_LABEL_PREFIX__) # _sym)

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
#define SCALE_KNOWN_LITTLE_ENDIAN
#endif
#endif

#if __SIZEOF_POINTER__ < 8
// WASM or 32-bit system
#if defined(__wasm__)
#define SCALE_SYSTEM "WASM32"
#define SCALE_WASM32 1
#elif defined(__arm__)
#define SCALE_SYSTEM "aarch32"
#define SCALE_ARM32  1
#elif defined(__i386__)
#define SCALE_SYSTEM "x86"
#define SCALE_X86    1
#else
#define SCALE_SYSTEM "unknown 32-bit"
#define SCALE_UNKNOWN_ARCH 1
#endif
#else
// 64-bit system
#if defined(__wasm__)
#define SCALE_SYSTEM	"WASM64"
#define SCALE_WASM64	1
#define SCALE_WASM32	1
#elif defined(__aarch64__)
#define SCALE_SYSTEM	"aarch64"
#define SCALE_AARCH64	1
#define SCALE_ARM64		1
#define SCALE_ARM32		1
#elif defined(__x86_64__)
#define SCALE_SYSTEM	"x86_64"
#define SCALE_X64		1
#define SCALE_X86_64	1
#define SCALE_X86		1
#else
#define SCALE_SYSTEM	"unknown 64-bit"
#define SCALE_UNKNOWN_ARCH 1
#endif
#endif

typedef void*				scale_any;

#if __SIZEOF_LONG__ == 8
typedef long				scale_int;
typedef unsigned long		scale_uint;
#define SCALE_INT_FMT		"%ld"
#define SCALE_UINT_FMT		"%lu"
#define SCALE_INT_HEX_FMT	"%lx"
#define SCALE_PTR_HEX_FMT	"0x%016lx"
#elif __SIZEOF_LONG_LONG__ == 8
typedef long long			scale_int;
typedef unsigned long long	scale_uint;
#define SCALE_INT_FMT		"%lld"
#define SCALE_UINT_FMT		"%llu"
#define SCALE_INT_HEX_FMT	"%llx"
#define SCALE_PTR_HEX_FMT	"0x%016llx"
#else
#error "Can't find a 64-bit integer type"
#endif

#define SCALE_int_MAX		 (1L << (sizeof(scale_int) * 8 - 1))
#define SCALE_int_MASK		((scale_int) -1)
#define SCALE_int_MIN		((1L << (sizeof(scale_int) * 8 - 1)) - 1)
#define SCALE_uint_MAX		((scale_uint) -1)
#define SCALE_uint_MASK		((scale_uint) -1)
#define SCALE_uint_MIN		((scale_uint) 0)

typedef float				scale_float32;
#if __SIZEOF_DOUBLE__ == 8
typedef double 				scale_float;
#elif __SIZEOF_LONG_DOUBLE__ == 8
typedef long double			scale_float;
#elif __SIZEOF_FLOAT__ == 8
typedef float				scale_float;
#else
#error "Can't find a 64-bit floating point type"
#endif

typedef scale_int			scale_bool;

typedef struct Struct_str*	scale_str;

#define SCALE_int64_MAX		((scale_int64) ((scale_uint) (1LL << (sizeof(scale_int64) * 8 - 1))) - 1ULL)
#define SCALE_int64_MASK	((scale_int64) -1)
#define SCALE_int64_MIN		((scale_int64) (1LL << (sizeof(scale_int64) * 8 - 1)))
#define SCALE_int32_MAX		((scale_int32) ((scale_uint) (1 << (sizeof(scale_int32) * 8 - 1))) - 1ULL)
#define SCALE_int32_MASK	((scale_int32) -1)
#define SCALE_int32_MIN		((scale_int32) (1 << (sizeof(scale_int32) * 8 - 1)))
#define SCALE_int16_MAX		((scale_int16) ((scale_uint) (1 << (sizeof(scale_int16) * 8 - 1))) - 1ULL)
#define SCALE_int16_MASK	((scale_int16) -1)
#define SCALE_int16_MIN		((scale_int16) (1 << (sizeof(scale_int16) * 8 - 1)))
#define SCALE_int8_MAX		((scale_int8) ((scale_uint) (1 << (sizeof(scale_int8) * 8 - 1))) - 1ULL)
#define SCALE_int8_MASK		((scale_int8) -1)
#define SCALE_int8_MIN		((scale_int8) (1 << (sizeof(scale_int8) * 8 - 1)))
#define SCALE_uint64_MAX	((scale_uint64) -1)
#define SCALE_uint64_MASK	((scale_uint64) -1)
#define SCALE_uint64_MIN	((scale_uint64) 0)
#define SCALE_uint32_MAX	((scale_uint32) -1)
#define SCALE_uint32_MASK	((scale_uint32) -1)
#define SCALE_uint32_MIN	((scale_uint32) 0)
#define SCALE_uint16_MAX	((scale_uint16) -1)
#define SCALE_uint16_MASK	((scale_uint16) -1)
#define SCALE_uint16_MIN	((scale_uint16) 0)
#define SCALE_uint8_MAX		((scale_uint8) -1)
#define SCALE_uint8_MASK	((scale_uint8) -1)
#define SCALE_uint8_MIN		((scale_uint8) 0)

typedef long long			scale_int64;
typedef int					scale_int32;
typedef short				scale_int16;
typedef char				scale_int8;
typedef unsigned long long	scale_uint64;
typedef unsigned int		scale_uint32;
typedef unsigned short		scale_uint16;
typedef unsigned char		scale_uint8;

typedef void(*scale_function)(void);
typedef void(*(*scale_lambda))(void*);

typedef scale_uint ID_t;

#if defined(__ANDROID__)
#define SCALE_OS_NAME "Android"
#elif defined(__amigaos__)
#define SCALE_OS_NAME "AmigaOS"
#elif defined(__bsdi__)
#define SCALE_OS_NAME "BSD-like"
#elif defined(__CYGWIN__)
#define SCALE_OS_NAME "Windows (Cygwin)"
#elif defined(__DragonFly__)
#define SCALE_OS_NAME "DragonFly"
#elif defined(__FreeBSD__)
#define SCALE_OS_NAME "FreeBSD"
#elif defined(__gnu_linux__)
#define SCALE_OS_NAME "GNU/Linux"
#elif defined(__GNU__)
#define SCALE_OS_NAME "GNU"
#elif defined(macintosh)
#define SCALE_OS_NAME "Classic Mac OS"
#elif defined(__APPLE__) && defined(__MACH__)
#define SCALE_OS_NAME "macOS"
#elif defined(__APPLE__) && defined(__IOS__)
#define SCALE_OS_NAME "iOS"
#elif defined(__APPLE__) && defined(__TVOS__)
#define SCALE_OS_NAME "tvOS"
#elif defined(__APPLE__) && defined(__WATCHOS__)
#define SCALE_OS_NAME "watchOS"
#elif defined(__minix)
#define SCALE_OS_NAME "MINIX"
#elif defined(__MSDOS__)
#define SCALE_OS_NAME "MS-DOS"
#elif defined(__NetBSD__)
#define SCALE_OS_NAME "NetBSD"
#elif defined(__OpenBSD__)
#define SCALE_OS_NAME "OpenBSD"
#elif defined(__OS2__)
#define SCALE_OS_NAME "IBM OS/2"
#elif defined(__unix__)
#define SCALE_OS_NAME "UNIX"
#elif defined(_WIN32) || defined(_WIN32_WCE)
#define SCALE_OS_NAME "Windows"
#else
#define SCALE_OS_NAME "Unknown OS"
#endif

// Create a new instance of a struct
#define ALLOC(_type)	({ \
	extern const TypeInfo $I ## _type; \
	(scale_ ## _type) scale_alloc_struct(&$I ## _type); \
})

// Try to cast the given instance to the given type
#define CAST0(_obj, _type, _type_hash) ((scale_ ## _type) scale_checked_cast(_obj, _type_hash, #_type))
// Try to cast the given instance to the given type
#define CAST(_obj, _type) CAST0(_obj, _type, type_id(#_type))

struct scale_exception_handler {
	scale_uint marker;
	scale_any exception;
	jmp_buf jmp;
	scale_any finalization_data;
	void (*finalizer)(scale_any);
};

struct scale_backtrace {
	scale_uint marker;
	const scale_int8* func_name;
};

#if __has_attribute(packed)
#define scale_packed __attribute__((packed))
#else
#define scale_packed
#endif

typedef struct {
	scale_int32 size scale_packed;
	scale_uint8 flags scale_packed;
	scale_int32 array_elem_size:24 scale_packed;
} memory_layout_t;

#define MEM_FLAG_INSTANCE	0x01
#define MEM_FLAG_ARRAY		0x02
#define MEM_FLAG_STATIC		0x04

struct scale_methodinfo {
	const ID_t							pure_name;
	const ID_t							signature;
};

typedef struct {
	memory_layout_t						layout;
	scale_function						funcs[];
} scale_vtable;

typedef struct {
	memory_layout_t						layout;
	struct scale_methodinfo				infos[];
} scale_methodinfo_t;

typedef struct TypeInfo {
	const ID_t							type;
	const struct TypeInfo*				super;
	const scale_int8*						type_name;
	const size_t						size;
	const struct scale_methodinfo* const	vtable_info;
	const scale_function*				vtable;
} TypeInfo;

struct Struct_str {
	const TypeInfo*						$type;
	scale_int8*							data;
	scale_int								length;
	scale_uint							hash;
};

#define CONCAT(a, b) CONCAT_(a, b)
#define CONCAT_(a, b) a ## b

#define likely(x) scale_expect(!!(x), 1)
#define unlikely(x) scale_expect(!!(x), 0)

// Get the offset of a member in a struct
#define scale_offsetof(type, member) ((scale_int)&((type*)0)->member)

#define REINTERPRET_CAST(_type, _value) ({ \
	union { \
		typeof(_value) a; \
		_type b; \
	} _tmp = {0}; \
	_tmp.a = _value; \
	_tmp.b; \
})

#define scale_async(x, structbody, ...) ({ \
	scale_any func = (x); \
	struct structbody tmp = { __VA_ARGS__ }; \
	typeof(tmp)* args = malloc(sizeof(tmp)); \
	memcpy(args, &tmp, sizeof(tmp)); \
	scale_push(scale_any, scale_run_async(func, args)); \
})
#define scale_sync(rtype, x, structbody, ...) ({ \
	rtype(*func)(scale_any) = (typeof(func)) (x); \
	struct structbody tmp = { __VA_ARGS__ }; \
	scale_push(rtype, func(&tmp)); \
})
#define scale_sync_v(x, structbody, ...) ({ \
	void(*func)(scale_any) = (typeof(func)) (x); \
	struct structbody tmp = { __VA_ARGS__ }; \
	func(&tmp); \
})
#define scale_await(rtype) (scale_top(rtype) = scale_run_await(scale_top(scale_any)))
#define scale_await_void() scale_run_await(scale_pop(scale_any))

#define SYMBOL(name) __asm__(scale_macro_to_string(__USER_LABEL_PREFIX__) name)

#define 			EXCEPTION_HANDLER_MARKER \
					0xF0E1D2C3B4A59687ULL

#define 			TRACE_MARKER \
					0xF7E8D9C0B1A28384ULL

#define 			TRY \
						struct scale_exception_handler scale_exception_handler; \
						scale_exception_handler.marker = EXCEPTION_HANDLER_MARKER; \
						scale_exception_handler.finalizer = nil; \
						if (setjmp(scale_exception_handler.jmp) != 666)
#define 			TRY_FINALLY(_then, _with) \
						struct scale_exception_handler scale_exception_handler; \
						scale_exception_handler.marker = EXCEPTION_HANDLER_MARKER; \
						scale_exception_handler.finalizer = _then; \
						scale_exception_handler.finalization_data = _with; \
						if (setjmp(scale_exception_handler.jmp) != 666)

#ifndef SCALE_EMBEDDED
#define				SCALE_BACKTRACE(_func_name) \
						struct scale_backtrace _scale_backtrace_cur __attribute__((cleanup(scale_trace_remove))) = { .marker = TRACE_MARKER, .func_name = (_func_name) }
#else
#define				SCALE_BACKTRACE(_func_name)
#endif

void				scale_trace_remove(struct scale_backtrace*);

#include "preproc.h"

// call a method on an instance
#define				virtual_call(instance, methodIdentifier, rtype, ...) \
						({ \
							typeof((instance)) _tmp = (instance); \
							((rtype(*)(typeof((instance)) __VA_OPT__(, _SCALE_TYPES(__VA_ARGS__)))) scale_get_vtable_function(_tmp, (methodIdentifier)))(_tmp, ##__VA_ARGS__); \
						})

// call a lambda
#define call_lambda(lambda, rtype, ...) ({ \
	rtype (*(*CONCAT(tmp, __LINE__)))(scale_any __VA_OPT__(, _SCALE_TYPES(__VA_ARGS__))) = (typeof(CONCAT(tmp, __LINE__))) (lambda); \
	(*CONCAT(tmp, __LINE__))(CONCAT(tmp, __LINE__) __VA_OPT__(,) __VA_ARGS__); \
})

scale_no_return void	scale_runtime_error(int code, const scale_int8* msg, ...);
scale_no_return void	scale_runtime_catch(scale_any ex);
scale_no_return void	scale_throw(scale_any ex);

scale_constructor
void				scale_setup(void);

#define scale_create_string(_data) ({ \
						const scale_int8* data = (_data); \
						scale_str str = ALLOC(str); \
						str->length = strlen(data); \
						str->data = (scale_int8*) scale_migrate_foreign_array(data, strlen(data) + 1, sizeof(scale_int8)); \
						str->hash = type_id(data); \
						str; \
					})

#define scale_uninitialized_constant_ctype(_type) ({ \
						static scale_symbol_hidden struct { \
							memory_layout_t layout; \
							_type data; \
						} _constant __asm__("lscale_const" scale_macro_to_string(__COUNTER__)) = { \
							.layout = { \
								.array_elem_size = 0, \
								.flags = MEM_FLAG_INSTANCE, \
								.size = sizeof(_type), \
							}, \
							.data = {}, \
						}; \
						&((typeof(_constant)*) scale_mark_static(&(_constant.layout)))->data; \
					})

#define scale_uninitialized_constant(_type) ({ \
						extern const TypeInfo $I ## _type; \
						scale_ ## _type _t; \
						static scale_symbol_hidden struct { \
							memory_layout_t layout; \
							typeof(*_t) data; \
						} _constant __asm__("lscale_const" scale_macro_to_string(__COUNTER__)) = { \
							.layout = { \
								.array_elem_size = 0, \
								.flags = MEM_FLAG_INSTANCE, \
								.size = sizeof(*_t), \
							}, \
							.data = { \
								.$type = &$I ## _type, \
							}, \
						}; \
						&((typeof(_constant)*) scale_mark_static(&(_constant.layout)))->data; \
					})

#define scale_stack_alloc(_type) ( \
						&((struct { \
							memory_layout_t layout; \
							struct Struct_ ## _type data; \
						}) { \
							.layout = { \
								.size = sizeof(struct Struct_ ## _type), \
								.flags = MEM_FLAG_INSTANCE, \
							}, \
							.data = { \
								.$type = ({ \
									extern const TypeInfo $I ## _type; \
									&$I ## _type; \
								}), \
							} \
						}).data \
					)

#define scale_stack_alloc_ctype(_type) ( \
						&((struct { \
							memory_layout_t layout; \
							_type data; \
						}) { \
							.layout = { \
								.size = sizeof(_type), \
							} \
						}).data \
					)

#define scale_stack_alloc_array(_type, _size) ( \
						((struct { \
							memory_layout_t layout; \
							_type data[(_size)]; \
						}) = { \
							.layout = { \
								.size = (_size); \
								.array_elem_size = sizeof(_type); \
								.flags = MEM_FLAG_ARRAY; \
							} \
						}).data \
					)

#define scale_static_cstring(_data) ({ \
						static scale_symbol_hidden struct { \
							memory_layout_t layout; \
							scale_int8 data[sizeof((_data))]; \
						} str_data __asm__("lscale_cstr" scale_macro_to_string(__COUNTER__)) = { \
							.layout = { \
								.array_elem_size = sizeof(scale_int8), \
								.flags = MEM_FLAG_ARRAY, \
								.size = sizeof((_data)) - 1, \
							}, \
							.data = (_data), \
						}; \
						scale_mark_static(&(str_data.layout)); \
						(scale_int8*) ((str_data).data); \
					})

#define scale_static_string(_data, _hash) ({ \
						extern const TypeInfo $Istr; \
						static scale_symbol_hidden struct { \
							memory_layout_t layout; \
							scale_int8 data[sizeof((_data))]; \
						} str_data __asm__("lscale_cstr" scale_macro_to_string(__COUNTER__)) = { \
							.layout = { \
								.array_elem_size = sizeof(scale_int8), \
								.flags = MEM_FLAG_ARRAY, \
								.size = sizeof((_data)) - 1, \
							}, \
							.data = (_data), \
						}; \
						static scale_symbol_hidden struct { \
							memory_layout_t layout; \
							struct Struct_str data; \
						} str __asm__("lscale_str" scale_macro_to_string(__COUNTER__)) = { \
							.layout = { \
								.size = sizeof(struct Struct_str), \
								.flags = MEM_FLAG_INSTANCE, \
							}, \
							.data = { \
								.$type = &$Istr, \
								.data = (str_data.data), \
								.length = sizeof((_data)) - 1, \
								.hash = _hash, \
							}, \
						}; \
						scale_mark_static(&(str_data.layout)); \
						&(((typeof(str)*) scale_mark_static(&(str.layout)))->data); \
					})

// TODO: More macro magic: make this macro auto convert all args to safe values
#define varargs(...)				_SCALE_PREPROC_NARG(__VA_ARGS__), _SCALE_VARARGS_SAFE(__VA_ARGS__)
#define scale_varargs				scale_int $count, ...
#define scale_varargs_safe(val)		REINTERPRET_CAST(scale_uint64, val)

#if __has_attribute(warn_unused_result)
#define scale_nodiscard __attribute__((warn_unused_result))
#else
#define scale_nodiscard
#endif

scale_nodiscard
scale_any				scale_realloc(scale_any ptr, scale_int size);
scale_nodiscard
scale_any				scale_alloc(scale_int size);
void					scale_free(scale_any ptr);
scale_int				scale_sizeof(scale_any ptr);
scale_nodiscard
scale_any				scale_alloc_struct(const TypeInfo* statics);
scale_nodiscard
scale_any				scale_init_struct(scale_any ptr, const TypeInfo* statics, memory_layout_t* layout);

ID_t					type_id(const scale_int8* data);

scale_int				scale_identity_hash(scale_any obj);
scale_int				scale_is_instance(scale_any ptr);
#ifndef SCALE_EMBEDDED
scale_any				scale_mark_static(memory_layout_t* layout);
#endif
scale_int				scale_is_instance_of(scale_any ptr, ID_t type_id);
scale_any				scale_get_vtable_function(scale_any instance, const scale_int8* methodIdentifier);
scale_any				scale_checked_cast(scale_any instance, ID_t target_type, const scale_int8* target_type_name);
scale_int8*				scale_typename_or_else(scale_any instance, const scale_int8* else_);
ID_t					scale_typeid_or_else(scale_any instance, ID_t else_);
scale_any				scale_cvarargs_to_array(va_list args, scale_int count);
scale_any				scale_copy_fields(scale_any dest, scale_any src, scale_int size);
char*					vstrformat(const char* fmt, va_list args);
char*					strformat(const char* fmt, ...);
void					builtinUnreachable(void);
scale_int				builtinIsInstanceOf(scale_any obj, scale_str type);
scale_int8*				scale_get_thread_name(void);
void					scale_set_thread_name(scale_int8* name);

scale_any				scale_new_array_by_size(scale_int num_elems, scale_int elem_size);
scale_any				scale_migrate_foreign_array(const void* const arr, scale_int num_elems, scale_int elem_size);
scale_int				scale_is_array(scale_any* arr);
scale_any*				scale_multi_new_array_by_size(scale_int dimensions, scale_int sizes[], scale_int elem_size);
scale_int				scale_array_size(scale_any* arr);
scale_int				scale_array_elem_size(scale_any* arr);
void					scale_array_check_bounds_or_throw(scale_any* arr, scale_int index);
scale_any*				scale_array_resize(scale_int new_size, scale_any* arr);
scale_str				scale_array_to_string(scale_any* arr);

scale_any				scale_thread_start(scale_any func, scale_any args);
void					scale_thread_finish(scale_any thread);
void					scale_thread_detach(scale_any thread);
scale_any				scale_run_async(scale_any func, scale_any func_args);
scale_any				scale_run_sync(scale_any func, scale_any func_args);
scale_any				scale_run_await(scale_any _args);
void					scale_yield();

// BEGIN C++ Concurrency API wrappers
scale_any				cxx_std_recursive_mutex_new(void);
void					cxx_std_recursive_mutex_delete(scale_any* mutex);
void					cxx_std_recursive_mutex_lock(scale_any* mutex);
void					cxx_std_recursive_mutex_unlock(scale_any* mutex);
// END C++ Concurrency API wrappers

static inline scale_always_inline void scale_reset_local_buffer(scale_int* ptr) {
	*ptr = 0;
}

#define scale_push(_type, _value)			((*(_type*) _local_stack_ptr = (_value)), _local_stack_ptr++)
#define scale_push_value(_type, _flags, _value) scale_push(_type*, \
												&((struct { \
													memory_layout_t layout; \
													_type data; \
												}) { \
													.layout = { \
														.size = sizeof(_type), \
														.flags = (_flags), \
													}, \
													.data = (_value) \
												}).data \
											)

#define scale_pop(_type)						(_local_stack_ptr--, *(_type*) _local_stack_ptr)
#define scale_positive_offset(offset, _type)	(*(_type*) (_local_stack_ptr + offset))
#define scale_top(_type)						(*(_type*) (_local_stack_ptr - 1))
#define scale_cast_stack(_to, _from)			(scale_top(_to) = (_to) scale_top(_from))
#define scale_popn(n)							(_local_stack_ptr -= (n))
#define scale_dup()								scale_push(scale_any, scale_top(scale_any))
#define scale_drop()							(--_local_stack_ptr)
#define scale_cast_positive_offset(offset, _type, _other) \
												((_other) (*(_type*) (_local_stack_ptr + offset)))

#define scale_swap() ({ \
		scale_int tmp = _local_stack_ptr[-1]; \
		_local_stack_ptr[-1] = _local_stack_ptr[-2]; \
		_local_stack_ptr[-2] = tmp; \
	})

#define scale_over() ({ \
		scale_int tmp = _local_stack_ptr[-1]; \
		_local_stack_ptr[-1] = _local_stack_ptr[-3]; \
		_local_stack_ptr[-3] = tmp; \
	})

#define scale_sdup2() ({ \
		_local_stack_ptr[0] = _local_stack_ptr[-2]; \
	})

#define scale_swap2() ({ \
		scale_int tmp = _local_stack_ptr[-2]; \
		_local_stack_ptr[-2] = _local_stack_ptr[-3]; \
		_local_stack_ptr[-3] = tmp; \
	})

#define scale_rot() ({ \
		scale_int tmp = _local_stack_ptr[-3]; \
		_local_stack_ptr[-3] = _local_stack_ptr[-2]; \
		_local_stack_ptr[-2] = _local_stack_ptr[-1]; \
		_local_stack_ptr[-1] = tmp; \
	})

#define scale_unrot() ({ \
		scale_int tmp = _local_stack_ptr[-1]; \
		_local_stack_ptr[-1] = _local_stack_ptr[-2]; \
		_local_stack_ptr[-2] = _local_stack_ptr[-3]; \
		_local_stack_ptr[-3] = tmp; \
	})

#define scale_add(a, b)					(a) + (b)
#define scale_sub(a, b)					(a) - (b)
#define scale_mul(a, b)					(a) * (b)
#define scale_div(a, b)					(a) / (b)
#define scale_pow(a, b)					pow((a), (b))
#define scale_mod(a, b)					(a) % (b)
#define scale_land(a, b)				(a) & (b)
#define scale_lor(a, b)					(a) | (b)
#define scale_lxor(a, b)				(a) ^ (b)
#define scale_lnot(a)					~((a))
#define scale_lsr(a, b)					(a) >> (b)
#define scale_asr(a, b)					(a) >> (b)
#define scale_lsl(a, b)					(a) << (b)
#define scale_eq(a, b)					(a) == (b)
#define scale_ne(a, b)					(a) != (b)
#define scale_gt(a, b)					(a) > (b)
#define scale_ge(a, b)					(a) >= (b)
#define scale_lt(a, b)					(a) < (b)
#define scale_le(a, b)					(a) <= (b)
#define scale_and(a, b)					(a) && (b)
#define scale_or(a, b)					(a) || (b)
#define scale_not(a)					!(a)
#define scale_at(a)						(*((a)))
#define scale_inc(a)					((a) + 1)
#define scale_dec(a)					((a) - 1)
#define scale_ann(a)					({ __auto_type _a = (a); scale_assert_fast(_a, "Expected non-nil value"); _a; })
#define scale_elvis(a, b)				({ __auto_type _a = (a); _a ? _a : (b); })
#if !defined(UNSAFE_ARRAY_ACCESS) && !defined(SCALE_EMBEDDED)
#define scale_checked_index(a, i)		({ __auto_type _a = (a); __auto_type _i = (i); scale_array_check_bounds_or_throw((scale_any*) _a, _i); _a[_i]; })
#define scale_checked_write(a, i, w)	({ __auto_type _a = (a); __auto_type _i = (i); scale_array_check_bounds_or_throw((scale_any*) _a, _i); scale_putlocal(_a[_i], (w)); })
#else
#define scale_checked_index(a, i)		((a)[(i)])
#define scale_checked_write(a, i, w)	scale_putlocal((a)[(i)], (w))
#endif
#define scale_putlocal(a, w) 			((a) = (w))

static inline scale_uint8 scale_ror8(scale_uint8 a, scale_int b) { return (a >> b) | (a << ((sizeof(scale_uint8) << 3) - b)); }
static inline scale_uint16 scale_ror16(scale_uint16 a, scale_int b) { return (a >> b) | (a << ((sizeof(scale_uint16) << 3) - b)); }
static inline scale_uint32 scale_ror32(scale_uint32 a, scale_int b) { return (a >> b) | (a << ((sizeof(scale_uint32) << 3) - b)); }
static inline scale_uint64 scale_ror64(scale_uint64 a, scale_int b) { return (a >> b) | (a << ((sizeof(scale_uint64) << 3) - b)); }

static inline scale_uint8 scale_rol8(scale_uint8 a, scale_int b) { return (a << b) | (a >> ((sizeof(scale_uint8) << 3) - b)); }
static inline scale_uint16 scale_rol16(scale_uint16 a, scale_int b) { return (a << b) | (a >> ((sizeof(scale_uint16) << 3) - b)); }
static inline scale_uint32 scale_rol32(scale_uint32 a, scale_int b) { return (a << b) | (a >> ((sizeof(scale_uint32) << 3) - b)); }
static inline scale_uint64 scale_rol64(scale_uint64 a, scale_int b) { return (a << b) | (a >> ((sizeof(scale_uint64) << 3) - b)); }

#define scale_ror(a, b)	(_Generic((a), \
		scale_int8: scale_ror8, scale_uint8: scale_ror8, \
		scale_int16: scale_ror16, scale_uint16: scale_ror16, \
		scale_int32: scale_ror32, scale_uint32: scale_ror32, \
		scale_int64: scale_ror64, scale_uint64: scale_ror64, \
		scale_int: scale_ror64, scale_uint: scale_ror64 \
	))((a), (b))
#define scale_rol(a, b)	(_Generic((a), \
		scale_int8: scale_ror8, scale_uint8: scale_ror8, \
		scale_int16: scale_ror16, scale_uint16: scale_ror16, \
		scale_int32: scale_ror32, scale_uint32: scale_ror32, \
		scale_int64: scale_ror64, scale_uint64: scale_ror64, \
		scale_int: scale_ror64, scale_uint: scale_ror64 \
	))((a), (b))

#ifdef SCALE_EMBEDDED
static inline scale_any scale_mark_static(scale_any x) { return x; }

scale_constructor
void scale_setup(void);

#define scale_assert_fast scale_assert
static inline scale_int scale_assert(scale_int b, const scale_int8* msg, ...) {
	if (unlikely(!b)) {
		va_list va;
		va_start(va, msg);
		vfprintf(stderr, msg, va);
		va_end(va);
		abort();
	}
	return b;
}
#else
static inline scale_int scale_assert_fast(scale_int b, const scale_int8* msg) {
	if (unlikely(!b)) {
		extern const TypeInfo $IAssertError;
		scale_any e = scale_alloc_struct(&$IAssertError);
		scale_str x = str_of_exact(msg);
		void AssertError$init(scale_any, scale_str) __asm__(scale_macro_to_string(__USER_LABEL_PREFIX__) "_M5Error4initEv");
		AssertError$init(e, x);
		scale_throw(e);
	}
	return b;
}
static inline scale_int scale_assert(scale_int b, const scale_int8* msg, ...) {
	if (unlikely(!b)) {
		va_list va;
		va_start(va, msg);
		char* m = vstrformat(msg, va);
		va_end(va);
		char* x = strformat("Assertion failed: %s", m);
		scale_assert_fast(0, x);
	}
	return b;
}
#endif

#if defined(__cplusplus)
}
#endif

#endif // __SCALE_RUNTIME_H__
