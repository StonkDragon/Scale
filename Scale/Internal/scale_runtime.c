#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#if defined(_WIN32)
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <synchapi.h>
#else
#include <unistd.h>
#define sleep(s) do { struct timespec __ts = {((s) / 1000), ((s) % 1000) * 1000000}; nanosleep(&__ts, NULL); } while (0)
#endif

#include "scale_runtime.h"

#if defined(__cplusplus)
extern "C" {
#endif

#if !defined(SCL_DEFAULT_STACK_FRAME_COUNT)
#define SCL_DEFAULT_STACK_FRAME_COUNT 4096
#endif

const ID_t SclObjectHash = 0x49CC1B82U; // SclObject

#define unimplemented do { fprintf(stderr, "%s:%d: %s: Not Implemented\n", __FILE__, __LINE__, __FUNCTION__); exit(1); } while (0)

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

#define MARKER 0x5C105C10

_scl_symbol_hidden static memory_layout_t* _scl_get_memory_layout(scl_any ptr) {
	if (_scl_expect(ptr == nil, 0)) {
		return nil;
	}
	if (_scl_expect(!GC_is_heap_ptr(ptr), 0)) {
		return nil;
	}
	memory_layout_t* layout = (memory_layout_t*) (((char*) ptr) - sizeof(memory_layout_t));
	if (_scl_expect(((scl_int) layout) < 0, 0)) {
		return nil;
	}
	if (_scl_expect(layout->marker != MARKER, 0)) {
		return nil;
	}
	return layout;
}

scl_int _scl_sizeof(scl_any ptr) {
	memory_layout_t* layout = _scl_get_memory_layout(ptr);
	if (_scl_expect(layout == nil, 0)) {
		return 0;
	}
	return layout->allocation_size;
}

scl_any _scl_alloc(scl_int size) {
	size = ((size + 7) >> 3) << 3;
	if (size == 0) size = 8;

	scl_any ptr = GC_malloc(size + sizeof(memory_layout_t));

	((memory_layout_t*) ptr)->marker = MARKER;
	((memory_layout_t*) ptr)->allocation_size = size;
	((memory_layout_t*) ptr)->is_instance = 0;
	((memory_layout_t*) ptr)->is_array = 0;

	if (_scl_expect(ptr == nil, 0)) {
		_scl_runtime_error(EX_BAD_PTR, "allocate() failed!");
	}
	return ptr + sizeof(memory_layout_t);
}

scl_any _scl_realloc(scl_any ptr, scl_int size) {
	if (size == 0) {
		_scl_free(ptr);
		return nil;
	}

	size = ((size + 7) >> 3) << 3;
	if (size == 0) size = 8;

	memory_layout_t* layout = _scl_get_memory_layout(ptr);

	ptr = GC_realloc(ptr - sizeof(memory_layout_t), size + sizeof(memory_layout_t));
	if (_scl_expect(ptr == nil, 0)) {
		_scl_runtime_error(EX_BAD_PTR, "realloc() failed!");
	}

	if (layout) {
		((memory_layout_t*) ptr)->marker = MARKER;
		((memory_layout_t*) ptr)->allocation_size = size;
		((memory_layout_t*) ptr)->is_instance = layout->is_instance;
		((memory_layout_t*) ptr)->is_array = layout->is_array;
	} else {
		((memory_layout_t*) ptr)->marker = MARKER;
		((memory_layout_t*) ptr)->allocation_size = size;
		((memory_layout_t*) ptr)->is_instance = 0;
		((memory_layout_t*) ptr)->is_array = 0;
	}

	return ptr + sizeof(memory_layout_t);
}

void _scl_free(scl_any ptr) {
	if (ptr == nil) return;
	ptr -= sizeof(memory_layout_t);
	if (GC_is_heap_ptr(ptr)) {
		GC_free(ptr);
	}
}

void _scl_assert(scl_int b, const scl_int8* msg, ...) {
	if (!b) {
		scl_int8* cmsg = (scl_int8*) _scl_alloc(strlen(msg) * 8);
		va_list list;
		va_start(list, msg);
		vsnprintf(cmsg, strlen(msg) * 8 - 1, msg, list);
		va_end(list);

		char* tmp = strdup(cmsg);
		snprintf(cmsg, 22 + strlen(cmsg), "Assertion failed: %s", tmp);
		free(tmp);

		assert(0 && cmsg);
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

scl_str builtinToString(scl_any obj) {
	if (_scl_is_instance(obj)) {
		return (scl_str) virtual_call(obj, "toString()s;");
	}
	if (_scl_is_array((scl_any*) obj)) {
		return _scl_array_to_string((scl_any*) obj);
	}
	scl_int8* data = (scl_int8*) _scl_alloc(32);
	snprintf(data, 31, SCL_INT_FMT, (scl_int) obj);
	return str_of_exact(data);
}

_scl_symbol_hidden static void native_trace(void);

_scl_no_return void _scl_runtime_error(int code, const scl_int8* msg, ...) {
	printf("\n");

	va_list args;
	va_start(args, msg);
	scl_int8* str = (scl_int8*) malloc(strlen(msg) * 8);
	vsnprintf(str, strlen(msg) * 8, msg, args);
	printf("Exception: %s\n", str);

	va_end(args);

	if (errno) {
		printf("errno: %s\n", strerror(errno));
	}
	native_trace();

	exit(code);
}

scl_any _scl_cvarargs_to_array(va_list args, scl_int count) {
	scl_any* arr = (scl_any*) _scl_new_array_by_size(count, sizeof(scl_any));
	for (scl_int i = 0; i < count; i++) {
		arr[i] = *va_arg(args, scl_any*);
	}
	return arr;
}

_scl_symbol_hidden static void _scl_sigaction_handler(scl_int sig_num, siginfo_t* sig_info, void* ucontext) {
	const scl_int8* signalString;
	// Signals
	if (sig_num == -1) {
		signalString = nil;
	} else if (sig_num == SIGINT) {
		signalString = "interrupt";
	} else if (sig_num == SIGILL) {
		signalString = "illegal instruction";
	} else if (sig_num == SIGTRAP) {
		signalString = "trace trap";
	} else if (sig_num == SIGABRT) {
		signalString = "abort()";
	} else if (sig_num == SIGBUS) {
		signalString = "bus error";
	} else if (sig_num == SIGSEGV) {
		signalString = "segmentation violation";
	} else {
		signalString = "other signal";
	}

	fprintf(stderr, "Signal: %s\n", signalString);
	if (sig_num == SIGSEGV) {
		fprintf(stderr, "Tried read/write of faulty address %p\n", sig_info->si_addr);
	} else {
		fprintf(stderr, "Faulty address: %p\n", sig_info->si_addr);
	}
	if (sig_info->si_errno) {
		fprintf(stderr, "Error code: %d\n", sig_info->si_errno);
	}
	native_trace();
	fprintf(stderr, "\n");
	exit(sig_num);
}

_scl_symbol_hidden static void _scl_signal_handler(scl_int sig_num) {
	const scl_int8* signalString;
	if (sig_num == -1) {
		signalString = nil;
	} else if (sig_num == SIGINT) {
		signalString = "interrupt";
	} else if (sig_num == SIGILL) {
		signalString = "illegal instruction";
	} else if (sig_num == SIGTRAP) {
		signalString = "trace trap";
	} else if (sig_num == SIGABRT) {
		signalString = "abort()";
	} else if (sig_num == SIGBUS) {
		signalString = "bus error";
	} else if (sig_num == SIGSEGV) {
		signalString = "segmentation violation";
	} else {
		signalString = "other signal";
	}

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
	if (!_scl_is_instance(obj)) {
		return;
	}
	cxx_std_recursive_mutex_lock(((Struct*) obj)->mutex);
}

void _scl_unlock(scl_any obj) {
	if (!_scl_is_instance(obj)) {
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
	if (!_scl_is_instance(_obj)) {
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
	if (_scl_expect(!_scl_is_instance(type), 0)) {
		_scl_runtime_error(EX_CAST_ERROR, "Tried getting method on non-struct type (%p)", type);
	}

	scl_int index = _scl_search_method_index(
		(
			!onSuper ?
			((Struct*) type)->statics :
			((Struct*) type)->statics->super
		)->vtable_info,
		method,
		signature
	);

	return
		index >= 0 ?
		(
			!onSuper ?
			((Struct*) type)->statics->vtable :
			((Struct*) type)->statics->super->vtable
		)[index] :
		nil;
}

_scl_symbol_hidden static size_t str_index_of(const scl_int8* str, char c) {
	size_t i = 0;
	while (str[i] != c) {
		if (str[i] == '\0') return -1;
		i++;
	}
	return i;
}

_scl_symbol_hidden static void split_at(const scl_int8* str, size_t len, size_t index, scl_int8** left, scl_int8** right) {
	*left = (scl_int8*) malloc(len - (len - index) + 1);
	strncpy(*left, str, index);
	(*left)[index] = '\0';

	*right = (scl_int8*) malloc(len - index + 1);
	strncpy(*right, str + index, len - index);
	(*right)[len - index] = '\0';
}

scl_any _scl_get_vtable_function(scl_int onSuper, scl_any instance, const scl_int8* methodIdentifier) {
	size_t methodLen = strlen(methodIdentifier);
	size_t methodNameLen = str_index_of(methodIdentifier, '(');
	scl_int8* methodName;
	scl_int8* signature;
	split_at(methodIdentifier, methodLen, methodNameLen, &methodName, &signature);
	ID_t methodNameHash = type_id(methodName);
	ID_t signatureHash = type_id(signature);

	_scl_lambda m = _scl_get_method_on_type(instance, methodNameHash, signatureHash, onSuper);
	if (_scl_expect(m == nil, 0)) {
		_scl_runtime_error(EX_BAD_PTR, "Method '%s%s' not found on type '%s'", methodName, signature, ((Struct*) instance)->statics->type_name);
	}
	free(methodName);
	free(signature);
	return m;
}

scl_any _scl_alloc_struct(scl_int size, const TypeInfo* statics) {
	scl_any ptr = _scl_alloc(size);

	memory_layout_t* layout = _scl_get_memory_layout(ptr);
	if (_scl_expect(layout == nil, 0)) {
		_scl_runtime_error(EX_BAD_PTR, "Tried to access nil pointer");
	}
	layout->is_instance = 1;

	((Struct*) ptr)->mutex = cxx_std_recursive_mutex_new();
	((Struct*) ptr)->statics = (TypeInfo*) statics;
	if (statics) {
		((Struct*) ptr)->vtable = (_scl_lambda*) statics->vtable;
	}

	return ptr;
}

scl_int _scl_is_instance(scl_any ptr) {
	if (_scl_expect(ptr == nil, 0)) return 0;
	memory_layout_t* layout = _scl_get_memory_layout(ptr);
	if (_scl_expect(layout == nil, 0)) return 0;
	return layout->is_instance;
}

scl_int _scl_is_instance_of(scl_any ptr, ID_t type_id) {
	if (_scl_expect(((scl_int) ptr) <= 0, 0)) return 0;
	if (_scl_expect(!_scl_is_instance(ptr), 0)) return 0;

	Struct* ptrStruct = (Struct*) ptr;

	for (const TypeInfo* super = ptrStruct->statics; super != nil; super = super->super) {
		if (super->type == type_id) return 1;
	}

	return 0;
}

_scl_symbol_hidden static scl_int _scl_search_method_index(const struct _scl_methodinfo* const methods, ID_t id, ID_t sig) {
	if (_scl_expect(methods == nil, 0)) return -1;

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

extern scl_any _create_cast_error(scl_int8* msg) __attribute__((weak));

scl_any _scl_checked_cast(scl_any instance, ID_t target_type, const scl_int8* target_type_name) {
	if (_scl_expect(!_scl_is_instance_of(instance, target_type), 0)) {
		typedef struct Struct_CastError {
			Struct rtFields;
			scl_str msg;
			struct Struct_Array* stackTrace;
			scl_str errno_str;
		}* scl_CastError;

		scl_int size;
		scl_int8* cmsg;
		if (instance == nil) {
			size = 96 + strlen(target_type_name);
			cmsg = (scl_int8*) _scl_alloc(size);
			snprintf(cmsg, size - 1, "Cannot cast nil to type '%s'\n", target_type_name);
			_scl_assert(0, cmsg);
		}

		memory_layout_t* layout = _scl_get_memory_layout(instance);

		if (layout && layout->is_instance) {
			size = 64 + strlen(((Struct*) instance)->statics->type_name) + strlen(target_type_name);
			cmsg = (scl_int8*) _scl_alloc(size);
			snprintf(cmsg, size - 1, "Cannot cast instance of struct '%s' to type '%s'\n", ((Struct*) instance)->statics->type_name, target_type_name);
		} else {
			size = 96 + strlen(target_type_name);
			cmsg = (scl_int8*) _scl_alloc(size);
			snprintf(cmsg, size - 1, "Cannot cast non-object to type '%s'\n", target_type_name);
		}
		_scl_assert(0, cmsg);
	}
	return instance;
}

scl_int8* _scl_typename_or_else(scl_any instance, const scl_int8* else_) {
	if (_scl_is_instance(instance)) {
		return (scl_int8*) ((Struct*) instance)->statics->type_name;
	}
	return (scl_int8*) else_;
}

scl_any _scl_new_array_by_size(scl_int num_elems, scl_int elem_size) {
	if (_scl_expect(num_elems < 0, 0)) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array size must not be less than 0");
	}
	scl_any* arr = (scl_any*) _scl_alloc(num_elems * elem_size + sizeof(scl_int));
	_scl_get_memory_layout(arr)->is_array = 1;
	((scl_int*) arr)[0] = num_elems;
	return arr + 1;
}

scl_any _scl_migrate_foreign_array(scl_any arr, scl_int num_elems, scl_int elem_size) {
	if (_scl_expect(num_elems < 0, 0)) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array size must not be less than 0");
	}
	scl_any* new_arr = (scl_any*) _scl_alloc(num_elems * elem_size + sizeof(scl_int));
	_scl_get_memory_layout(new_arr)->is_array = 1;
	((scl_int*) new_arr)[0] = num_elems;
	memcpy(new_arr + 1, arr, num_elems * elem_size);
	return new_arr + 1;
}

scl_int _scl_is_array(scl_any* arr) {
	if (_scl_expect(arr == nil, 0)) return 0;
	memory_layout_t* layout = _scl_get_memory_layout(arr - 1);
	if (_scl_expect(layout == nil, 0)) return 0;
	return layout->is_array;
}

scl_any* _scl_multi_new_array_by_size(scl_int dimensions, scl_int sizes[], scl_int elem_size) {
	if (_scl_expect(dimensions < 1, 0)) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array dimensions must not be less than 1");
	}
	if (dimensions == 1) {
		return (scl_any*) _scl_new_array_by_size(sizes[0], elem_size);
	}
	scl_any* arr = (scl_any*) _scl_alloc(sizes[0] * sizeof(scl_any) + sizeof(scl_int));
	((scl_int*) arr)[0] = sizes[0];
	for (scl_int i = 1; i <= sizes[0]; i++) {
		arr[i] = _scl_multi_new_array_by_size(dimensions - 1, &(sizes[1]), elem_size);
	}
	return arr + 1;
}

scl_int _scl_array_size_unchecked(scl_any* arr) {
	return *((scl_int*) arr - 1);
}

scl_int _scl_array_size(scl_any* arr) {
	if (_scl_expect(arr == nil, 0)) {
		_scl_runtime_error(EX_BAD_PTR, "nil pointer detected");
	}
	if (_scl_expect(!_scl_is_array(arr), 0)) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array must be initialized with 'new[]')");
	}
	return *((scl_int*) arr - 1);
}

void _scl_array_check_bounds_or_throw(scl_any* arr, scl_int index) {
	if (_scl_expect(arr == nil, 0)) {
		_scl_runtime_error(EX_BAD_PTR, "nil pointer detected");
	}
	if (_scl_expect(!_scl_is_array(arr), 0)) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array must be initialized with 'new[]')");
	}
	scl_int size = *((scl_int*) arr - 1);
	if (index < 0 || index >= size) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Index " SCL_INT_FMT " out of bounds for array of size " SCL_INT_FMT, index, size);
	}
}

scl_any* _scl_array_resize(scl_any* arr, scl_int new_size) {
	if (_scl_expect(arr == nil, 0)) {
		_scl_runtime_error(EX_BAD_PTR, "nil pointer detected");
	}
	if (_scl_expect(!_scl_is_array(arr), 0)) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array must be initialized with 'new[]')");
	}
	if (_scl_expect(new_size < 1, 0)) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array size must not be less than 1");
	}
	scl_any* new_arr = (scl_any*) _scl_realloc(arr - 1, new_size * sizeof(scl_any) + sizeof(scl_int));
	_scl_get_memory_layout(new_arr)->is_array = 1;
	((scl_int*) new_arr)[0] = new_size;
	return new_arr + 1;
}

scl_any* _scl_array_sort(scl_any* arr) {
	if (_scl_expect(arr == nil, 0)) {
		_scl_runtime_error(EX_BAD_PTR, "nil pointer detected");
	}
	if (_scl_expect(!_scl_is_array(arr), 0)) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array must be initialized with 'new[]')");
	}
	scl_int size = _scl_array_size_unchecked(arr);
	for (scl_int i = 0; i < size; i++) {
		scl_any tmp = arr[i];
		scl_int j = i - 1;
		while (j >= 0) {
			if (tmp < arr[j]) {
				arr[j + 1] = arr[j];
				arr[j] = tmp;
			} else {
				arr[j + 1] = tmp;
				break;
			}
			j--;
		}
	}
	return arr;
}

scl_any* _scl_array_reverse(scl_any* arr) {
	if (_scl_expect(arr == nil, 0)) {
		_scl_runtime_error(EX_BAD_PTR, "nil pointer detected");
	}
	if (_scl_expect(!_scl_is_array(arr), 0)) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array must be initialized with 'new[]')");
	}
	scl_int size = _scl_array_size_unchecked(arr);
	scl_int half = size / 2;
	for (scl_int i = 0; i < half; i++) {
		scl_any tmp = arr[i];
		arr[i] = arr[size - i - 1];
		arr[size - i - 1] = tmp;
	}
	return arr;
}

scl_str _scl_array_to_string(scl_any* arr) {
	if (_scl_expect(arr == nil, 0)) {
		_scl_runtime_error(EX_BAD_PTR, "nil pointer detected");
	}
	if (_scl_expect(!_scl_is_array(arr), 0)) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array must be initialized with 'new[]')");
	}
	scl_int size = _scl_array_size_unchecked(arr);
	scl_str s = str_of_exact("[");
	for (scl_int i = 0; i < size; i++) {
		if (i) {
			s = (scl_str) virtual_call(s, "append(s;)s;", str_of_exact(", "));
		}
		scl_str tmp;
		if (_scl_is_instance(arr[i])) {
			tmp = (scl_str) virtual_call(arr[i], "toString()s;");
		} else {
			scl_int8* str = (scl_int8*) _scl_alloc(32);
			snprintf(str, 31, SCL_INT_FMT, ((scl_int*) arr)[i]);
			tmp = str_of_exact(str);
		}
		s = (scl_str) virtual_call(s, "append(s;)s;", tmp);
	}
	return (scl_str) virtual_call(s, "append(s;)s;", str_of_exact("]"));
}

void _scl_trace_remove(const struct _scl_backtrace* _) {}

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

	scl_int iteration_direction = 1;
	if (stack_top > stack_bottom) {
		iteration_direction = -1;
	}

	while (stack_top != stack_bottom) {
		if (*stack_top != EXCEPTION_HANDLER_MARKER) {
			stack_top += iteration_direction;
			continue;
		}

		struct _scl_exception_handler* handler = (struct _scl_exception_handler*) stack_top;
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
