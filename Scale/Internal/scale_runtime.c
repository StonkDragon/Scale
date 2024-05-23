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

typedef struct Struct {
	// Typeinfo for this object
	TypeInfo*		type;
	// Mutex for this object
	scl_any			mutex;
} Struct;

typedef struct Struct_Exception {
	Struct rtFields;
	scl_str msg;
	scl_str* stackTrace;
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
	scl_str* stackTrace;
}* scl_AssertError;
typedef struct Struct_SignalError {
	Struct rtFields;
	scl_str msg;
	scl_str* stackTrace;
}* scl_SignalError;

typedef struct Struct_UnreachableError {
	Struct rtFields;
	scl_str msg;
	scl_str* stackTrace;
}* scl_UnreachableError;

struct Struct_str {
	struct scale_string s;
};

typedef struct Struct_InvalidArgumentException {
	struct Struct_Exception self;
}* scl_InvalidArgumentException;

tls scl_int8* thread_name;

static memory_layout_t** static_ptrs = nil;
static scl_int static_ptrs_cap = 0;
static scl_int static_ptrs_count = 0;
static scl_any lock = nil;

scl_any _scl_mark_static(memory_layout_t* layout) {
	cxx_std_recursive_mutex_lock(&lock);
	for (scl_int i = 0; i < static_ptrs_count; i++) {
		if (static_ptrs[i] == layout) {
			cxx_std_recursive_mutex_unlock(&lock);
			return layout;
		}
	}
	static_ptrs_count++;
	if (static_ptrs_count >= static_ptrs_cap) {
		static_ptrs_cap = static_ptrs_cap == 0 ? 16 : (static_ptrs_cap * 2);
		static_ptrs = realloc(static_ptrs, static_ptrs_cap * sizeof(memory_layout_t*));
	}
	static_ptrs[static_ptrs_count - 1] = layout;
	cxx_std_recursive_mutex_unlock(&lock);
	return layout;
}

_scl_symbol_hidden static scl_int _scl_on_stack(scl_any ptr) {
	struct GC_stack_base base;
	if (GC_get_stack_base(&base) != GC_SUCCESS) {
		return 0;
	}
	if (base.mem_base > (void*) &base) {
		return ptr <= base.mem_base && ptr >= (void*) &base;
	} else {
		return ptr >= base.mem_base && ptr <= (void*) &base;
	}
}

_scl_symbol_hidden static memory_layout_t* _scl_get_memory_layout(scl_any ptr) {
	if (unlikely(ptr == nil)) return nil;
	if (likely(GC_is_heap_ptr(ptr))) {
		return (memory_layout_t*) GC_base(ptr);
	}
	ptr -= sizeof(memory_layout_t);
	if (_scl_on_stack(ptr)) {
		return (memory_layout_t*) ptr;
	}
	if (unlikely(ptr == nil)) return nil;
	cxx_std_recursive_mutex_lock(&lock);
	memory_layout_t* l = nil;
	for (scl_int i = 0; i < static_ptrs_count; i++) {
		if (static_ptrs[i] == ptr) {
			l = ptr;
			break;
		}
	}
	cxx_std_recursive_mutex_unlock(&lock);
	return l;
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

void _scl_finalize(scl_any ptr) {
	ptr = _scl_get_memory_layout(ptr);
	if (ptr && (((memory_layout_t*) ptr)->flags & MEM_FLAG_INSTANCE)) {
		scl_any obj = (scl_any) (ptr + sizeof(memory_layout_t));
		virtual_call(obj, "finalize()V;");
	}
	((memory_layout_t*) ptr)->size = 0;
	((memory_layout_t*) ptr)->flags = 0;
	((memory_layout_t*) ptr)->array_elem_size = 0;
}

scl_any _scl_alloc(scl_int size) {
	scl_int orig_size = size;
	size = ((size + 7) >> 3) << 3;
	if (unlikely(size == 0)) size = 8;

	scl_any ptr = GC_malloc(size + sizeof(memory_layout_t));
	if (unlikely(ptr == nil)) {
		raise(SIGSEGV);
	}
	GC_register_finalizer(ptr, (GC_finalization_proc) _scl_finalize, nil, nil, nil);

	((memory_layout_t*) ptr)->size = orig_size;

	return ptr + sizeof(memory_layout_t);
}

scl_any _scl_realloc(scl_any ptr, scl_int size) {
	if (unlikely(size == 0)) {
		_scl_free(ptr);
		return nil;
	}
	if (unlikely(ptr == nil)) {
		return _scl_alloc(size);
	}

	scl_int orig_size = size;
	size = ((size + 7) >> 3) << 3;
	if (unlikely(size == 0)) size = 8;

	ptr = GC_realloc(ptr - sizeof(memory_layout_t), size + sizeof(memory_layout_t));
	if (unlikely(ptr == nil)) {
		raise(SIGSEGV);
	}
	GC_register_finalizer(ptr, (GC_finalization_proc) _scl_finalize, nil, nil, nil);

	((memory_layout_t*) ptr)->size = orig_size;
	
	return ptr + sizeof(memory_layout_t);
}

void _scl_free(scl_any ptr) {
	if (unlikely(ptr == nil)) return;
	scl_any base = GC_base(ptr);
	if (likely(base)) {
		_scl_finalize(ptr);
		if (_scl_is_instance(ptr)) {
			cxx_std_recursive_mutex_delete(&((Struct*) ptr)->mutex);
		}
		GC_free(base);
	}
}

_scl_symbol_hidden static void native_trace(void);

char* vstrformat(const char* fmt, va_list args) {
	size_t len = vsnprintf(nil, 0, fmt, args);
	char* s = _scl_alloc(len + 1);
	vsnprintf(s, len + 1, fmt, args);
	return s;
}

char* strformat(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	char* str = vstrformat(fmt, args);
	va_end(args);
	return str;
}

_scl_no_return void _scl_runtime_error(int code, const scl_int8* msg, ...) {
	va_list args;
	va_start(args, msg);
	char* str = vstrformat(msg, args);
	va_end(args);

	scl_RuntimeError ex = ALLOC(RuntimeError);
	virtual_call(ex, "init(s;)V;", _scl_create_string(str));
	
	_scl_throw(ex);
}

scl_any _scl_cvarargs_to_array(va_list args, scl_int count) {
	scl_any* arr = (scl_any*) _scl_new_array_by_size(count, sizeof(scl_any));
	for (scl_int i = 0; i < count; i++) {
		arr[i] = *va_arg(args, scl_any*);
	}
	return arr;
}

_scl_symbol_hidden static void _scl_signal_handler(scl_int sig_num) {
	static int handling_signal = 0;
	static int with_errno = 0;
	const scl_int8* signalString = nil;

	if (handling_signal) {
		signalString = strsignal(handling_signal);
		fprintf(stderr, "\n");

		fprintf(stderr, "Signal: %s\n", signalString);
		if (errno) {
			fprintf(stderr, "errno: %s\n", strerror(with_errno));
		}
		native_trace();

		exit(handling_signal);
	}

	signalString = strsignal(sig_num);
	handling_signal = sig_num;
	with_errno = errno;

	scl_SignalError sigErr = ALLOC(SignalError);
	virtual_call(sigErr, "init(s;)V;", _scl_create_string(signalString));

	handling_signal = 0;
	with_errno = 0;
	_scl_throw(sigErr);
}

void _scl_sleep(scl_int millis) {
	sleep(millis);
}

void _scl_lock(scl_any obj) {
	if (unlikely(!_scl_is_instance(obj))) {
		return;
	}
	cxx_std_recursive_mutex_lock(&((Struct*) obj)->mutex);
}

void _scl_unlock(scl_any obj) {
	if (unlikely(!_scl_is_instance(obj))) {
		return;
	}
	cxx_std_recursive_mutex_unlock(&((Struct*) obj)->mutex);
}

void _scl_assert(scl_int b, const scl_int8* msg, ...) {
	if (unlikely(!b)) {
		scl_AssertError e = ALLOC(AssertError);
		va_list list;
		va_start(list, msg);
		virtual_call(e, "init(s;)V;", str_of_exact(strformat("Assertion failed: %s", vstrformat(msg, list))));
		va_end(list);
		_scl_throw(e);
	}
}

void builtinUnreachable(void) {
	scl_UnreachableError e = ALLOC(UnreachableError);
	virtual_call(e, "init(s;)V;", str_of_exact("Unreachable"));
	_scl_throw(e);
}

scl_int builtinIsInstanceOf(scl_any obj, scl_str type) {
	return _scl_is_instance_of(obj, type->hash);
}

#if !defined(__pure2)
#define __pure2
#endif

__pure2 ID_t type_id(const scl_int8* data) {
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
	cxx_std_recursive_mutex_lock(&obj->mutex);
	scl_int size = _scl_sizeof(obj);
	scl_int hash = REINTERPRET_CAST(scl_int, obj) % 17;
	for (scl_int i = 0; i < size; i++) {
		hash = _scl_rotl(hash, 5) ^ ((scl_int8*) obj)[i];
	}
	cxx_std_recursive_mutex_unlock(&obj->mutex);
	return hash;
}

scl_any _scl_atomic_clone(scl_any ptr) {
	scl_int size = _scl_sizeof(ptr);
	scl_any clone = _scl_alloc(size);

	memmove(clone, ptr, size);
	memory_layout_t* src = _scl_get_memory_layout(ptr);
	memory_layout_t* dest = _scl_get_memory_layout(clone);
	memmove(dest, src, sizeof(memory_layout_t));
	
	return clone;
}

scl_any _scl_copy_fields(scl_any dest, scl_any src, scl_int size) {
	size -= sizeof(struct Struct);
	if (size == 0) return dest;
	memmove(dest + sizeof(struct Struct), src + sizeof(struct Struct), size);
	return dest;
}

_scl_symbol_hidden static scl_int _scl_search_method_index(const struct _scl_methodinfo* const methods, ID_t id, ID_t sig);

static inline _scl_lambda _scl_get_method_on_type(scl_any type, ID_t method, ID_t signature, int onSuper) {
	const struct TypeInfo* ti;
	if (likely(!onSuper)) {
		ti = ((Struct*) type)->type;
	} else {
		ti = ((Struct*) type)->type->super;
	}
	const struct _scl_methodinfo* mi = ti->vtable_info;
	const _scl_lambda* vtable = ti->vtable;

	scl_int index = _scl_search_method_index(mi, method, signature);
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
	if (unlikely(!_scl_is_instance(instance))) {
		_scl_runtime_error(EX_CAST_ERROR, "Tried getting method on non-struct type (%p)", instance);
	}

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
		if (unlikely(((Struct*) instance)->type == nil)) {
			_scl_runtime_error(EX_BAD_PTR, "instance->type is nil");
		}
		_scl_runtime_error(EX_BAD_PTR, "Method '%s%s' not found on type '%s'", methodName, signature, ((Struct*) instance)->type->type_name);
	}
	return m;
}

scl_any _scl_init_struct(scl_any ptr, const TypeInfo* statics, memory_layout_t* layout) {
	if (unlikely(layout == nil || ptr == nil)) {
		_scl_runtime_error(EX_BAD_PTR, "Tried to access nil pointer");
	}
	layout->flags |= MEM_FLAG_INSTANCE;
	((Struct*) ptr)->type = (TypeInfo*) statics;
	return ptr;
}

scl_any _scl_alloc_struct(const TypeInfo* statics) {
	scl_any p = _scl_alloc(statics->size);
	return _scl_init_struct(p, statics, _scl_get_memory_layout(p));
}

scl_int _scl_is_instance(scl_any ptr) {
	memory_layout_t* layout = _scl_get_memory_layout(ptr);
	return layout && (layout->flags & MEM_FLAG_INSTANCE);
}

scl_int _scl_is_instance_of(scl_any ptr, ID_t type_id) {
	if (unlikely(((scl_int) ptr) <= 0)) return 0;
	if (unlikely(!_scl_is_instance(ptr))) return 0;

	Struct* ptrStruct = (Struct*) ptr;

	for (const TypeInfo* super = ptrStruct->type; super != nil; super = super->super) {
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
#define SIGACTION(_sig) signal(_sig, (void(*)(int)) _scl_signal_handler)
	
#ifdef SIGINT
	SIGACTION(SIGINT);
#endif
#ifdef SIGILL
	SIGACTION(SIGILL);
#endif
#ifdef SIGTRAP
	SIGACTION(SIGTRAP);
#endif
#ifdef SIGABRT
	SIGACTION(SIGABRT);
#endif
#ifdef SIGBUS
	SIGACTION(SIGBUS);
#endif
#ifdef SIGSEGV
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
			scl_str* stackTrace;
		}* scl_CastError;

		scl_CastError e = ALLOC(CastError);
		if (instance == nil) {
			virtual_call(e, "init(s;)V;", str_of_exact(strformat("Cannot cast nil to type '%s'", target_type_name)));
			_scl_throw(e);
		}

		memory_layout_t* layout = _scl_get_memory_layout(instance);

		scl_bool is_instance = layout && (layout->flags & MEM_FLAG_INSTANCE);
		if (is_instance) {
			virtual_call(e, "init(s;)V;", str_of_exact(strformat("Cannot cast instance of struct '%s' to type '%s'\n", ((Struct*) instance)->type->type_name, target_type_name)));
		} else {
			virtual_call(e, "init(s;)V;", str_of_exact(strformat("Cannot cast non-object to type '%s'\n", target_type_name)));
		}
		_scl_throw(e);
	}
	return instance;
}

scl_int8* _scl_typename_or_else(scl_any instance, const scl_int8* else_) {
	if (likely(_scl_is_instance(instance))) {
		return (scl_int8*) ((Struct*) instance)->type->type_name;
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
	layout->flags |= MEM_FLAG_ARRAY;
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
	memmove(new_arr, arr, num_elems * elem_size);
	return new_arr;
}

scl_int _scl_is_array(scl_any* arr) {
	memory_layout_t* layout = _scl_get_memory_layout(arr);
	return layout && (layout->flags & MEM_FLAG_ARRAY);
}

scl_any* _scl_multi_new_array_by_size(scl_int dimensions, scl_int sizes[], scl_int elem_size) {
	if (unlikely(dimensions < 1)) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array dimensions must not be less than 1");
	}
	if (dimensions == 1) {
		return (scl_any*) _scl_new_array_by_size(sizes[0], elem_size);
	}
	scl_any* arr = (scl_any*) _scl_new_array_by_size(sizes[0], sizeof(scl_any));
	for (scl_int i = 0; i < sizes[0]; i++) {
		arr[i] = _scl_multi_new_array_by_size(dimensions - 1, &(sizes[1]), elem_size);
	}
	return arr;
}

scl_int _scl_array_size_unchecked(scl_any* arr) {
	memory_layout_t lay = *_scl_get_memory_layout(arr);
	return lay.size / lay.array_elem_size;
}

scl_int _scl_array_elem_size_unchecked(scl_any* arr) {
	return _scl_get_memory_layout(arr)->array_elem_size;
}

scl_int _scl_array_size(scl_any* arr) {
	if (unlikely(!_scl_is_array(arr))) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array must be initialized with 'new[]'");
	}
	return _scl_array_size_unchecked(arr);
}

scl_int _scl_array_elem_size(scl_any* arr) {
	if (unlikely(!_scl_is_array(arr))) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array must be initialized with 'new[]'");
	}
	return _scl_array_elem_size_unchecked(arr);
}

void _scl_array_check_bounds_or_throw_unchecked(scl_any* arr, scl_int index) {
	scl_int size = _scl_array_size_unchecked(arr);
	if (index < 0 || index >= size) {
		scl_IndexOutOfBoundsException e = ALLOC(IndexOutOfBoundsException);
		virtual_call(e, "init(s;)V;", _scl_create_string(strformat("Index " SCL_INT_FMT " out of bounds for array of size " SCL_INT_FMT, index, size)));
		_scl_throw(e);
	}
}

void _scl_array_check_bounds_or_throw(scl_any* arr, scl_int index) {
	if (unlikely(!_scl_is_array(arr))) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array must be initialized with 'new[]'");
	}
	_scl_array_check_bounds_or_throw_unchecked(arr, index);
}

scl_any* _scl_array_resize(scl_int new_size, scl_any* arr) {
	if (unlikely(!_scl_is_array(arr))) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array must be initialized with 'new[]'");
	}
	if (unlikely(new_size < 0)) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array size must not be negative");
	}
	memory_layout_t* l = _scl_get_memory_layout(arr);
	scl_int elem_size = l->array_elem_size;
	scl_any* new_arr;
	if (_scl_on_stack(arr)) {
		if (new_size > (l->size / l->array_elem_size)) {
			new_arr = (scl_any*) _scl_alloc(new_size * elem_size);
			memcpy(new_arr, arr, l->size);
		} else {
			new_arr = arr; // if the stack array is smaller, don't actually do anything
		}
	} else {
	 	new_arr = (scl_any*) _scl_realloc(arr, new_size * elem_size);
	}
	memory_layout_t* layout = _scl_get_memory_layout(new_arr);
	layout->flags |= MEM_FLAG_ARRAY;
	layout->array_elem_size = elem_size;
	return new_arr;
}

void _scl_trace_remove(struct _scl_backtrace* bt) {
	bt->marker = 0;
}

void _scl_unlock_ptr(void* lock_ptr) {
	cxx_std_recursive_mutex_unlock(&(*(Struct**) lock_ptr)->mutex);
}

void _scl_delete_ptr(void* ptr) {
	_scl_free(*(scl_any*) ptr);
}

void _scl_throw(scl_any ex) {
	scl_bool is_error = _scl_is_instance_of(ex, type_id("Error"));
	if (is_error) {
		_scl_runtime_catch(ex);
	}

	scl_uint* stack_top = (scl_uint*) &stack_top;
	struct GC_stack_base sb;
	GC_get_my_stackbottom(&sb);
	scl_uint* stack_bottom = sb.mem_base;

	scl_int iteration_direction = stack_top < stack_bottom ? 1 : -1;
	while (stack_top != stack_bottom) {
		if (*stack_top != EXCEPTION_HANDLER_MARKER) {
			stack_top += iteration_direction;
			continue;
		}

		struct _scl_exception_handler* handler = (struct _scl_exception_handler*) stack_top;

		if (handler->finalizer) {
			handler->finalizer(handler->finalization_data);
		}

		handler->exception = ex;
		handler->marker = 0;

		longjmp(handler->jmp, 666);
	}

	_scl_runtime_catch(ex);
}

_scl_symbol_hidden static void native_trace(void) {
#if !defined(_WIN32) && !defined(__wasm__)
	void* callstack[1024];
	int frames = backtrace(callstack, 1024);
	int write(int, char*, size_t);
	write(2, "Backtrace:\n", 11);
	backtrace_symbols_fd(callstack, frames, 2);
	write(2, "\n", 1);
#endif
}

_scl_symbol_hidden static inline void print_stacktrace_of(scl_Exception e) {
	#ifndef _WIN32
	int write(int, char*, size_t);
	#else
	#define write _write
	#endif
	const char* do_print = getenv("SCL_BACKTRACE");
	if (do_print) {
		if (strcmp(do_print, "full") == 0) {
			native_trace();
		} else if (atoi(do_print) == 1) {
			if (_scl_is_array((scl_any*) e->stackTrace)) {
				for (scl_int i = 0; i < _scl_array_size_unchecked((scl_any*) e->stackTrace); i++) {
					fprintf(stderr, "    %s\n", e->stackTrace[i]->data);
				}
				fprintf(stderr, "Some information was omitted. Run with 'SCL_BACKTRACE=full' environment variable set to display full information\n");
			}
		}
	} else {
		fprintf(stderr, "Run with 'SCL_BACKTRACE=1' environment variable set to display a backtrace\n");
	}
	#ifdef _WIN32
	#undef write
	#endif
}

_scl_no_return void _scl_runtime_catch(scl_any _ex) {
	scl_Exception ex = (scl_Exception) _ex;
	scl_str msg = ex->msg;

	fprintf(stderr, "Unexpected %s", ex->rtFields.type->type_name);
	if (thread_name) {
		fprintf(stderr, " in '%s'", thread_name);
	}
	fprintf(stderr, ": %s\n", msg->data);	
	
	print_stacktrace_of(ex);
	exit(EX_THROWN);
}

_scl_no_return _scl_symbol_hidden static void* _scl_oom(scl_uint size) {
	fprintf(stderr, "Out of memory! Tried to allocate %lu bytes\n", size);
	native_trace();
	exit(-1);
}

_scl_constructor
void _scl_setup(void) {
	static int setupCalled;
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
