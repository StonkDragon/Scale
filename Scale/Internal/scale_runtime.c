#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>

#include "scale_runtime.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define unimplemented(_what) do { fprintf(stderr, "%s:%d: %s: Not Implemented: %s\n", __FILE__, __LINE__, __FUNCTION__, _what); exit(1); } while (0)

typedef struct Struct_SclObject {
	// Typeinfo for this object
	TypeInfo*		type;
} Struct;

typedef Struct* scl_SclObject;

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

typedef struct Struct_InvalidArgumentException {
	struct Struct_Exception self;
}* scl_InvalidArgumentException;

tls scl_int8* thread_name;

void _scl_set_thread_name(scl_int8* name) {
	thread_name = name;
}

scl_int8* _scl_get_thread_name(void) {
	return thread_name;
}

#ifdef _WIN32
int gettimeofday(struct timeval* tp, scl_any tzp) {
	(void) tzp;

#define WIN_EPOCH ((uint64_t) 116444736000000000ULL)

    SYSTEMTIME system_time;
    FILETIME file_time;
    uint64_t time;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    time = ((uint64_t) file_time.dwLowDateTime) + (((uint64_t) file_time.dwHighDateTime) << 32);

    tp->tv_sec  = (long) ((time - WIN_EPOCH) / 10000000L);
    tp->tv_usec = (long) (system_time.wMilliseconds * 1000);
    return 0;
}

size_t getline(char **lineptr, size_t *n, FILE *stream) {
    char *bufptr = NULL;
    char *p = bufptr;
    size_t size;
    int c;

    if (lineptr == NULL) {
        return -1;
    }
    if (stream == NULL) {
        return -1;
    }
    if (n == NULL) {
        return -1;
    }
    bufptr = *lineptr;
    size = *n;

    c = fgetc(stream);
    if (c == EOF) {
        return -1;
    }
    if (bufptr == NULL) {
        bufptr = GC_malloc(128);
        if (bufptr == NULL) {
            return -1;
        }
        size = 128;
    }
    p = bufptr;
    while(c != EOF) {
        if ((p - bufptr) > (size - 1)) {
            size = size + 128;
            bufptr = GC_realloc(bufptr, size);
            if (bufptr == NULL) {
                return -1;
            }
        }
        *p++ = c;
        if (c == '\n') {
            break;
        }
        c = fgetc(stream);
    }

    *p++ = '\0';
    *lineptr = bufptr;
    *n = size;

    return p - bufptr - 1;
}

scl_any Parser$peek(scl_any ptr) {
	static scl_any(*peeker)(scl_any) = nil;
	if (peeker == nil) {
		peeker = (typeof(peeker)) GetProcAddress(GetModuleHandleA(nil), "SclParser$peek");
	}
	return peeker(ptr);
}

scl_any Parser$consume(scl_any ptr) {
	static scl_any(*consumer)(scl_any) = nil;
	if (consumer == nil) {
		consumer = (typeof(consumer)) GetProcAddress(GetModuleHandleA(nil), "SclParser$consume");
	}
	return consumer(ptr);
}

scl_any Parser$getConfig(scl_any ptr) {
	static scl_any(*consumer)(scl_any) = nil;
	if (consumer == nil) {
		consumer = (typeof(consumer)) GetProcAddress(GetModuleHandleA(nil), "SclParser$getConfig");
	}
	return consumer(ptr);
}

scl_any Config$getKey(scl_any ptr, scl_any arg) {
	static scl_any(*consumer)(scl_any, scl_any) = nil;
	if (consumer == nil) {
		consumer = (typeof(consumer)) GetProcAddress(GetModuleHandleA(nil), "SclConfig$getKey");
	}
	return consumer(ptr, arg);
}

scl_any Config$hasKey(scl_any ptr, scl_any arg) {
	static scl_any(*consumer)(scl_any, scl_any) = nil;
	if (consumer == nil) {
		consumer = (typeof(consumer)) GetProcAddress(GetModuleHandleA(nil), "SclConfig$hasKey");
	}
	return consumer(ptr, arg);
}

#endif

static memory_layout_t** static_ptrs = nil;
static scl_int static_ptrs_cap = 0;
static scl_int static_ptrs_count = 0;

scl_any _scl_mark_static(memory_layout_t* layout) {
	if (layout->flags & MEM_FLAG_STATIC) return layout;
	layout->flags |= MEM_FLAG_STATIC;
	GC_alloc_lock();
	for (scl_int i = 0; i < static_ptrs_count; i++) {
		if (static_ptrs[i] == layout) {
			GC_alloc_unlock();
			return layout;
		}
	}
	static_ptrs_count++;
	if (static_ptrs_count >= static_ptrs_cap) {
		static_ptrs_cap = static_ptrs_cap == 0 ? 16 : (static_ptrs_cap * 2);
		static_ptrs = realloc(static_ptrs, static_ptrs_cap * sizeof(memory_layout_t*));
	}
	static_ptrs[static_ptrs_count - 1] = layout;
	GC_alloc_unlock();
	return layout;
}

_scl_symbol_hidden static scl_int _scl_on_stack(scl_any ptr) {
	struct GC_stack_base base = {0};
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
	GC_alloc_lock();
	memory_layout_t* l = nil;
	for (scl_int i = 0; i < static_ptrs_count; i++) {
		if (static_ptrs[i] == ptr) {
			l = ptr;
			break;
		}
	}
	GC_alloc_unlock();
	return l;
}

scl_int _scl_sizeof(scl_any ptr) {
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
		virtual_call(obj, "finalize()V;", void);
	}
	memset(ptr, 0, sizeof(memory_layout_t));
}

_scl_nodiscard _Nonnull
scl_any _scl_alloc(scl_int size) {
	scl_int orig_size = size;
	size = ((size + 7) >> 3) << 3;
	if (unlikely(size == 0)) size = 8;

	if (unlikely(size > SCL_int32_MAX)) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Cannot allocate more than 2147483647 bytes of memory!");
	}

	scl_any ptr = GC_malloc(size + sizeof(memory_layout_t));
	if (unlikely(ptr == nil)) {
		raise(SIGSEGV);
	}
	GC_register_finalizer(ptr, (GC_finalization_proc) _scl_finalize, nil, nil, nil);

	((memory_layout_t*) ptr)->size = orig_size;

	return ptr + sizeof(memory_layout_t);
}

_scl_nodiscard _Nonnull
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

	if (unlikely(size > SCL_int32_MAX)) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Cannot allocate more than 2147483647 bytes of memory!");
	}

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
		GC_free(base);
	}
}

_scl_symbol_hidden static void native_trace(void);

char* vstrformat(const char* fmt, va_list args) {
	size_t len = vsnprintf(nil, 0, fmt, args);
	char* s = _scl_alloc(len + 2);
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
	virtual_call(ex, "init(s;)V;", void, _scl_create_string(str));
	
	_scl_throw(ex);
}

scl_any _scl_cvarargs_to_array(va_list args, scl_int count) {
	scl_any* arr = (scl_any*) _scl_new_array_by_size(count, sizeof(scl_any));
	for (scl_int i = 0; i < count; i++) {
		arr[i] = *va_arg(args, scl_any*);
	}
	return arr;
}

#ifdef _WIN32
const char* strsignal(int sig) {
	static const char* sigs[] = {
		[SIGINT] = "interrupt",
		[SIGILL] = "illegal instruction - invalid function image",
		[SIGFPE] = "floating point exception",
		[SIGSEGV] = "segment violation",
		[SIGTERM] = "Software termination signal from kill",
		[SIGBREAK] = "Ctrl-Break sequence",
		[SIGABRT] = "abnormal termination triggered by abort call",
	};
	const char* s = sigs[sig];
	if (s == nil) {
		return "Unknown signal";
	}
	return s;
}
#endif

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
	virtual_call(sigErr, "init(s;)V;", void, _scl_create_string(signalString));

	handling_signal = 0;
	with_errno = 0;
	_scl_throw(sigErr);
}

void _scl_sleep(scl_int millis) {
	sleep(millis);
}

void builtinUnreachable(void) {
	scl_UnreachableError e = ALLOC(UnreachableError);
	virtual_call(e, "init(s;)V;", void, str_of_exact("Unreachable"));
	_scl_throw(e);
}

scl_int builtinIsInstanceOf(scl_any obj, scl_str type) {
	return _scl_is_instance_of(obj, type_id(type->data));
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

scl_int _scl_identity_hash(scl_any obj) {
	if (unlikely(!_scl_is_instance(obj))) {
		return REINTERPRET_CAST(scl_int, obj);
	}
	scl_int size = _scl_sizeof(obj);
	scl_int hash = REINTERPRET_CAST(scl_int, obj) % 17;
	for (scl_int i = 0; i < size; i++) {
		hash = _scl_rotl(hash, 5) ^ ((scl_int8*) obj)[i];
	}
	return hash;
}

scl_any _scl_atomic_clone(scl_any ptr) {
	if (_scl_is_instance(ptr)) {
		TypeInfo* x = ((Struct*) ptr)->type;
		scl_any clone = _scl_alloc_struct(x);
		memmove(clone, ptr, x->size);
		return clone;
	}
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

static inline _scl_function _scl_get_method_on_type(scl_any type, ID_t method, ID_t signature) {
	const struct TypeInfo* ti = ((Struct*) type)->type;
	const struct _scl_methodinfo* mi = ti->vtable_info;
	const _scl_function* vtable = ti->vtable;

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

scl_any _scl_get_vtable_function(scl_any instance, const scl_int8* methodIdentifier) {
	if (unlikely(!_scl_is_instance(instance))) {
		_scl_runtime_error(EX_CAST_ERROR, "Tried getting method '%s' on non-struct type (%p)", methodIdentifier, instance);
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

	_scl_function m = _scl_get_method_on_type(instance, methodNameHash, signatureHash);
	if (unlikely(m == nil)) {
		if (unlikely(((Struct*) instance)->type == nil)) {
			_scl_runtime_error(EX_BAD_PTR, "instance->type is nil");
		}
		_scl_runtime_error(EX_BAD_PTR, "Method '%s%s' not found on type '%s'", methodName, signature, ((Struct*) instance)->type->type_name);
	}
	return m;
}

_scl_nodiscard _Nonnull
scl_any _scl_init_struct(scl_any ptr, const TypeInfo* statics, memory_layout_t* layout) {
	if (unlikely(layout == nil || ptr == nil)) {
		_scl_runtime_error(EX_BAD_PTR, "Tried to access nil pointer");
	}
	layout->flags |= MEM_FLAG_INSTANCE;
	((Struct*) ptr)->type = (TypeInfo*) statics;
	return ptr;
}

_scl_nodiscard _Nonnull
scl_any _scl_alloc_struct(const TypeInfo* statics) {
	scl_any p = _scl_alloc(statics->size);
	return _scl_init_struct(p, statics, _scl_get_memory_layout(p));
}

scl_int _scl_is_instance(scl_any ptr) {
	if (ptr == nil) return 0;
	memory_layout_t* layout = _scl_get_memory_layout(ptr);
	return (layout != nil) && ((layout->flags & MEM_FLAG_INSTANCE) != 0);
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
#ifdef SIGBREAK
	SIGACTION(SIGBREAK);
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
			virtual_call(e, "init(s;)V;", void, str_of_exact(strformat("Cannot cast nil to type '%s'", target_type_name)));
			_scl_throw(e);
		}

		memory_layout_t* layout = _scl_get_memory_layout(instance);

		scl_bool is_instance = layout && (layout->flags & MEM_FLAG_INSTANCE);
		if (is_instance) {
			virtual_call(e, "init(s;)V;", void, str_of_exact(strformat("Cannot cast instance of struct '%s' to type '%s'\n", ((Struct*) instance)->type->type_name, target_type_name)));
		} else {
			virtual_call(e, "init(s;)V;", void, str_of_exact(strformat("Cannot cast non-object to type '%s'\n", target_type_name)));
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

ID_t _scl_typeid_or_else(scl_any instance, ID_t else_) {
	if (likely(_scl_is_instance(instance))) {
		return ((Struct*) instance)->type->type;
	}
	return else_;
}

scl_any _scl_new_array_by_size(scl_int num_elems, scl_int elem_size) {
	if (unlikely(num_elems < 0)) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array size must not be less than 0");
	}
	if (unlikely(elem_size > 0xFFFFFF)) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Cannot create array where element size exceeds 16777215 bytes!");
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
	_scl_function funcs[];
};

scl_any _scl_migrate_foreign_array(const void* const arr, scl_int num_elems, scl_int elem_size) {
	if (unlikely(num_elems < 0)) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array size must not be less than 0");
	}
	scl_any* new_arr = (scl_any*) _scl_new_array_by_size(num_elems, elem_size);
	memmove(new_arr, arr, num_elems * elem_size);
	return new_arr;
}

scl_int _scl_is_array(scl_any* arr) {
	if (arr == nil) return 0;
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
		virtual_call(e, "init(s;)V;", void, _scl_create_string(strformat("Index " SCL_INT_FMT " out of bounds for array of size " SCL_INT_FMT, index, size)));
		_scl_throw(e);
	}
}

void _scl_array_check_bounds_or_throw(scl_any* arr, scl_int index) {
	if (unlikely(!_scl_is_array(arr))) {
		if (_scl_get_memory_layout(arr) != nil) {
			// points into valid memory so *probably* fine
			return;
		}
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

void _scl_throw(scl_any ex) {
	// scl_bool is_error = _scl_is_instance_of(ex, type_id("Error"));
	// if (is_error) {
	// 	_scl_runtime_catch(ex);
	// }

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
	fprintf(stderr, "Out of memory! Tried to allocate " SCL_UINT_FMT " bytes\n", size);
	native_trace();
	exit(-1);
}

scl_any _scl_thread_start(scl_any func, scl_any args) {
#ifdef _WIN32
	unsigned long thread_id;
	scl_any handle;
	handle = GC_CreateThread(
		NULL,
		0,
		func,
		args,
		0,
		&thread_id // unused
	);
	if (handle == NULL) {
		_scl_runtime_error(EX_THREAD_ERROR, "Failed to create thread");
	}
	return handle;
#else
	pthread_t x;
	if (GC_pthread_create(&x, NULL, func, args) != 0) {
		_scl_runtime_error(EX_THREAD_ERROR, "Failed to create thread");
	}
	return (scl_any) x;
#endif
}

void _scl_thread_finish(scl_any thread) {
#ifdef _WIN32
	TerminateThread(thread, 0);
#else
	if (pthread_join((pthread_t) thread, NULL) != 0) {
		_scl_runtime_error(EX_THREAD_ERROR, "Failed to join thread");
	}
#endif
}

void _scl_thread_detach(scl_any thread) {
#ifdef _WIN32
	unimplemented("Threads can't detach on windows");
#else
	if (pthread_detach((pthread_t) thread) != 0) {
		_scl_runtime_error(EX_THREAD_ERROR, "Failed to detach thread");
	}
#endif
}

struct async_func {
    scl_any thread;
    scl_any ret;
    scl_any args;
    scl_any(*func)(scl_any);
};

_scl_symbol_hidden void _scl_async_runner(struct async_func* args) {
	TRY {
		args->ret = args->func(args->args);
		if (args->args) free(args->args);
	} else {
		if (args->args) free(args->args);
		_scl_runtime_catch(_scl_exception_handler.exception);
	}
}

scl_any _scl_run_async(scl_any func, scl_any func_args) {
	struct async_func* args = malloc(sizeof(struct async_func));
	args->args = func_args;
	args->func = func;
	args->thread = _scl_thread_start(&_scl_async_runner, args);
	return args;
}

scl_any _scl_run_await(scl_any _args) {
	struct async_func* args = (struct async_func*) _args;
	if (args->thread) _scl_thread_finish(args->thread);
	scl_any ret = args->ret;
	free(args);
	return ret;
}

void _scl_yield() {
#ifdef _WIN32
	SwitchToThread();
#else
	sched_yield();
#endif
}

_scl_constructor
void _scl_setup(void) {
	static int setupCalled;
	if (setupCalled) {
		return;
	}
	_scl_set_thread_name("Main Thread");
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

	setupCalled = 1;
}

#if defined(__cplusplus)
}
#endif
