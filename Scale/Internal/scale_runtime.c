#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>

#include "scale_runtime.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define unimplemented do { fprintf(stderr, "%s:%d: %s: Not Implemented\n", __FILE__, __LINE__, __FUNCTION__); exit(1); } while (0)

#define likely(x) _scl_expect(!!(x), 1)
#define unlikely(x) _scl_expect(!!(x), 0)

typedef struct Struct {
	// Actual function ptrs for this object
	_scl_lambda*	vtable;
	// Typeinfo for this object
	TypeInfo*		statics;
	// Mutex for this object
	scl_any			mutex;
} Struct;

typedef struct Struct_Exception {
	Struct rtFields;
	scl_str msg;
	struct Struct_Array* stackTrace;
	scl_str errno_str;
}* scl_Exception;

typedef struct Struct_RuntimeError {
	struct Struct_Exception self;
}* scl_RuntimeError;

typedef struct Struct_NullPointerException {
	struct Struct_Exception self;
}* scl_NullPointerException;

struct Struct_Array {
	Struct rtFields;
	scl_any* values;
	scl_int count;
	scl_int capacity;
	scl_int initCapacity;
};

typedef struct Struct_IndexOutOfBoundsException {
	struct Struct_Exception self;
}* scl_IndexOutOfBoundsException;

typedef struct Struct_AssertError {
	Struct rtFields;
	scl_str msg;
	struct Struct_Array* stackTrace;
	scl_str errno_str;
}* scl_AssertError;

typedef struct Struct_UnreachableError {
	Struct rtFields;
	scl_str msg;
	struct Struct_Array* stackTrace;
	scl_str errno_str;
}* scl_UnreachableError;

struct Struct_str {
	struct scale_string s;
};

typedef struct Struct_InvalidArgumentException {
	struct Struct_Exception self;
}* scl_InvalidArgumentException;

_scl_symbol_hidden static memory_layout_t* _scl_get_memory_layout(scl_any ptr) {
	if (likely(GC_is_heap_ptr(ptr))) {
		return (memory_layout_t*) GC_base(ptr);
	}
	return nil;
}

scl_int _scl_sizeof(scl_any ptr) {
	if (unlikely(ptr == nil || !GC_is_heap_ptr(ptr))) {
		return 0;
	}
	memory_layout_t* layout = _scl_get_memory_layout(ptr);
	if (unlikely(layout == nil)) {
		return 0;
	}
	return layout->size;
}

#define FLAG_INSTANCE	0b00000001
#define FLAG_ARRAY		0b00000010

scl_any _scl_alloc(scl_int size) {
	scl_int orig_size = size;
	size = ((size + 7) >> 3) << 3;
	if (unlikely(size == 0)) size = 8;

	scl_any ptr = GC_malloc(size + sizeof(memory_layout_t));
	if (unlikely(ptr == nil)) {
		_scl_runtime_error(EX_BAD_PTR, "allocate() failed!");
	}

	((memory_layout_t*) ptr)->size = orig_size;
	((memory_layout_t*) ptr)->flags = 0;

	return ptr + sizeof(memory_layout_t);
}

scl_any _scl_realloc(scl_any ptr, scl_int size) {
	if (unlikely(size == 0)) {
		_scl_free(ptr);
		return nil;
	}

	scl_int orig_size = size;
	size = ((size + 7) >> 3) << 3;
	if (unlikely(size == 0)) size = 8;

	memory_layout_t* layout = _scl_get_memory_layout(ptr);

	ptr = GC_realloc(ptr - sizeof(memory_layout_t), size + sizeof(memory_layout_t));
	if (unlikely(ptr == nil)) {
		_scl_runtime_error(EX_BAD_PTR, "realloc() failed!");
	}

	((memory_layout_t*) ptr)->size = orig_size;
	if (likely(layout)) {
		((memory_layout_t*) ptr)->flags = layout->flags;
	} else {
		((memory_layout_t*) ptr)->flags = 0;
	}

	return ptr + sizeof(memory_layout_t);
}

void _scl_free(scl_any ptr) {
	if (unlikely(ptr == nil)) return;
	if (_scl_is_instance(ptr)) {
		cxx_std_recursive_mutex_delete(((Struct*) ptr)->mutex);
	}
	ptr = GC_base(ptr);
	if (likely(ptr != nil)) {
		((memory_layout_t*) ptr)->size = 0;
		((memory_layout_t*) ptr)->flags = 0;
		((memory_layout_t*) ptr)->array_elem_size = 0;
		GC_free(ptr);
	}
}

void _scl_assert(scl_int b, const scl_int8* msg, ...) {
	if (unlikely(!b)) {
		scl_int8 cmsg[strlen(msg) * 8];
		va_list list;
		va_start(list, msg);
		vsnprintf(cmsg, strlen(msg) * 8 - 1, msg, list);
		va_end(list);

		char tmp[strlen(cmsg)];
		snprintf(cmsg, 22 + strlen(cmsg), "Assertion failed: %s", tmp);

		fprintf(stderr, "%s\n", cmsg);
		abort();
	}
}

void builtinUnreachable(void) {
#if __has_builtin(__builtin_unreachable)
	__builtin_unreachable();
#else
	abort();
#endif
}

scl_int builtinIsInstanceOf(scl_any obj, scl_str type) {
	return _scl_is_instance_of(obj, type->hash);
}

_scl_symbol_hidden static void native_trace(void);

_scl_no_return void _scl_runtime_error(int code, const scl_int8* msg, ...) {
	va_list args;
	va_start(args, msg);
	size_t len = strlen(msg) * 8;
	scl_int8 str[len];
	vsnprintf(str, len, msg, args);
	scl_int size = 22 + strlen(str);
	scl_int8 cmsg[size];
	snprintf(cmsg, size, "Exception: %s\n", str);
	va_end(args);

	scl_RuntimeError ex = ALLOC(RuntimeError);
	virtual_call(ex, "init(s;)V;", _scl_create_string(cmsg));
	
	_scl_throw(ex);
}

scl_any _scl_cvarargs_to_array(va_list args, scl_int count) {
	scl_any* arr = (scl_any*) _scl_new_array_by_size(count, sizeof(scl_any));
	for (scl_int i = 0; i < count; i++) {
		arr[i] = *va_arg(args, scl_any*);
	}
	return arr;
}

_scl_symbol_hidden static void _scl_sigaction_handler(scl_int sig_num, siginfo_t* sig_info, void* ucontext) {
	const scl_int8* signalString = strsignal(sig_num);

	fprintf(stderr, "Signal: %s\n", signalString);
	fprintf(stderr, ((sig_num == SIGSEGV) ? "Tried read/write of faulty address %p\n" : "Faulty address: %p\n"), sig_info->si_addr);
	if (unlikely(sig_info->si_errno)) {
		fprintf(stderr, "Error code: %d\n", sig_info->si_errno);
	}
	native_trace();
	fprintf(stderr, "\n");
	exit(sig_num);
}

_scl_symbol_hidden static void _scl_signal_handler(scl_int sig_num) {
	const scl_int8* signalString = strsignal(sig_num);

	printf("\n");

	printf("Signal: %s\n", signalString);
	if (errno) {
		printf("errno: %s\n", strerror(errno));
	}
	native_trace();

	exit(sig_num);
}

void _scl_sleep(scl_int millis) {
	sleep(millis);
}

void _scl_lock(scl_any obj) {
	if (unlikely(!_scl_is_instance(obj))) {
		return;
	}
	cxx_std_recursive_mutex_lock(((Struct*) obj)->mutex);
}

void _scl_unlock(scl_any obj) {
	if (unlikely(!_scl_is_instance(obj))) {
		return;
	}
	cxx_std_recursive_mutex_unlock(((Struct*) obj)->mutex);
}

#if !defined(__pure2)
#define __pure2
#endif

__pure2 const ID_t type_id(const scl_int8* data) {
	ID_t h = 3323198485UL;
	for (;*data;++data) {
		h ^= *data;
		h *= 0x5BD1E995;
		h ^= h >> 15;
	}
	return h;
}

__pure2 _scl_symbol_hidden static scl_uint _scl_rotl(const scl_uint value, const scl_int shift) {
    return (value << shift) | (value >> ((sizeof(scl_uint) << 3) - shift));
}

__pure2 _scl_symbol_hidden static scl_uint _scl_rotr(const scl_uint value, const scl_int shift) {
    return (value >> shift) | (value << ((sizeof(scl_uint) << 3) - shift));
}

scl_int _scl_identity_hash(scl_any _obj) {
	if (unlikely(!_scl_is_instance(_obj))) {
		return REINTERPRET_CAST(scl_int, _obj);
	}
	Struct* obj = (Struct*) _obj;
	cxx_std_recursive_mutex_lock(obj->mutex);
	scl_int size = _scl_sizeof(obj);
	scl_int hash = REINTERPRET_CAST(scl_int, obj) % 17;
	for (scl_int i = 0; i < size; i++) {
		hash = _scl_rotl(hash, 5) ^ ((scl_int8*) obj)[i];
	}
	cxx_std_recursive_mutex_unlock(obj->mutex);
	return hash;
}

scl_any _scl_atomic_clone(scl_any ptr) {
	scl_int size = _scl_sizeof(ptr);
	scl_any clone = _scl_alloc(size);

	memcpy(clone, ptr, size);
	return clone;
}

_scl_symbol_hidden static scl_int _scl_search_method_index(const struct _scl_methodinfo* const methods, ID_t id, ID_t sig);

_scl_lambda _scl_get_method_on_type(scl_any type, ID_t method, ID_t signature, int onSuper) {
	if (unlikely(!_scl_is_instance(type))) {
		_scl_runtime_error(EX_CAST_ERROR, "Tried getting method on non-struct type (%p)", type);
	}

	const struct TypeInfo* ti;
	const _scl_lambda* vtable;

	if (likely(!onSuper)) {
		ti = ((Struct*) type)->statics;
		vtable = ((Struct*) type)->vtable;
	} else {
		ti = ((Struct*) type)->statics->super;
		vtable = ((Struct*) type)->statics->super->vtable;
	}

	scl_int index = _scl_search_method_index(ti->vtable_info, method, signature);
	return index >= 0 ? vtable[index] : nil;
}

_scl_symbol_hidden static size_t str_index_of_or(const scl_int8* str, char c, size_t len) {
	size_t i = 0;
	while (str[i] != c) {
		if (str[i] == '\0') return len;
		i++;
	}
	return i;
}

_scl_symbol_hidden static size_t str_index_of(const scl_int8* str, char c) {
	return str_index_of_or(str, c, -1);
}

_scl_symbol_hidden static void split_at(const scl_int8* str, size_t len, size_t index, scl_int8* left, scl_int8* right) {
	strncpy(left, str, index);
	left[index] = '\0';

	strncpy(right, str + index, len - index);
	right[len - index] = '\0';
}

scl_any _scl_get_vtable_function(scl_int onSuper, scl_any instance, const scl_int8* methodIdentifier) {
	size_t methodLen = strlen(methodIdentifier);
	ID_t signatureHash;
	size_t methodNameLen = str_index_of_or(methodIdentifier, '(', methodLen);
	scl_int8 methodName[methodNameLen];
	scl_int8 signature[methodLen - methodNameLen + 1];
	if (likely(sizeof(signature) > 1)) {
		split_at(methodIdentifier, methodLen, methodNameLen, methodName, signature);
		signatureHash = type_id(signature);
	} else {
		strncpy(methodName, methodIdentifier, methodLen);
		methodName[methodLen] = '\0';
		signature[0] = '\0';
		signatureHash = 0;
	}
	ID_t methodNameHash = type_id(methodName);

	_scl_lambda m = _scl_get_method_on_type(instance, methodNameHash, signatureHash, onSuper);
	if (unlikely(m == nil)) {
		_scl_runtime_error(EX_BAD_PTR, "Method '%s%s' not found on type '%s'", methodName, signature, ((Struct*) instance)->statics->type_name);
	}
	return m;
}

scl_any _scl_alloc_struct(const TypeInfo* statics) {
	scl_any ptr = _scl_alloc(statics->size);

	memory_layout_t* layout = _scl_get_memory_layout(ptr);
	if (unlikely(layout == nil)) {
		_scl_runtime_error(EX_BAD_PTR, "Tried to access nil pointer");
	}
	layout->flags |= FLAG_INSTANCE;

	((Struct*) ptr)->mutex = cxx_std_recursive_mutex_new();
	((Struct*) ptr)->statics = (TypeInfo*) statics;
	((Struct*) ptr)->vtable = (_scl_lambda*) statics->vtable;

	return ptr;
}

scl_int _scl_is_instance(scl_any ptr) {
	if (unlikely(ptr == nil)) return 0;
	memory_layout_t* layout = _scl_get_memory_layout(ptr);
	if (unlikely(layout == nil)) return 0;
	return layout->flags & FLAG_INSTANCE;
}

scl_int _scl_is_instance_of(scl_any ptr, ID_t type_id) {
	if (unlikely(((scl_int) ptr) <= 0)) return 0;
	if (unlikely(!_scl_is_instance(ptr))) return 0;

	Struct* ptrStruct = (Struct*) ptr;

	for (const TypeInfo* super = ptrStruct->statics; super != nil; super = super->super) {
		if (super->type == type_id) return 1;
	}

	return 0;
}

_scl_symbol_hidden static scl_int _scl_search_method_index(const struct _scl_methodinfo* const methods, ID_t id, ID_t sig) {
	if (unlikely(methods == nil)) return -1;

	for (scl_int i = 0; *(scl_int*) &(methods[i].pure_name); i++) {
		if (methods[i].pure_name == id && (sig == 0 || methods[i].signature == sig)) {
			return i;
		}
	}
	return -1;
}

_scl_symbol_hidden static void _scl_set_up_signal_handler(void) {
#if !defined(_WIN32)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-W#pragma-messages"
_scl_symbol_hidden 
	static struct sigaction act = {
		.sa_sigaction = (void(*)(int, siginfo_t*, void*)) _scl_sigaction_handler,
		.sa_flags = SA_SIGINFO,
		.sa_mask = sigmask(SIGINT) | sigmask(SIGILL) | sigmask(SIGTRAP) | sigmask(SIGABRT) | sigmask(SIGBUS) | sigmask(SIGSEGV)
	};
	#define SIGACTION(_sig) if (sigaction(_sig, &act, NULL) == -1) { fprintf(stderr, "Failed to set up signal handler\n"); fprintf(stderr, "Error: %s (%d)\n", strerror(errno), errno); }
	
	SIGACTION(SIGINT);
	SIGACTION(SIGILL);
	SIGACTION(SIGTRAP);
	SIGACTION(SIGABRT);
	SIGACTION(SIGBUS);
	SIGACTION(SIGSEGV);

	#undef SIGACTION
#pragma clang diagnostic pop
#else
#define SIGACTION(_sig) signal(_sig, (void(*)(int)) _scl_signal_handler)
	
	SIGACTION(SIGINT);
	SIGACTION(SIGILL);
	SIGACTION(SIGTRAP);
	SIGACTION(SIGABRT);
	SIGACTION(SIGBUS);
	SIGACTION(SIGSEGV);
#endif
}

scl_any _scl_get_stderr() {
	return (scl_any) stderr;
}

scl_any _scl_get_stdout() {
	return (scl_any) stdout;
}

scl_any _scl_get_stdin() {
	return (scl_any) stdin;
}

scl_any _scl_checked_cast(scl_any instance, ID_t target_type, const scl_int8* target_type_name) {
	if (unlikely(!_scl_is_instance_of(instance, target_type))) {
		typedef struct Struct_CastError {
			Struct rtFields;
			scl_str msg;
			struct Struct_Array* stackTrace;
			scl_str errno_str;
		}* scl_CastError;

		if (instance == nil) {
			scl_int size = 96 + strlen(target_type_name);
			scl_int8 cmsg[size];
			snprintf(cmsg, size - 1, "Cannot cast nil to type '%s'\n", target_type_name);
			_scl_assert(0, cmsg);
		}

		memory_layout_t* layout = _scl_get_memory_layout(instance);

		scl_int size;
		scl_bool is_instance = layout && (layout->flags & FLAG_INSTANCE);
		if (is_instance) {
			size = 64 + strlen(((Struct*) instance)->statics->type_name) + strlen(target_type_name);
		} else {
			size = 96 + strlen(target_type_name);
		}

		char cmsg[size];
		if (is_instance) {
			snprintf(cmsg, size - 1, "Cannot cast instance of struct '%s' to type '%s'\n", ((Struct*) instance)->statics->type_name, target_type_name);
		} else {
			snprintf(cmsg, size - 1, "Cannot cast non-object to type '%s'\n", target_type_name);
		}
		_scl_assert(0, cmsg);
	}
	return instance;
}

scl_int8* _scl_typename_or_else(scl_any instance, const scl_int8* else_) {
	if (likely(_scl_is_instance(instance))) {
		return (scl_int8*) ((Struct*) instance)->statics->type_name;
	}
	return (scl_int8*) else_;
}

scl_any _scl_new_array_by_size(scl_int num_elems, scl_int elem_size) {
	if (unlikely(num_elems < 0)) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array size must not be less than 0");
	}
	scl_int size = num_elems * elem_size;
	scl_any arr = _scl_alloc(size);
	memory_layout_t* layout = _scl_get_memory_layout(arr);
	layout->flags |= FLAG_ARRAY;
	layout->array_elem_size = elem_size;
	return arr;
}

struct vtable {
	memory_layout_t memory_layout;
	_scl_lambda funcs[];
};

scl_any _scl_migrate_foreign_array(const void* const arr, scl_int num_elems, scl_int elem_size) {
	if (unlikely(num_elems < 0)) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array size must not be less than 0");
	}
	scl_any* new_arr = _scl_new_array_by_size(num_elems, elem_size);
	memcpy(new_arr, arr, num_elems * elem_size);
	return new_arr;
}

scl_int _scl_is_array(scl_any* arr) {
	if (unlikely(arr == nil)) {
		return 0;
	}
	memory_layout_t* layout = _scl_get_memory_layout(arr);
	if (unlikely(layout == nil)) {
		return 0;
	}
	return layout->flags & FLAG_ARRAY;
}

scl_any* _scl_multi_new_array_by_size(scl_int dimensions, scl_int sizes[], scl_int elem_size) {
	if (unlikely(dimensions < 1)) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array dimensions must not be less than 1");
	}
	if (dimensions == 1) {
		return (scl_any*) _scl_new_array_by_size(sizes[0], elem_size);
	}
	scl_any* arr = (scl_any*) _scl_alloc(sizes[0] * sizeof(scl_any));
	memory_layout_t* layout = _scl_get_memory_layout(arr);
	layout->flags |= FLAG_ARRAY;
	layout->array_elem_size = sizeof(scl_any);
	for (scl_int i = 0; i < sizes[0]; i++) {
		arr[i] = _scl_multi_new_array_by_size(dimensions - 1, &(sizes[1]), elem_size);
	}
	return arr;
}

scl_int _scl_array_size_unchecked(scl_any* arr) {
	memory_layout_t* lay = _scl_get_memory_layout(arr);
	return lay->size / lay->array_elem_size;
}

scl_int _scl_array_elem_size_unchecked(scl_any* arr) {
	return _scl_get_memory_layout(arr)->array_elem_size;
}

scl_int _scl_array_size(scl_any* arr) {
	if (unlikely(arr == nil)) {
		_scl_runtime_error(EX_BAD_PTR, "nil pointer detected");
	}
	if (unlikely(!_scl_is_array(arr))) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array must be initialized with 'new[]'");
	}
	return _scl_array_size_unchecked(arr);
}

scl_int _scl_array_elem_size(scl_any* arr) {
	if (unlikely(arr == nil)) {
		_scl_runtime_error(EX_BAD_PTR, "nil pointer detected");
	}
	if (unlikely(!_scl_is_array(arr))) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array must be initialized with 'new[]'");
	}
	return _scl_array_elem_size_unchecked(arr);
}

void _scl_array_check_bounds_or_throw(scl_any* arr, scl_int index) {
	if (unlikely(arr == nil)) {
		_scl_runtime_error(EX_BAD_PTR, "nil pointer detected");
	}
	if (unlikely(!_scl_is_array(arr))) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array must be initialized with 'new[]'");
	}
	scl_int size = _scl_array_size_unchecked(arr);
	if (index < 0 || index >= size) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Index " SCL_INT_FMT " out of bounds for array of size " SCL_INT_FMT, index, size);
	}
}

scl_any* _scl_array_resize(scl_int new_size, scl_any* arr) {
	if (unlikely(arr == nil)) {
		_scl_runtime_error(EX_BAD_PTR, "nil pointer detected");
	}
	if (unlikely(!_scl_is_array(arr))) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array must be initialized with 'new[]'");
	}
	if (unlikely(new_size < 1)) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array size must not be less than 1");
	}
	scl_int elem_size = _scl_get_memory_layout(arr)->array_elem_size;
	scl_any* new_arr = (scl_any*) _scl_realloc(arr, new_size * elem_size);
	memory_layout_t* layout = _scl_get_memory_layout(new_arr);
	layout->flags |= FLAG_ARRAY;
	layout->array_elem_size = elem_size;
	return new_arr;
}

#define NBYTES_TO_MASK(nbytes) ((1 << (nbytes * 8)) - 1)

static inline void write_int8(scl_any ptr, scl_int8 value) { *(scl_int8*) ptr = value; }
static inline void write_int16(scl_any ptr, scl_int16 value) { *(scl_int16*) ptr = value; }
static inline void write_int32(scl_any ptr, scl_int32 value) { *(scl_int32*) ptr = value; }
static inline void write_int64(scl_any ptr, scl_int64 value) { *(scl_int64*) ptr = value; }
static inline void write_sized(scl_any ptr, scl_int value, scl_int size) {
	switch (size) {
		case 1: write_int8(ptr, value); break;
		case 2: write_int16(ptr, value); break;
		case 4: write_int32(ptr, value); break;
		case 8: write_int64(ptr, value); break;
		default: _scl_runtime_error(EX_INVALID_ARGUMENT, "Invalid size: " SCL_INT_FMT, size);
	}
}

static inline scl_int8 read_int8(scl_any ptr) { return *(scl_int8*) ptr; }
static inline scl_int16 read_int16(scl_any ptr) { return *(scl_int16*) ptr; }
static inline scl_int32 read_int32(scl_any ptr) { return *(scl_int32*) ptr; }
static inline scl_int64 read_int64(scl_any ptr) { return *(scl_int64*) ptr; }
static inline scl_int read_sized(scl_any ptr, scl_int size) {
	switch (size) {
		case 1: return read_int8(ptr);
		case 2: return read_int16(ptr);
		case 4: return read_int32(ptr);
		case 8: return read_int64(ptr);
		default: _scl_runtime_error(EX_INVALID_ARGUMENT, "Invalid size: " SCL_INT_FMT, size);
	}
}

void _scl_array_set(scl_any arr, scl_int index, scl_int value) {
	if (unlikely(arr == nil)) {
		_scl_runtime_error(EX_BAD_PTR, "nil pointer detected");
	}
	if (unlikely(!_scl_is_array(arr))) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array must be initialized with 'new[]'");
	}

	scl_int size = _scl_array_size_unchecked(arr);

	if (unlikely(index < 0 || index >= size)) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Index " SCL_INT_FMT " out of bounds for array of size " SCL_INT_FMT, index, size);
	}

	scl_int elem_size = _scl_array_elem_size_unchecked(arr);
	write_sized(arr + index * elem_size, value, elem_size);
}

scl_any _scl_array_get(scl_any arr, scl_int index) {
	if (unlikely(arr == nil)) {
		_scl_runtime_error(EX_BAD_PTR, "nil pointer detected");
	}
	if (unlikely(!_scl_is_array(arr))) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array must be initialized with 'new[]'");
	}

	scl_int size = _scl_array_size_unchecked(arr);

	if (unlikely(index < 0 || index >= size)) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Index " SCL_INT_FMT " out of bounds for array of size " SCL_INT_FMT, index, size);
	}

	scl_int elem_size = _scl_array_elem_size_unchecked(arr);
	return (scl_any) read_sized(arr + index * elem_size, elem_size);
}

void _scl_array_setf(scl_any arr, scl_int index, scl_float value) {
	_scl_array_set(arr, index, REINTERPRET_CAST(scl_int, value));
}

scl_float _scl_array_getf(scl_any arr, scl_int index) {
	return REINTERPRET_CAST(scl_float, _scl_array_get(arr, index));
}

scl_any* _scl_array_sort(scl_any* arr) {
	if (unlikely(arr == nil)) {
		_scl_runtime_error(EX_BAD_PTR, "nil pointer detected");
	}
	if (unlikely(!_scl_is_array(arr))) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array must be initialized with 'new[]'");
	}
	scl_int size = _scl_array_size_unchecked(arr);
	scl_int elem_size = _scl_array_elem_size_unchecked(arr);
	scl_int8* tmpArr = (scl_int8*) arr;
	const scl_uint mask = NBYTES_TO_MASK(elem_size);
	for (scl_int i = 0; i < size; i++) {
		scl_int tmp = read_sized(tmpArr + i * elem_size, elem_size);
		scl_int j = i - 1;
		while (j >= 0) {
			scl_int jTmp = read_sized(tmpArr + j * elem_size, elem_size);
			if (tmp < jTmp) {
				write_sized(tmpArr + (j + 1) * elem_size, read_sized(tmpArr + j * elem_size, elem_size), elem_size);
				write_sized(tmpArr + j * elem_size, tmp, elem_size);
			} else {
				write_sized(tmpArr + (j + 1) * elem_size, tmp, elem_size);
				break;
			}
			j--;
		}
	}
	return arr;
}

scl_any* _scl_array_reverse(scl_any* arr) {
	if (unlikely(arr == nil)) {
		_scl_runtime_error(EX_BAD_PTR, "nil pointer detected");
	}
	if (unlikely(!_scl_is_array(arr))) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array must be initialized with 'new[]'");
	}
	scl_int size = _scl_array_size_unchecked(arr);
	scl_int elem_size = _scl_array_elem_size_unchecked(arr);
	scl_int half = size / 2;
	scl_int8* tmpArr = (scl_int8*) arr;
	for (scl_int i = 0; i < half; i++) {
		scl_int tmp = read_sized(tmpArr + i * elem_size, elem_size);
		write_sized(tmpArr + i * elem_size, read_sized(tmpArr + (size - i - 1) * elem_size, elem_size), elem_size);
		write_sized(tmpArr + (size - i - 1) * elem_size, tmp, elem_size);
	}
	return arr;
}

void _scl_trace_remove(struct _scl_backtrace* bt) {
	bt->marker = 0;
}

void _scl_unlock_ptr(void* lock_ptr) {
	Struct* lock = *(Struct**) lock_ptr;
	cxx_std_recursive_mutex_unlock(lock->mutex);
}

void _scl_delete_ptr(void* _ptr) {
	scl_any* ptr = (scl_any*) _ptr;
	_scl_free(*ptr);
}

void _scl_throw(scl_any ex) {
	scl_uint* stack_top = (scl_uint*) &stack_top;
	struct GC_stack_base sb;
	GC_get_my_stackbottom(&sb);
	scl_uint* stack_bottom = sb.mem_base;

	scl_int iteration_direction = stack_top < stack_bottom ? 1 : -1;

	ID_t error_type = type_id("Error");

	if (likely(!_scl_is_instance_of(ex, error_type))) {
		while (stack_top != stack_bottom) {
			if (likely(*stack_top != EXCEPTION_HANDLER_MARKER)) {
				stack_top += iteration_direction;
				continue;
			}

			struct _scl_exception_handler* handler = (struct _scl_exception_handler*) stack_top;
			handler->exception = ex;
			handler->marker = 0;

			longjmp(handler->jmp, 666);
		}
	}

	_scl_runtime_catch(ex);
}

_scl_symbol_hidden static void native_trace(void) {
#if !defined(_WIN32) && !defined(__wasm__)
	void* callstack[1024];
	int frames = backtrace(callstack, 1024);
	write(2, "Backtrace:\n", 11);
	backtrace_symbols_fd(callstack, frames, 2);
	write(2, "\n", 1);
#endif
}

_scl_symbol_hidden static inline void print_stacktrace_of(scl_Exception e) {
	for (scl_int i = e->stackTrace->count - 1; i >= 0; i--) {
		fprintf(stderr, "    %s\n", ((scl_str*) e->stackTrace->values)[i]->data);
	}

	fprintf(stderr, "\n");
	native_trace();
}

_scl_no_return void _scl_runtime_catch(scl_any _ex) {
	scl_Exception ex = (scl_Exception) _ex;
	scl_str msg = ex->msg;

	fprintf(stderr, "Uncaught %s: %s\n", ex->rtFields.statics->type_name, msg->data);
	print_stacktrace_of(ex);
	exit(EX_THROWN);
}

_scl_no_return _scl_symbol_hidden static void* _scl_oom(scl_uint size) {
	fprintf(stderr, "Out of memory! Tried to allocate %lu bytes\n", size);
	native_trace();
	exit(-1);
}

_scl_symbol_hidden static int setupCalled = 0;

_scl_constructor
void _scl_setup(void) {
	if (setupCalled) {
		return;
	}
#if !defined(SCL_KNOWN_LITTLE_ENDIAN)
	// Endian-ness detection
	short word = 0x0001;
	char *byte = (scl_int8*) &word;
	if (!byte[0]) {
		fprintf(stderr, "Invalid byte order detected!");
		exit(-1);
	}
#endif

	GC_INIT();
	GC_set_oom_fn((GC_oom_func) &_scl_oom);

	_scl_set_up_signal_handler();
	GC_allow_register_threads();

	setupCalled = 1;
}

#if defined(__cplusplus)
}
#endif
