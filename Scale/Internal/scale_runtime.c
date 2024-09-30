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

typedef Struct* scale_SclObject;

typedef struct Struct_Exception {
	Struct rtFields;
	scale_str msg;
	scale_str* stackTrace;
}* scale_Exception;

typedef struct Struct_RuntimeError {
	struct Struct_Exception self;
}* scale_RuntimeError;

typedef struct Struct_NullPointerException {
	struct Struct_Exception self;
}* scale_NullPointerException;

struct Struct_Array {
	Struct rtFields;
	scale_any* values;
	scale_int count;
	scale_int capacity;
};

typedef struct Struct_IndexOutOfBoundsException {
	struct Struct_Exception self;
}* scale_IndexOutOfBoundsException;

typedef struct Struct_AssertError {
	Struct rtFields;
	scale_str msg;
	scale_str* stackTrace;
}* scale_AssertError;

typedef struct Struct_SignalError {
	Struct rtFields;
	scale_str msg;
	scale_str* stackTrace;
}* scale_SignalError;

typedef struct Struct_UnreachableError {
	Struct rtFields;
	scale_str msg;
	scale_str* stackTrace;
}* scale_UnreachableError;

typedef struct Struct_InvalidArgumentException {
	struct Struct_Exception self;
}* scale_InvalidArgumentException;

tls scale_int8* thread_name;

void scale_set_thread_name(scale_int8* name) {
	thread_name = name;
}

scale_int8* scale_get_thread_name(void) {
	return thread_name;
}

#ifdef _WIN32
int gettimeofday(struct timeval* tp, scale_any tzp) {
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

scale_any Parser$peek(scale_any ptr) {
	static scale_any(*peeker)(scale_any) = nil;
	if (peeker == nil) {
		peeker = (typeof(peeker)) GetProcAddress(GetModuleHandleA(nil), "SclParser$peek");
	}
	return peeker(ptr);
}

scale_any Parser$consume(scale_any ptr) {
	static scale_any(*consumer)(scale_any) = nil;
	if (consumer == nil) {
		consumer = (typeof(consumer)) GetProcAddress(GetModuleHandleA(nil), "SclParser$consume");
	}
	return consumer(ptr);
}

scale_any Parser$getConfig(scale_any ptr) {
	static scale_any(*consumer)(scale_any) = nil;
	if (consumer == nil) {
		consumer = (typeof(consumer)) GetProcAddress(GetModuleHandleA(nil), "SclParser$getConfig");
	}
	return consumer(ptr);
}

scale_any Config$getKey(scale_any ptr, scale_any arg) {
	static scale_any(*consumer)(scale_any, scale_any) = nil;
	if (consumer == nil) {
		consumer = (typeof(consumer)) GetProcAddress(GetModuleHandleA(nil), "SclConfig$getKey");
	}
	return consumer(ptr, arg);
}

scale_any Config$hasKey(scale_any ptr, scale_any arg) {
	static scale_any(*consumer)(scale_any, scale_any) = nil;
	if (consumer == nil) {
		consumer = (typeof(consumer)) GetProcAddress(GetModuleHandleA(nil), "SclConfig$hasKey");
	}
	return consumer(ptr, arg);
}

#endif

static memory_layout_t** static_ptrs = nil;
static scale_int static_ptrs_cap = 0;
static scale_int static_ptrs_count = 0;

scale_any scale_mark_static(memory_layout_t* layout) {
	if (layout->flags & MEM_FLAG_STATIC) return layout;
	layout->flags |= MEM_FLAG_STATIC;
	GC_alloc_lock();
	for (scale_int i = 0; i < static_ptrs_count; i++) {
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

scale_symbol_hidden static scale_int scale_on_stack(scale_any ptr) {
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

scale_symbol_hidden static memory_layout_t* scale_get_memory_layout(scale_any ptr) {
	if (unlikely(ptr == nil)) return nil;
	if (likely(GC_is_heap_ptr(ptr))) {
		return (memory_layout_t*) GC_base(ptr);
	}
	ptr -= sizeof(memory_layout_t);
	if (scale_on_stack(ptr)) {
		return (memory_layout_t*) ptr;
	}
	GC_alloc_lock();
	memory_layout_t* l = nil;
	for (scale_int i = 0; i < static_ptrs_count; i++) {
		if (static_ptrs[i] == ptr) {
			l = ptr;
			break;
		}
	}
	GC_alloc_unlock();
	return l;
}

scale_int scale_sizeof(scale_any ptr) {
	memory_layout_t* layout = scale_get_memory_layout(ptr);
	if (unlikely(layout == nil)) {
		return 0;
	}
	return layout->size;
}

void scale_finalize(scale_any ptr) {
	ptr = scale_get_memory_layout(ptr);
	if (ptr && (((memory_layout_t*) ptr)->flags & MEM_FLAG_INSTANCE)) {
		scale_any obj = (scale_any) (ptr + sizeof(memory_layout_t));
		virtual_call(obj, "deinit()V;", void);
	}
	memset(ptr, 0, sizeof(memory_layout_t));
}

scale_nodiscard _Nonnull
scale_any scale_alloc(scale_int size) {
	scale_int orig_size = size;
	size = ((size + 7) >> 3) << 3;
	if (unlikely(size == 0)) size = 8;

	if (unlikely(size > SCALE_int32_MAX)) {
		scale_runtime_error(EX_INVALID_ARGUMENT, "Cannot allocate more than 2147483647 bytes of memory!");
	}

	scale_any ptr = GC_malloc(size + sizeof(memory_layout_t));
	if (unlikely(ptr == nil)) {
		raise(SIGSEGV);
	}
	GC_register_finalizer(ptr, (GC_finalization_proc) scale_finalize, nil, nil, nil);

	((memory_layout_t*) ptr)->size = orig_size;

	return ptr + sizeof(memory_layout_t);
}

scale_nodiscard _Nonnull
scale_any scale_realloc(scale_any ptr, scale_int size) {
	if (unlikely(size == 0)) {
		scale_free(ptr);
		return nil;
	}
	if (unlikely(ptr == nil)) {
		return scale_alloc(size);
	}

	scale_int orig_size = size;
	size = ((size + 7) >> 3) << 3;
	if (unlikely(size == 0)) size = 8;

	if (unlikely(size > SCALE_int32_MAX)) {
		scale_runtime_error(EX_INVALID_ARGUMENT, "Cannot allocate more than 2147483647 bytes of memory!");
	}

	ptr = GC_realloc(ptr - sizeof(memory_layout_t), size + sizeof(memory_layout_t));
	if (unlikely(ptr == nil)) {
		raise(SIGSEGV);
	}
	GC_register_finalizer(ptr, (GC_finalization_proc) scale_finalize, nil, nil, nil);

	((memory_layout_t*) ptr)->size = orig_size;
	
	return ptr + sizeof(memory_layout_t);
}

void scale_free(scale_any ptr) {
	if (unlikely(ptr == nil)) return;
	scale_any base = GC_base(ptr);
	if (likely(base)) {
		scale_finalize(ptr);
		GC_free(base);
	}
}

scale_symbol_hidden static void native_trace(void);

char* vstrformat(const char* fmt, va_list args) {
	size_t len = vsnprintf(nil, 0, fmt, args);
	char* s = scale_alloc(len + 2);
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

scale_no_return void scale_runtime_error(int code, const scale_int8* msg, ...) {
	va_list args;
	va_start(args, msg);
	char* str = vstrformat(msg, args);
	va_end(args);

	scale_RuntimeError ex = ALLOC(RuntimeError);
	virtual_call(ex, "init(s;)V;", void, scale_create_string(str));
	
	scale_throw(ex);
}

scale_any scale_cvarargs_to_array(va_list args, scale_int count) {
	scale_uint64* arr = (scale_uint64*) scale_new_array_by_size(count, sizeof(scale_uint64));
	for (scale_int i = 0; i < count; i++) {
		arr[i] = va_arg(args, scale_uint64);
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

scale_symbol_hidden static void scale_signal_handler(scale_int sig_num) {
	static int handling_signal = 0;
	static int with_errno = 0;
	const scale_int8* signalString = nil;

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

	scale_SignalError sigErr = ALLOC(SignalError);
	virtual_call(sigErr, "init(s;)V;", void, scale_create_string(signalString));

	handling_signal = 0;
	with_errno = 0;
	scale_throw(sigErr);
}

void scale_sleep(scale_int millis) {
	sleep(millis);
}

void builtinUnreachable(void) {
	scale_UnreachableError e = ALLOC(UnreachableError);
	virtual_call(e, "init(s;)V;", void, str_of_exact("Unreachable"));
	scale_throw(e);
}

scale_int builtinIsInstanceOf(scale_any obj, scale_str type) {
	return scale_is_instance_of(obj, type_id(type->data));
}

#if !defined(__pure2)
#define __pure2
#endif

__pure2 ID_t type_id(const scale_int8* data) {
	ID_t h = 3323198485UL;
	for (;*data;++data) {
		h ^= *data;
		h *= 0x5BD1E995;
		h ^= h >> 15;
	}
	return h;
}

__pure2 scale_symbol_hidden static scale_uint scale_rotl(const scale_uint value, const scale_int shift) {
    return (value << shift) | (value >> ((sizeof(scale_uint) << 3) - shift));
}

__pure2 scale_symbol_hidden static scale_uint scale_rotr(const scale_uint value, const scale_int shift) {
    return (value >> shift) | (value << ((sizeof(scale_uint) << 3) - shift));
}

scale_int scale_identity_hash(scale_any obj) {
	if (unlikely(!scale_is_instance(obj))) {
		return REINTERPRET_CAST(scale_int, obj);
	}
	scale_int size = scale_sizeof(obj);
	scale_int hash = REINTERPRET_CAST(scale_int, obj) % 17;
	for (scale_int i = 0; i < size; i++) {
		hash = scale_rotl(hash, 5) ^ ((scale_int8*) obj)[i];
	}
	return hash;
}

scale_any scale_atomic_clone(scale_any ptr) {
	if (scale_is_instance(ptr)) {
		TypeInfo* x = ((Struct*) ptr)->type;
		scale_any clone = scale_alloc_struct(x);
		memmove(clone, ptr, x->size);
		return clone;
	}
	scale_int size = scale_sizeof(ptr);
	scale_any clone = scale_alloc(size);

	memmove(clone, ptr, size);
	memory_layout_t* src = scale_get_memory_layout(ptr);
	memory_layout_t* dest = scale_get_memory_layout(clone);
	memmove(dest, src, sizeof(memory_layout_t));
	
	return clone;
}

scale_any scale_copy_fields(scale_any dest, scale_any src, scale_int size) {
	size -= sizeof(struct Struct_SclObject);
	if (size == 0) return dest;
	memmove(dest + sizeof(struct Struct_SclObject), src + sizeof(struct Struct_SclObject), size);
	return dest;
}

scale_symbol_hidden static scale_int scale_search_method_index(const struct scale_methodinfo* const methods, ID_t id, ID_t sig);

static inline scale_function scale_get_method_on_type(scale_any type, ID_t method, ID_t signature) {
	const struct TypeInfo* ti = ((Struct*) type)->type;
	const struct scale_methodinfo* mi = ti->vtable_info;
	const scale_function* vtable = ti->vtable;

	scale_int index = scale_search_method_index(mi, method, signature);
	return index >= 0 ? vtable[index] : nil;
}

scale_symbol_hidden static size_t str_index_of_or(const scale_int8* str, char c, size_t len) {
	size_t i = 0;
	while (str[i] != c) {
		if (str[i] == '\0') return len;
		i++;
	}
	return i;
}

scale_symbol_hidden static size_t str_index_of(const scale_int8* str, char c) {
	return str_index_of_or(str, c, -1);
}

scale_symbol_hidden static void split_at(const scale_int8* str, size_t len, size_t index, scale_int8* left, scale_int8* right) {
	strncpy(left, str, index);
	left[index] = '\0';

	strncpy(right, str + index, len - index);
	right[len - index] = '\0';
}

scale_any scale_get_vtable_function(scale_any instance, const scale_int8* methodIdentifier) {
	if (unlikely(!scale_is_instance(instance))) {
		scale_runtime_error(EX_CAST_ERROR, "Tried getting method '%s' on non-struct type (%p)", methodIdentifier, instance);
	}

	size_t methodLen = strlen(methodIdentifier);
	ID_t signatureHash;
	size_t methodNameLen = str_index_of_or(methodIdentifier, '(', methodLen);
	scale_int8 methodName[methodNameLen];
	scale_int8 signature[methodLen - methodNameLen + 1];
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

	scale_function m = scale_get_method_on_type(instance, methodNameHash, signatureHash);
	if (unlikely(m == nil)) {
		if (unlikely(((Struct*) instance)->type == nil)) {
			scale_runtime_error(EX_BAD_PTR, "instance->type is nil");
		}
		scale_runtime_error(EX_BAD_PTR, "Method '%s%s' not found on type '%s'", methodName, signature, ((Struct*) instance)->type->type_name);
	}
	return m;
}

scale_nodiscard _Nonnull
scale_any scale_init_struct(scale_any ptr, const TypeInfo* statics, memory_layout_t* layout) {
	if (unlikely(layout == nil || ptr == nil)) {
		scale_runtime_error(EX_BAD_PTR, "Tried to access nil pointer");
	}
	layout->flags |= MEM_FLAG_INSTANCE;
	((Struct*) ptr)->type = (TypeInfo*) statics;
	return ptr;
}

scale_nodiscard _Nonnull
scale_any scale_alloc_struct(const TypeInfo* statics) {
	scale_any p = scale_alloc(statics->size);
	return scale_init_struct(p, statics, scale_get_memory_layout(p));
}

scale_int scale_is_instance(scale_any ptr) {
	if (ptr == nil) return 0;
	memory_layout_t* layout = scale_get_memory_layout(ptr);
	return (layout != nil) && ((layout->flags & MEM_FLAG_INSTANCE) != 0);
}

scale_int scale_is_instance_of(scale_any ptr, ID_t type_id) {
	if (unlikely(((scale_int) ptr) <= 0)) return 0;
	if (unlikely(!scale_is_instance(ptr))) return 0;

	Struct* ptrStruct = (Struct*) ptr;

	for (const TypeInfo* super = ptrStruct->type; super != nil; super = super->super) {
		if (super->type == type_id) return 1;
	}

	return 0;
}

scale_symbol_hidden static scale_int scale_search_method_index(const struct scale_methodinfo* const methods, ID_t id, ID_t sig) {
	if (unlikely(methods == nil)) return -1;

	for (scale_int i = 0; *(scale_int*) &(methods[i].pure_name); i++) {
		if (methods[i].pure_name == id && (sig == 0 || methods[i].signature == sig)) {
			return i;
		}
	}
	return -1;
}

scale_symbol_hidden static void scale_set_up_signal_handler(void) {
#define SIGACTION(_sig) signal(_sig, (void(*)(int)) scale_signal_handler)
	
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

scale_any scale_get_stderr() {
	return (scale_any) stderr;
}

scale_any scale_get_stdout() {
	return (scale_any) stdout;
}

scale_any scale_get_stdin() {
	return (scale_any) stdin;
}

scale_any scale_checked_cast(scale_any instance, ID_t target_type, const scale_int8* target_type_name) {
	if (unlikely(!scale_is_instance_of(instance, target_type))) {
		typedef struct Struct_CastError {
			Struct rtFields;
			scale_str msg;
			scale_str* stackTrace;
		}* scale_CastError;

		scale_CastError e = ALLOC(CastError);
		if (instance == nil) {
			virtual_call(e, "init(s;)V;", void, str_of_exact(strformat("Cannot cast nil to type '%s'", target_type_name)));
			scale_throw(e);
		}

		memory_layout_t* layout = scale_get_memory_layout(instance);

		scale_bool is_instance = layout && (layout->flags & MEM_FLAG_INSTANCE);
		if (is_instance) {
			virtual_call(e, "init(s;)V;", void, str_of_exact(strformat("Cannot cast instance of struct '%s' to type '%s'\n", ((Struct*) instance)->type->type_name, target_type_name)));
		} else {
			virtual_call(e, "init(s;)V;", void, str_of_exact(strformat("Cannot cast non-object to type '%s'\n", target_type_name)));
		}
		scale_throw(e);
	}
	return instance;
}

scale_int8* scale_typename_or_else(scale_any instance, const scale_int8* else_) {
	if (likely(scale_is_instance(instance))) {
		return (scale_int8*) ((Struct*) instance)->type->type_name;
	}
	return (scale_int8*) else_;
}

ID_t scale_typeid_or_else(scale_any instance, ID_t else_) {
	if (likely(scale_is_instance(instance))) {
		return ((Struct*) instance)->type->type;
	}
	return else_;
}

scale_any scale_new_array_by_size(scale_int num_elems, scale_int elem_size) {
	if (unlikely(num_elems < 0)) {
		scale_runtime_error(EX_INVALID_ARGUMENT, "Array size must not be less than 0");
	}
	if (unlikely(elem_size > 0xFFFFFF)) {
		scale_runtime_error(EX_INVALID_ARGUMENT, "Cannot create array where element size exceeds 16777215 bytes!");
	}
	scale_int size = num_elems * elem_size;
	scale_any arr = scale_alloc(size);
	memory_layout_t* layout = scale_get_memory_layout(arr);
	layout->flags |= MEM_FLAG_ARRAY;
	layout->array_elem_size = elem_size;
	return arr;
}

struct vtable {
	memory_layout_t memory_layout;
	scale_function funcs[];
};

scale_any scale_migrate_foreign_array(const void* const arr, scale_int num_elems, scale_int elem_size) {
	if (unlikely(num_elems < 0)) {
		scale_runtime_error(EX_INVALID_ARGUMENT, "Array size must not be less than 0");
	}
	scale_any* new_arr = (scale_any*) scale_new_array_by_size(num_elems, elem_size);
	memmove(new_arr, arr, num_elems * elem_size);
	return new_arr;
}

scale_int scale_is_array(scale_any* arr) {
	if (arr == nil) return 0;
	memory_layout_t* layout = scale_get_memory_layout(arr);
	return layout && (layout->flags & MEM_FLAG_ARRAY);
}

scale_any* scale_multi_new_array_by_size(scale_int dimensions, scale_int sizes[], scale_int elem_size) {
	if (unlikely(dimensions < 1)) {
		scale_runtime_error(EX_INVALID_ARGUMENT, "Array dimensions must not be less than 1");
	}
	if (dimensions == 1) {
		return (scale_any*) scale_new_array_by_size(sizes[0], elem_size);
	}
	scale_any* arr = (scale_any*) scale_new_array_by_size(sizes[0], sizeof(scale_any));
	for (scale_int i = 0; i < sizes[0]; i++) {
		arr[i] = scale_multi_new_array_by_size(dimensions - 1, &(sizes[1]), elem_size);
	}
	return arr;
}

scale_int scale_array_size_unchecked(scale_any* arr) {
	memory_layout_t lay = *scale_get_memory_layout(arr);
	return lay.size / lay.array_elem_size;
}

scale_int scale_array_elem_size_unchecked(scale_any* arr) {
	return scale_get_memory_layout(arr)->array_elem_size;
}

scale_int scale_array_size(scale_any* arr) {
	if (unlikely(!scale_is_array(arr))) {
		scale_runtime_error(EX_INVALID_ARGUMENT, "Array must be initialized with 'new[]'");
	}
	return scale_array_size_unchecked(arr);
}

scale_int scale_array_elem_size(scale_any* arr) {
	if (unlikely(!scale_is_array(arr))) {
		scale_runtime_error(EX_INVALID_ARGUMENT, "Array must be initialized with 'new[]'");
	}
	return scale_array_elem_size_unchecked(arr);
}

void scale_array_check_bounds_or_throw_unchecked(scale_any* arr, scale_int index) {
	scale_int size = scale_array_size_unchecked(arr);
	if (index < 0 || index >= size) {
		scale_IndexOutOfBoundsException e = ALLOC(IndexOutOfBoundsException);
		virtual_call(e, "init(s;)V;", void, scale_create_string(strformat("Index " SCALE_INT_FMT " out of bounds for array of size " SCALE_INT_FMT, index, size)));
		scale_throw(e);
	}
}

void scale_array_check_bounds_or_throw(scale_any* arr, scale_int index) {
	if (unlikely(!scale_is_array(arr))) {
		if (scale_get_memory_layout(arr) != nil) {
			// points into valid memory so *probably* fine
			return;
		}
		scale_runtime_error(EX_INVALID_ARGUMENT, "Array must be initialized with 'new[]'");
	}
	scale_array_check_bounds_or_throw_unchecked(arr, index);
}

scale_any* scale_array_resize(scale_int new_size, scale_any* arr) {
	if (unlikely(!scale_is_array(arr))) {
		scale_runtime_error(EX_INVALID_ARGUMENT, "Array must be initialized with 'new[]'");
	}
	if (unlikely(new_size < 0)) {
		scale_runtime_error(EX_INVALID_ARGUMENT, "Array size must not be negative");
	}
	memory_layout_t* l = scale_get_memory_layout(arr);
	scale_int elem_size = l->array_elem_size;
	scale_any* new_arr;
	if (scale_on_stack(arr)) {
		if (new_size > (l->size / l->array_elem_size)) {
			new_arr = (scale_any*) scale_alloc(new_size * elem_size);
			memcpy(new_arr, arr, l->size);
		} else {
			new_arr = arr; // if the stack array is smaller, don't actually do anything
		}
	} else {
	 	new_arr = (scale_any*) scale_realloc(arr, new_size * elem_size);
	}
	memory_layout_t* layout = scale_get_memory_layout(new_arr);
	layout->flags |= MEM_FLAG_ARRAY;
	layout->array_elem_size = elem_size;
	return new_arr;
}

void scale_trace_remove(struct scale_backtrace* bt) {
	bt->marker = 0;
}

void scale_throw(scale_any ex) {
	// scale_bool is_error = scale_is_instance_of(ex, type_id("Error"));
	// if (is_error) {
	// 	scale_runtime_catch(ex);
	// }

	scale_uint* stack_top = (scale_uint*) &stack_top;
	struct GC_stack_base sb;
	GC_get_my_stackbottom(&sb);
	scale_uint* stack_bottom = sb.mem_base;

	scale_int iteration_direction = stack_top < stack_bottom ? 1 : -1;
	while (stack_top != stack_bottom) {
		if (*stack_top != EXCEPTION_HANDLER_MARKER) {
			stack_top += iteration_direction;
			continue;
		}

		struct scale_exception_handler* handler = (struct scale_exception_handler*) stack_top;

		if (handler->finalizer) {
			handler->finalizer(handler->finalization_data);
		}

		handler->exception = ex;
		handler->marker = 0;

		longjmp(handler->jmp, 666);
	}

	scale_runtime_catch(ex);
}

scale_symbol_hidden static void native_trace(void) {
#if !defined(_WIN32) && !defined(__wasm__)
	void* callstack[1024];
	int frames = backtrace(callstack, 1024);
	int write(int, char*, size_t);
	write(2, "Backtrace:\n", 11);
	backtrace_symbols_fd(callstack, frames, 2);
	write(2, "\n", 1);
#endif
}

scale_symbol_hidden static inline void print_stacktrace_of(scale_Exception e) {
	#ifndef _WIN32
	int write(int, char*, size_t);
	#endif
	const char* do_print = getenv("SCALE_BACKTRACE");
	if (do_print) {
		if (strcmp(do_print, "full") == 0) {
			native_trace();
		} else if (atoi(do_print) == 1) {
			if (scale_is_array((scale_any*) e->stackTrace)) {
				for (scale_int i = 0; i < scale_array_size_unchecked((scale_any*) e->stackTrace); i++) {
					fprintf(stderr, "    %s\n", e->stackTrace[i]->data);
				}
				fprintf(stderr, "Some information was omitted. Run with 'SCALE_BACKTRACE=full' environment variable set to display full information\n");
			}
		}
	} else {
		fprintf(stderr, "Run with 'SCALE_BACKTRACE=1' environment variable set to display a backtrace\n");
	}
	#ifdef _WIN32
	#undef write
	#endif
}

scale_no_return void scale_runtime_catch(scale_any _ex) {
	scale_Exception ex = (scale_Exception) _ex;
	scale_str msg = ex->msg;

	fprintf(stderr, "Unexpected %s", ex->rtFields.type->type_name);
	if (thread_name) {
		fprintf(stderr, " in '%s'", thread_name);
	}
	fprintf(stderr, ": %s\n", msg->data);	
	
	print_stacktrace_of(ex);
	exit(EX_THROWN);
}

scale_no_return scale_symbol_hidden static void* scale_oom(scale_uint size) {
	fprintf(stderr, "Out of memory! Tried to allocate " SCALE_UINT_FMT " bytes\n", size);
	native_trace();
	exit(-1);
}

scale_any scale_thread_start(scale_any func, scale_any args) {
#ifdef _WIN32
	unsigned long thread_id;
	scale_any handle;
	handle = GC_CreateThread(
		NULL,
		0,
		func,
		args,
		0,
		&thread_id // unused
	);
	if (handle == NULL) {
		scale_runtime_error(EX_THREAD_ERROR, "Failed to create thread");
	}
	return handle;
#else
	pthread_t x;
	if (GC_pthread_create(&x, NULL, func, args) != 0) {
		scale_runtime_error(EX_THREAD_ERROR, "Failed to create thread");
	}
	return (scale_any) x;
#endif
}

void scale_thread_finish(scale_any thread) {
#ifdef _WIN32
	TerminateThread(thread, 0);
#else
	if (pthread_join((pthread_t) thread, NULL) != 0) {
		scale_runtime_error(EX_THREAD_ERROR, "Failed to join thread");
	}
#endif
}

void scale_thread_detach(scale_any thread) {
#ifdef _WIN32
	unimplemented("Threads can't detach on windows");
#else
	if (pthread_detach((pthread_t) thread) != 0) {
		scale_runtime_error(EX_THREAD_ERROR, "Failed to detach thread");
	}
#endif
}

struct async_func {
    scale_any thread;
    scale_any ret;
    scale_any args;
    scale_any(*func)(scale_any);
};

scale_symbol_hidden void scale_async_runner(struct async_func* args) {
	TRY {
		args->ret = args->func(args->args);
		if (args->args) free(args->args);
	} else {
		if (args->args) free(args->args);
		scale_runtime_catch(scale_exception_handler.exception);
	}
}

scale_any scale_run_async(scale_any func, scale_any func_args) {
	struct async_func* args = malloc(sizeof(struct async_func));
	args->args = func_args;
	args->func = func;
	args->thread = scale_thread_start(&scale_async_runner, args);
	return args;
}

scale_any scale_run_await(scale_any _args) {
	struct async_func* args = (struct async_func*) _args;
	if (args->thread) scale_thread_finish(args->thread);
	scale_any ret = args->ret;
	free(args);
	return ret;
}

void scale_yield() {
#ifdef _WIN32
	SwitchToThread();
#else
	sched_yield();
#endif
}

scale_constructor
void scale_setup(void) {
	static int setupCalled;
	if (setupCalled) {
		return;
	}
	scale_set_thread_name("Main Thread");
#if !defined(SCALE_KNOWN_LITTLE_ENDIAN)
	// Endian-ness detection
	short word = 0x0001;
	char *byte = (scale_int8*) &word;
	if (!byte[0]) {
		fprintf(stderr, "Invalid byte order detected!");
		exit(-1);
	}
#endif

// #if defined(__APPLE__) || defined(_WIN32)
// 	GC_use_threads_discovery();
// #endif
	GC_INIT();
	GC_set_oom_fn((GC_oom_func) &scale_oom);

	scale_set_up_signal_handler();

	setupCalled = 1;
}

#if defined(__cplusplus)
}
#endif
