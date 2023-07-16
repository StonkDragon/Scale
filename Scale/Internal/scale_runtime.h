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

#define system_allocate(size)				malloc(size)
#define system_free(_ptr)					free(_ptr)
#define system_realloc(_ptr, size)			realloc(_ptr, size)

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
typedef double 				scl_float;
typedef pthread_mutex_t*	mutex_t;

typedef struct scale_string* scl_str;

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

typedef void(*_scl_lambda)(void);

typedef unsigned int ID_t;

struct _scl_methodinfo {
	const _scl_lambda					ptr;
	const ID_t							pure_name;
	const ID_t							signature;
};

typedef struct StaticMembers {
	const struct _scl_methodinfo* const	vtable;
	const struct _scl_methodinfo* const	super_vtable;
	const ID_t							type;
	const ID_t*							supers;
	const scl_int8*						type_name;
} StaticMembers;

struct scale_string {
	const StaticMembers* const			$statics;
	const mutex_t						$mutex;
	scl_int8*							_data;
	scl_int								_len;
	scl_int								_hash;
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

typedef union {
	scl_int		i;
	scl_str		s;
	scl_float	f;
	scl_any		v;
	struct {
		StaticMembers*	$statics;
		mutex_t			$mutex;
	}*			o;
} _scl_frame_t;

typedef struct {
	// Stack
	_scl_frame_t*	sp; // stack pointer
	_scl_frame_t*	bp; // base pointer

	// Trace
	scl_int8**		tp; // trace pointer
	scl_int8**		tbp; // trace base pointer

	// Exception
	jmp_buf*		jmp; // jump pointer
	jmp_buf*		jmp_base; // jump base pointer
	scl_any*		ex; // exception pointer
	scl_any*		ex_base; // exception base pointer
	scl_int8***		et; // exception trace pointer
	scl_int8***		et_base; // exception trace base pointer
	_scl_frame_t**	sp_save; // base pointer save
	_scl_frame_t**	sp_save_base; // stack pointer save base
} _scl_stack_t;

// Create a new instance of a struct
#define ALLOC(_type)	({ extern const StaticMembers _scl_statics_ ## _type; _scl_alloc_struct(sizeof(struct Struct_ ## _type), &_scl_statics_ ## _type); })

// Try to cast the given instance to the given type
#define CAST0(_obj, _type, _type_hash) ((scl_ ## _type) _scl_checked_cast(_obj, _type_hash, #_type))
// Try to cast the given instance to the given type
#define CAST(_obj, _type) CAST0(_obj, _type, id(#_type))

#define TRY			if (setjmp(*(_stack.jmp - 1)) != 666)

// Get the offset of a member in a struct
#define _scl_offsetof(type, member) ((scl_int)&((type*)0)->member)

#define REINTERPRET_CAST(_type, _value) (*(_type*)&(_value))

extern tls _scl_stack_t _stack;

// call a method on an instance
// returns `nil` if the method return type is `none`
scl_any				virtual_call(scl_any instance, scl_int8* methodIdentifier, ...);
// call a method on the super class of an instance
// returns `nil` if the method return type is `none`
scl_any				virtual_call_super(scl_any instance, scl_int8* methodIdentifier, ...);
// call a method on an instance
scl_float			virtual_callf(scl_any instance, scl_int8* methodIdentifier, ...);
// call a method on the super class of an instance
scl_float			virtual_call_superf(scl_any instance, scl_int8* methodIdentifier, ...);
// Throws the given exception
_scl_no_return void scale_throw(scl_any exception) AKA(_scl_throw);
// Returns the current size of the stack
scl_int				stack_size(void) AKA(_scl_stack_size);
// Reallocates the given pointer to the given size
scl_any				managed_realloc(scl_any ptr, scl_int size) AKA(_scl_realloc);
// Allocates a new pointer of the given size
scl_any				managed_alloc(scl_int size) AKA(_scl_alloc);
// Frees the given pointer
void				managed_free(scl_any ptr) AKA(_scl_free);
// Returns the size of the given pointer
scl_int				size_of(scl_any ptr) AKA(_scl_sizeof);
// Asserts that 'b' is true, otherwise throws an 'AssertError' with the given message
void				scl_assert(scl_int b, scl_int8* msg, ...) AKA(_scl_assert);
// Creates a new array with the given size
scl_any				array_of(scl_int size, scl_int element_size) AKA(_scl_new_array_by_size);
// Returns the size of the given array. Throws an 'InvalidArgumentException' if the given object is not an array
scl_int				array_size(scl_any* arr) AKA(_scl_array_size);

#define TYPEID(_type) id(#_type)

_scl_no_return void	_scl_security_throw(int code, scl_int8* msg, ...);
_scl_no_return void	_scl_security_safe_exit(int code);
_scl_no_return void	_scl_runtime_catch();

_scl_constructor
void				_scl_setup(void);

void				_scl_default_signal_handler(scl_int sig_num);
void				print_stacktrace(void);

scl_int				_scl_stack_size(void);
scl_str				_scl_create_string(scl_int8* data);
void				_scl_stack_new(void);
void				_scl_stack_free(void);
void				_scl_stack_resize_fit(scl_int sz);

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
scl_any				_scl_checked_cast(scl_any instance, ID_t target_type, scl_int8* target_type_name);
void				_scl_not_nil_cast(scl_int val, scl_int8* name);
void				_scl_struct_allocation_failure(scl_int val, scl_int8* name);
void				_scl_nil_ptr_dereference(scl_int val, scl_int8* name);
void				_scl_check_not_nil_store(scl_int val, scl_int8* name);
void				_scl_not_nil_return(scl_int val, scl_int8* name);
void				_scl_exception_push(void);
void				_scl_exception_drop(void);
void				_scl_throw(scl_any ex);
int					_scl_run(int argc, char** argv, scl_any main);

const ID_t			id(const scl_int8* data);

scl_int				_scl_identity_hash(scl_any obj);
scl_any				_scl_alloc_struct(scl_int size, const StaticMembers* statics);
void				_scl_free_struct(scl_any ptr);
scl_any				_scl_add_struct(scl_any ptr);
scl_int				_scl_is_instance_of(scl_any ptr, ID_t typeId);
scl_any				_scl_get_method_on_type(scl_any type, ID_t method, ID_t signature, int onSuper);
void				_scl_call_method_or_throw(scl_any instance, ID_t method, ID_t signature, int on_super, scl_int8* method_name, scl_int8* signature_str);
scl_int8**			_scl_callstack_push(void);

scl_int				_scl_find_index_of_struct(scl_any ptr);
scl_any				_scl_cvarargs_to_array(va_list args, scl_int count);

scl_any*			_scl_new_array(scl_int num_elems);
scl_any				_scl_new_array_by_size(scl_int num_elems, scl_int elem_size);
scl_int				_scl_is_array(scl_any* arr);
scl_any*			_scl_multi_new_array(scl_int dimensions, scl_int sizes[dimensions]);
scl_any*			_scl_multi_new_array_by_size(scl_int dimensions, scl_int sizes[dimensions], scl_int elem_size);
scl_int				_scl_array_size(scl_any* arr);
void				_scl_array_check_bounds_or_throw(scl_any* arr, scl_int index);
scl_any*			_scl_array_resize(scl_any* arr, scl_int new_size);
scl_any*			_scl_array_sort(scl_any* arr);
scl_any*			_scl_array_reverse(scl_any* arr);
scl_str				_scl_array_to_string(scl_any* arr);

void				_scl_cleanup_stack_allocations(void);
void				_scl_add_stackallocation(scl_any ptr, scl_int size);
void				_scl_stackalloc_check_bounds_or_throw(scl_any ptr, scl_int index);
scl_int				_scl_index_of_stackalloc(scl_any ptr);
void				_scl_remove_stackallocation(scl_any ptr);
scl_int				_scl_stackalloc_size(scl_any ptr);

int					_scl_gc_is_disabled(void);
int					_scl_gc_pthread_create(pthread_t*, const pthread_attr_t*, scl_any(*)(scl_any), scl_any);
int					_scl_gc_pthread_join(pthread_t, scl_any*);

scl_int				_scl_binary_search(scl_any* arr, scl_int count, scl_any val);
scl_int				_scl_search_method_index(scl_any* methods, scl_int count, ID_t id, ID_t sig);

void				Process$lock(volatile scl_any obj);
void				Process$unlock(volatile scl_any obj);
mutex_t				_scl_mutex_new(void);

__deprecated_msg("Stack resizing has been removed. This function does nothing.")
void				_scl_resize_stack(void);

_scl_always_inline static inline _scl_frame_t* _scl_push() {
	return _stack.sp++;
}

_scl_always_inline static inline _scl_frame_t* _scl_pop() {
#if defined(SCL_DEBUG)
	if (_scl_expect(_stack.sp <= _stack.bp, 0)) {
		_scl_security_throw(EX_STACK_UNDERFLOW, "pop() failed: Not enough data on the stack! Stack pointer was " SCL_HEX_INT_FMT, _stack.sp);
	}
#endif

	return --_stack.sp;
}

_scl_always_inline static inline _scl_frame_t* _scl_offset(scl_int offset) {
	return _stack.bp + offset;
}

_scl_always_inline static inline _scl_frame_t* _scl_positive_offset(scl_int offset) {
	return _stack.sp + offset;
}

_scl_always_inline static inline _scl_frame_t* _scl_top() {
#if defined(SCL_DEBUG)
	if (_scl_expect(_stack.sp <= _stack.bp, 0)) {
		_scl_security_throw(EX_STACK_UNDERFLOW, "top() failed: Not enough data on the stack! Stack pointer was " SCL_HEX_INT_FMT, _stack.sp);
	}
#endif

	return _stack.sp - 1;
}

_scl_always_inline static inline void _scl_popn(scl_int n) {
	_stack.sp -= n;

#if defined(SCL_DEBUG)
	if (_scl_expect(_stack.sp < _stack.bp, 0)) {
		_scl_security_throw(EX_STACK_UNDERFLOW, "popn() failed: Not enough data on the stack! Stack pointer was " SCL_HEX_INT_FMT, _stack.sp);
	}
#endif
}

_scl_always_inline static inline void _scl_swap() {
	_scl_frame_t a = *(_stack.sp - 2);
	_scl_frame_t b = *(_stack.sp - 2);
	*(_stack.sp - 2) = b;
	*(_stack.sp - 2) = a;
}

_scl_always_inline static inline void _scl_over() {
	_scl_frame_t a = *(_stack.sp - 2);
	_scl_frame_t b = *(_stack.sp - 2);
	_scl_frame_t c = *(_stack.sp - 3);
	*(_stack.sp - 2) = c;
	*(_stack.sp - 2) = b;
	*(_stack.sp - 3) = a;
}

_scl_always_inline static inline void _scl_sdup2() {
	_scl_frame_t a = *(_stack.sp - 2);
	_scl_frame_t b = *(_stack.sp - 2);

	*(_stack.sp) = b;
	*(_stack.sp - 2) = a;
	*(_stack.sp - 2) = b;
	_stack.sp++;
}

_scl_always_inline static inline void _scl_swap2() {
	_scl_frame_t c = *(_stack.sp - 2);
	_scl_frame_t b = *(_stack.sp - 2);
	_scl_frame_t a = *(_stack.sp - 3);
	*(_stack.sp - 2) = c;
	*(_stack.sp - 2) = a;
	*(_stack.sp - 3) = b;
}

_scl_always_inline static inline void _scl_add_ii() {
	(_stack.sp - 2)->i += (_stack.sp - 1)->i;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_add_ii_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_add_ii_chk() failed: Not enough data on the stack!");
	_scl_add_ii();
}

_scl_always_inline static inline void _scl_add_if() {
	(_stack.sp - 2)->f = ((scl_float) (_stack.sp - 2)->i) + (_stack.sp - 1)->f;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_add_if_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_add_if_chk() failed: Not enough data on the stack!");
	_scl_add_if();
}

_scl_always_inline static inline void _scl_add_fi() {
	(_stack.sp - 2)->f += ((scl_float) (_stack.sp - 1)->i);
	_stack.sp--;
}

_scl_always_inline static inline void _scl_add_fi_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_add_fi_chk() failed: Not enough data on the stack!");
	_scl_add_fi();
}

_scl_always_inline static inline void _scl_add_ff() {
	(_stack.sp - 2)->f += (_stack.sp - 1)->f;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_add_ff_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_add_ff_chk() failed: Not enough data on the stack!");
	_scl_add_ff();
}

_scl_always_inline static inline void _scl_sub_ii() {
	(_stack.sp - 2)->i -= (_stack.sp - 1)->i;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_sub_ii_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_sub_ii_chk() failed: Not enough data on the stack!");
	_scl_sub_ii();
}

_scl_always_inline static inline void _scl_sub_if() {
	(_stack.sp - 2)->f = ((scl_float) (_stack.sp - 2)->i) - (_stack.sp - 1)->f;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_sub_if_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_sub_if_chk() failed: Not enough data on the stack!");
	_scl_sub_if();
}

_scl_always_inline static inline void _scl_sub_fi() {
	(_stack.sp - 2)->f -= ((scl_float) (_stack.sp - 1)->i);
	_stack.sp--;
}

_scl_always_inline static inline void _scl_sub_fi_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_sub_fi_chk() failed: Not enough data on the stack!");
	_scl_sub_fi();
}

_scl_always_inline static inline void _scl_sub_ff() {
	(_stack.sp - 2)->f -= (_stack.sp - 1)->f;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_sub_ff_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_sub_ff_chk() failed: Not enough data on the stack!");
	_scl_sub_ff();
}

_scl_always_inline static inline void _scl_mul_ii() {
	(_stack.sp - 2)->i *= (_stack.sp - 1)->i;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_mul_ii_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_mul_ii_chk() failed: Not enough data on the stack!");
	_scl_mul_ii();
}

_scl_always_inline static inline void _scl_mul_if() {
	(_stack.sp - 2)->f = ((scl_float) (_stack.sp - 2)->i) * (_stack.sp - 1)->f;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_mul_if_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_mul_if_chk() failed: Not enough data on the stack!");
	_scl_mul_if();
}

_scl_always_inline static inline void _scl_mul_fi() {
	(_stack.sp - 2)->f *= ((scl_float) (_stack.sp - 1)->i);
	_stack.sp--;
}

_scl_always_inline static inline void _scl_mul_fi_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_mul_fi_chk() failed: Not enough data on the stack!");
	_scl_mul_fi();
}

_scl_always_inline static inline void _scl_mul_ff() {
	(_stack.sp - 2)->f *= (_stack.sp - 1)->f;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_mul_ff_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_mul_ff_chk() failed: Not enough data on the stack!");
	_scl_mul_ff();
}

_scl_always_inline static inline void _scl_div_ii() {
	(_stack.sp - 2)->i /= (_stack.sp - 1)->i;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_div_ii_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_div_ii_chk() failed: Not enough data on the stack!");
	_scl_div_ii();
}

_scl_always_inline static inline void _scl_div_if() {
	(_stack.sp - 2)->f = ((scl_float) (_stack.sp - 2)->i) / (_stack.sp - 1)->f;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_div_if_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_div_if_chk() failed: Not enough data on the stack!");
	_scl_div_if();
}

_scl_always_inline static inline void _scl_div_fi() {
	(_stack.sp - 2)->f /= ((scl_float) (_stack.sp - 1)->i);
	_stack.sp--;
}

_scl_always_inline static inline void _scl_div_fi_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_div_fi_chk() failed: Not enough data on the stack!");
	_scl_div_fi();
}

_scl_always_inline static inline void _scl_div_ff() {
	(_stack.sp - 2)->f /= (_stack.sp - 1)->f;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_div_ff_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_div_ff_chk() failed: Not enough data on the stack!");
	_scl_div_ff();
}


_scl_always_inline static inline void _scl_pow_ii() {
	(_stack.sp - 2)->i = (scl_int) pow((_stack.sp - 2)->i, (_stack.sp - 1)->i);
	_stack.sp--;
}

_scl_always_inline static inline void _scl_pow_ii_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_pow_ii_chk() failed: Not enough data on the stack!");
	_scl_pow_ii();
}

_scl_always_inline static inline void _scl_pow_if() {
	(_stack.sp - 2)->f = pow((scl_float) (_stack.sp - 2)->i, (_stack.sp - 1)->f);
	_stack.sp--;
}

_scl_always_inline static inline void _scl_pow_if_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_pow_if_chk() failed: Not enough data on the stack!");
	_scl_pow_if();
}

_scl_always_inline static inline void _scl_pow_fi() {
	(_stack.sp - 2)->f = pow((_stack.sp - 2)->f, (scl_float) (_stack.sp - 1)->i);
	_stack.sp--;
}

_scl_always_inline static inline void _scl_pow_fi_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_pow_fi_chk() failed: Not enough data on the stack!");
	_scl_pow_fi();
}

_scl_always_inline static inline void _scl_pow_ff() {
	(_stack.sp - 2)->f = pow((_stack.sp - 2)->f, (_stack.sp - 1)->f);
	_stack.sp--;
}

_scl_always_inline static inline void _scl_pow_ff_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_pow_ff_chk() failed: Not enough data on the stack!");
	_scl_pow_ff();
}

_scl_always_inline static inline void _scl_mod() {
	(_stack.sp - 2)->i %= (_stack.sp - 1)->i;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_mod_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_mod_chk() failed: Not enough data on the stack!");
	_scl_mod();
}

_scl_always_inline static inline void _scl_land() {
	(_stack.sp - 2)->i &= (_stack.sp - 1)->i;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_land_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_land_chk() failed: Not enough data on the stack!");
	_scl_land();
}

_scl_always_inline static inline void _scl_lor() {
	(_stack.sp - 2)->i |= (_stack.sp - 1)->i;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_lor_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_lor_chk() failed: Not enough data on the stack!");
	_scl_lor();
}

_scl_always_inline static inline void _scl_lxor() {
	(_stack.sp - 2)->i ^= (_stack.sp - 1)->i;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_lxor_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_lxor_chk() failed: Not enough data on the stack!");
	_scl_lxor();
}

_scl_always_inline static inline void _scl_lnot() {
	(_stack.sp - 1)->i = ~((_stack.sp - 1)->i);
}

_scl_always_inline static inline void _scl_lnot_chk() {
	_scl_assert(_scl_stack_size() >= 1, "_scl_lnot_chk() failed: Not enough data on the stack!");
	_scl_lnot();
}

_scl_always_inline static inline void _scl_lsr() {
	(_stack.sp - 2)->i >>= (_stack.sp - 1)->i;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_lsr_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_lsr_chk() failed: Not enough data on the stack!");
	_scl_lsr();
}

_scl_always_inline static inline void _scl_lsl() {
	(_stack.sp - 2)->i <<= (_stack.sp - 1)->i;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_lsl_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_lsl_chk() failed: Not enough data on the stack!");
	_scl_lsl();
}


_scl_always_inline static inline void _scl_eq_ii() {
	(_stack.sp - 2)->i = (_stack.sp - 2)->i == (_stack.sp - 1)->i;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_eq_ii_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_eq_ii_chk() failed: Not enough data on the stack!");
	_scl_eq_ii();
}

_scl_always_inline static inline void _scl_eq_if() {
	(_stack.sp - 2)->i = ((scl_float) (_stack.sp - 2)->i) == (_stack.sp - 1)->f;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_eq_if_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_eq_if_chk() failed: Not enough data on the stack!");
	_scl_eq_if();
}

_scl_always_inline static inline void _scl_eq_fi() {
	(_stack.sp - 2)->i = (_stack.sp - 2)->f == ((scl_float) (_stack.sp - 1)->i);
	_stack.sp--;
}

_scl_always_inline static inline void _scl_eq_fi_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_eq_fi_chk() failed: Not enough data on the stack!");
	_scl_eq_fi();
}

_scl_always_inline static inline void _scl_eq_ff() {
	(_stack.sp - 2)->i = (_stack.sp - 2)->i == (_stack.sp - 1)->f;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_eq_ff_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_eq_ff_chk() failed: Not enough data on the stack!");
	_scl_eq_ff();
}

_scl_always_inline static inline void _scl_ne_ii() {
	(_stack.sp - 2)->i = (_stack.sp - 2)->i != (_stack.sp - 1)->i;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_ne_ii_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_ne_ii_chk() failed: Not enough data on the stack!");
	_scl_ne_ii();
}

_scl_always_inline static inline void _scl_ne_if() {
	(_stack.sp - 2)->i = ((scl_float) (_stack.sp - 2)->i) != (_stack.sp - 1)->f;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_ne_if_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_ne_if_chk() failed: Not enough data on the stack!");
	_scl_ne_if();
}

_scl_always_inline static inline void _scl_ne_fi() {
	(_stack.sp - 2)->i = (_stack.sp - 2)->f != ((scl_float) (_stack.sp - 1)->i);
	_stack.sp--;
}

_scl_always_inline static inline void _scl_ne_fi_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_ne_fi_chk() failed: Not enough data on the stack!");
	_scl_ne_fi();
}

_scl_always_inline static inline void _scl_ne_ff() {
	(_stack.sp - 2)->i = (_stack.sp - 2)->f != (_stack.sp - 1)->f;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_ne_ff_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_ne_ff_chk() failed: Not enough data on the stack!");
	_scl_ne_ff();
}

_scl_always_inline static inline void _scl_gt_ii() {
	(_stack.sp - 2)->i = (_stack.sp - 2)->i > (_stack.sp - 1)->i;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_gt_ii_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_gt_ii_chk() failed: Not enough data on the stack!");
	_scl_gt_ii();
}

_scl_always_inline static inline void _scl_gt_if() {
	(_stack.sp - 2)->i = ((scl_float) (_stack.sp - 2)->i) > (_stack.sp - 1)->f;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_gt_if_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_gt_if_chk() failed: Not enough data on the stack!");
	_scl_gt_if();
}

_scl_always_inline static inline void _scl_gt_fi() {
	(_stack.sp - 2)->i = (_stack.sp - 2)->f > ((scl_float) (_stack.sp - 1)->i);
	_stack.sp--;
}

_scl_always_inline static inline void _scl_gt_fi_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_gt_fi_chk() failed: Not enough data on the stack!");
	_scl_gt_fi();
}

_scl_always_inline static inline void _scl_gt_ff() {
	(_stack.sp - 2)->i = (_stack.sp - 2)->f > (_stack.sp - 1)->f;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_gt_ff_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_gt_ff_chk() failed: Not enough data on the stack!");
	_scl_gt_ff();
}

_scl_always_inline static inline void _scl_ge_ii() {
	(_stack.sp - 2)->i = (_stack.sp - 2)->i >= (_stack.sp - 1)->i;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_ge_ii_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_ge_ii_chk() failed: Not enough data on the stack!");
	_scl_ge_ii();
}

_scl_always_inline static inline void _scl_ge_if() {
	(_stack.sp - 2)->i = ((scl_float) (_stack.sp - 2)->i) >= (_stack.sp - 1)->f;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_ge_if_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_ge_if_chk() failed: Not enough data on the stack!");
	_scl_ge_if();
}

_scl_always_inline static inline void _scl_ge_fi() {
	(_stack.sp - 2)->i = (_stack.sp - 2)->f >= ((scl_float) (_stack.sp - 1)->i);
	_stack.sp--;
}

_scl_always_inline static inline void _scl_ge_fi_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_ge_fi_chk() failed: Not enough data on the stack!");
	_scl_ge_fi();
}

_scl_always_inline static inline void _scl_ge_ff() {
	(_stack.sp - 2)->i = (_stack.sp - 2)->f >= (_stack.sp - 1)->f;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_ge_ff_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_ge_ff_chk() failed: Not enough data on the stack!");
	_scl_ge_ff();
}

_scl_always_inline static inline void _scl_lt_ii() {
	(_stack.sp - 2)->i = (_stack.sp - 2)->i < (_stack.sp - 1)->i;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_lt_ii_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_lt_ii_chk() failed: Not enough data on the stack!");
	_scl_lt_ii();
}

_scl_always_inline static inline void _scl_lt_if() {
	(_stack.sp - 2)->i = ((scl_float) (_stack.sp - 2)->i) < (_stack.sp - 1)->f;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_lt_if_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_lt_if_chk() failed: Not enough data on the stack!");
	_scl_lt_if();
}

_scl_always_inline static inline void _scl_lt_fi() {
	(_stack.sp - 2)->i = (_stack.sp - 2)->f < ((scl_float) (_stack.sp - 1)->i);
	_stack.sp--;
}

_scl_always_inline static inline void _scl_lt_fi_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_lt_fi_chk() failed: Not enough data on the stack!");
	_scl_lt_fi();
}

_scl_always_inline static inline void _scl_lt_ff() {
	(_stack.sp - 2)->i = (_stack.sp - 2)->f < (_stack.sp - 1)->f;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_lt_ff_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_lt_ff_chk() failed: Not enough data on the stack!");
	_scl_lt_ff();
}

_scl_always_inline static inline void _scl_le_ii() {
	(_stack.sp - 2)->i = (_stack.sp - 2)->i <= (_stack.sp - 1)->i;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_le_ii_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_le_ii_chk() failed: Not enough data on the stack!");
	_scl_le_ii();
}

_scl_always_inline static inline void _scl_le_if() {
	(_stack.sp - 2)->i = ((scl_float) (_stack.sp - 2)->i) <= (_stack.sp - 1)->f;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_le_if_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_le_if_chk() failed: Not enough data on the stack!");
	_scl_le_if();
}

_scl_always_inline static inline void _scl_le_fi() {
	(_stack.sp - 2)->i = (_stack.sp - 2)->f <= ((scl_float) (_stack.sp - 1)->i);
	_stack.sp--;
}

_scl_always_inline static inline void _scl_le_fi_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_le_fi_chk() failed: Not enough data on the stack!");
	_scl_le_fi();
}

_scl_always_inline static inline void _scl_le_ff() {
	(_stack.sp - 2)->i = (_stack.sp - 2)->f <= (_stack.sp - 1)->f;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_le_ff_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_le_ff_chk() failed: Not enough data on the stack!");
	_scl_le_ff();
}

_scl_always_inline static inline void _scl_and() {
	(_stack.sp - 2)->i = (_stack.sp - 2)->i && (_stack.sp - 1)->i;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_and_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_and_chk() failed: Not enough data on the stack!");
	_scl_and();
}

_scl_always_inline static inline void _scl_or() {
	(_stack.sp - 2)->i = (_stack.sp - 2)->i || (_stack.sp - 1)->i;
	_stack.sp--;
}

_scl_always_inline static inline void _scl_or_chk() {
	_scl_assert(_scl_stack_size() >= 2, "_scl_or_chk() failed: Not enough data on the stack!");
	_scl_or();
}

_scl_always_inline static inline void _scl_not() {
	(_stack.sp - 1)->i = !(_stack.sp - 1)->i;
}

_scl_always_inline static inline void _scl_not_chk() {
	_scl_assert(_scl_stack_size() >= 1, "_scl_not_chk() failed: Not enough data on the stack!");
	_scl_not();
}

#endif // __SCALE_RUNTIME_H__
