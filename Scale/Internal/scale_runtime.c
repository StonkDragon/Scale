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

tls _scl_stack_t		__cs = {0};

#define unimplemented do { fprintf(stderr, "%s:%d: %s: Not Implemented\n", __FILE__, __LINE__, __FUNCTION__); exit(1); } while (0)

typedef struct Struct {
	// Actual function ptrs for this object
	_scl_lambda*	vtable;
	// Typeinfo for this object
	TypeInfo*		statics;
	// Mutex for this object
	mutex_t			mutex;
} Struct;

typedef struct Struct_Exception {
	Struct rtFields;
	scl_str msg;
	struct Struct_Array* stackTrace;
	scl_str errno_str;
}* scl_Exception;

#define EXCEPTION	((scl_Exception) *(__cs.ex))

typedef struct Struct_NullPointerException {
	struct Struct_Exception self;
}* scl_NullPointerException;

typedef struct Struct_Array {
	Struct rtFields;
	scl_any* values;
	scl_int count;
	scl_int capacity;
	scl_int initCapacity;
}* scl_Array;

typedef struct Struct_ReadOnlyArray {
	struct Struct_Array self;
}* scl_ReadOnlyArray;

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

typedef struct memory_layout {
	scl_int32 marker;
	scl_int allocation_size;
	scl_int8 is_instance:1;
	scl_int8 is_array:1;
} memory_layout_t;

memory_layout_t* _scl_get_memory_layout(scl_any ptr) {
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

scl_int is_array(scl_any ptr) {
	memory_layout_t* layout = _scl_get_memory_layout(ptr);
	if (_scl_expect(layout == nil, 0)) {
		return 0;
	}
	return layout->is_array;
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
		_scl_security_throw(EX_BAD_PTR, "allocate() failed!");
	}
	return ptr + sizeof(memory_layout_t);
}

scl_any _scl_realloc(scl_any ptr, scl_int size) {
	if (size == 0) {
		scl_NullPointerException ex = ALLOC(NullPointerException);
		virtual_call(ex, "init()V;");
		ex->self.msg = str_of_exact("realloc() called with size 0");
		_scl_throw(ex);
	}

	size = ((size + 7) >> 3) << 3;
	if (size == 0) size = 8;

	memory_layout_t* layout = _scl_get_memory_layout(ptr);

	ptr = GC_realloc(ptr - sizeof(memory_layout_t), size + sizeof(memory_layout_t));
	if (_scl_expect(ptr == nil, 0)) {
		_scl_security_throw(EX_BAD_PTR, "realloc() failed!");
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

		snprintf(cmsg, 22 + strlen(cmsg), "Assertion failed: %s", _scl_strdup(cmsg));
		scl_AssertError err = ALLOC(AssertError);
		virtual_call(err, "init(s;)V;", str_of_exact(cmsg));

		_scl_throw(err);
	}
}

void builtinUnreachable(void) {
	scl_UnreachableError err = ALLOC(UnreachableError);
	virtual_call(err, "init(s;)V;", str_of_exact("Unreachable!"));

	_scl_throw(err);
}

scl_int builtinIsInstanceOf(scl_any obj, scl_str type) {
	return _scl_is_instance_of(obj, type->hash);
}

scl_str builtinToString(scl_any obj) {
	if (_scl_is_instance_of(obj, SclObjectHash)) {
		return (scl_str) virtual_call(obj, "toString()s;");
	}
	if (_scl_is_array((scl_any*) obj)) {
		return _scl_array_to_string((scl_any*) obj);
	}
	scl_int8* data = (scl_int8*) _scl_alloc(32);
	snprintf(data, 31, SCL_INT_FMT, (scl_int) obj);
	return str_of_exact(data);
}

_scl_no_return void _scl_security_throw(int code, const scl_int8* msg, ...) {
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
	print_stacktrace();

	_scl_security_safe_exit(code);
}

_scl_no_return void _scl_security_safe_exit(int code) {
	exit(code);
}

scl_int8* _scl_strndup(const scl_int8* str, scl_int len) {
	scl_int8* newStr = (scl_int8*) _scl_alloc(len + 1);
	strncpy(newStr, str, len);
	return newStr;
}

scl_int8* _scl_strdup(const scl_int8* str) {
	scl_int len = strlen(str);
	return _scl_strndup(str, len);
}

scl_str _scl_string_with_hash_len(const scl_int8* data, ID_t hash, scl_int len) {
	scl_str self = ALLOC(str);
	self->data = (scl_int8*) data;
	self->length = len;
	self->hash = hash;
	return self;
}

scl_str _scl_create_string(const scl_int8* data) {
	scl_str self = ALLOC(str);
	self->data = (scl_int8*) data;
	self->length = strlen(data);
	self->hash = type_id(data);
	return self;
}

scl_any _scl_cvarargs_to_array(va_list args, scl_int count) {
	scl_Array arr = (scl_Array) ALLOC(ReadOnlyArray);
	virtual_call(arr, "init(i;)V;", count);
	for (scl_int i = 0; i < count; i++) {
		arr->values[i] = *(va_arg(args, scl_any*));
	}
	arr->count = count;
	return arr;
}

void nativeTrace();

void print_stacktrace(void) {
	fprintf(stderr, "Stacktrace:\n");
	nativeTrace();
	fprintf(stderr, "\n");
}

void _scl_sigaction_handler(scl_int sig_num, siginfo_t* sig_info, void* ucontext) {
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
	nativeTrace();
	fprintf(stderr, "\n");
	exit(sig_num);
}

void _scl_signal_handler(scl_int sig_num) {
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
	print_stacktrace();

	exit(sig_num);
}

scl_any _scl_c_arr_to_scl_array(scl_any arr[]) {
	scl_Array array = (scl_Array) ALLOC(ReadOnlyArray);
	scl_int cap = 0;
	while (arr[cap] != nil) {
		cap++;
	}

	array->capacity = array->initCapacity = cap;
	array->count = 0;
	array->values = _scl_new_array_by_size(cap, sizeof(scl_str));
	for (scl_int i = 0; i < cap; i++) {
		((scl_str*) array->values)[(scl_int) array->count++] = _scl_create_string(((scl_int8**) arr)[i]);
	}
	return array;
}

void _scl_sleep(scl_int millis) {
	sleep(millis);
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

__pure2 scl_uint _scl_rotl(const scl_uint value, const scl_int shift) {
    return (value << shift) | (value >> ((sizeof(scl_uint) << 3) - shift));
}

__pure2 scl_uint _scl_rotr(const scl_uint value, const scl_int shift) {
    return (value >> shift) | (value << ((sizeof(scl_uint) << 3) - shift));
}

scl_int _scl_identity_hash(scl_any _obj) {
	if (!_scl_is_instance_of(_obj, SclObjectHash)) {
		return REINTERPRET_CAST(scl_int, _obj);
	}
	Struct* obj = (Struct*) _obj;
	Process$lock((volatile scl_any) obj);
	scl_int size = _scl_sizeof(obj);
	scl_int hash = REINTERPRET_CAST(scl_int, obj) % 17;
	for (scl_int i = 0; i < size; i++) {
		hash = _scl_rotl(hash, 5) ^ ((scl_int8*) obj)[i];
	}
	Process$unlock((volatile scl_any) obj);
	return hash;
}

scl_any _scl_atomic_clone(scl_any ptr) {
	unimplemented;
	scl_int size = _scl_sizeof(ptr);
	scl_any clone = _scl_alloc(size);

	memcpy(clone, ptr, size);
	return clone;
}

_scl_lambda _scl_get_method_on_type(scl_any type, ID_t method, ID_t signature, int onSuper) {
	if (_scl_expect(!_scl_is_instance_of(type, SclObjectHash), 0)) {
		_scl_security_throw(EX_CAST_ERROR, "Tried getting method on non-struct type (%p)", type);
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

static scl_int8* substr_of(const scl_int8* str, size_t len, size_t beg, size_t end) {
	scl_int8* sub = (scl_int8*) malloc(len);
	strcpy(sub, str + beg);
	sub[end - beg] = '\0';
	return sub;
}

static size_t str_index_of(const scl_int8* str, char c) {
	size_t i = 0;
	while (str[i] != c) {
		i++;
	}
	return i;
}

static size_t str_last_index_of(const scl_int8* str, char c) {
	size_t i = strlen(str) - 1;
	while (str[i] != c) {
		i--;
	}
	return i;
}

static size_t str_num_of_occurences(const scl_int8* str, char c) {
	size_t i = 0;
	size_t count = 0;
	while (str[i] != '\0') {
		if (str[i] == c) {
			count++;
		}
		i++;
	}
	return count;
}

static void split_at(const scl_int8* str, size_t len, size_t index, scl_int8** left, scl_int8** right) {
	*left = (scl_int8*) malloc(len - (len - index) + 1);
	strncpy(*left, str, index);
	(*left)[index] = '\0';

	*right = (scl_int8*) malloc(len - index + 1);
	strncpy(*right, str + index, len - index);
	(*right)[len - index] = '\0';
}

void dumpStack();

#define strequals(a, b) (strcmp(a, b) == 0)
#define strstarts(a, b) (strncmp((a), (b), strlen((b))) == 0)

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
		_scl_security_throw(EX_BAD_PTR, "Method '%s%s' not found on type '%s'", methodName, signature, ((Struct*) instance)->statics->type_name);
	}
	free(methodName);
	free(signature);
	return m;
}

mutex_t _scl_mutex_new(void) {
	return cxx_std_recursive_mutex_new();
}

scl_any _scl_alloc_struct(scl_int size, const TypeInfo* statics) {
	scl_any ptr = _scl_alloc(size);

	memory_layout_t* layout = _scl_get_memory_layout(ptr);
	if (_scl_expect(layout == nil, 0)) {
		_scl_security_throw(EX_BAD_PTR, "Tried to access nil pointer");
	}
	layout->is_instance = 1;

	((Struct*) ptr)->mutex = _scl_mutex_new();
	((Struct*) ptr)->statics = (TypeInfo*) statics;
	((Struct*) ptr)->vtable = (_scl_lambda*) statics->vtable;

	return ptr;
}

scl_int _scl_is_instance_of(scl_any ptr, ID_t type_id) {
	if (_scl_expect(((scl_int) ptr) <= 0, 0)) return 0;

	memory_layout_t* layout = _scl_get_memory_layout(ptr);
	if (_scl_expect(layout == nil, 0)) return 0;
	if (_scl_expect(!layout->is_instance, 0)) return 0;

	Struct* ptrStruct = (Struct*) ptr;

	if (type_id == SclObjectHash) return 1;

	for (const TypeInfo* super = ptrStruct->statics; super != nil; super = super->super) {
		if (super->type == type_id) return 1;
	}

	return 0;
}

scl_int _scl_search_method_index(const struct _scl_methodinfo* const methods, ID_t id, ID_t sig) {
	if (_scl_expect(methods == nil, 0)) return -1;

	for (scl_int i = 0; *(scl_int*) &(methods[i].pure_name); i++) {
		if (methods[i].pure_name == id && (sig == 0 || methods[i].signature == sig)) {
			return i;
		}
	}
	return -1;
}

typedef void (*_scl_sigHandler)(scl_int);

int _scl_gc_is_disabled(void) {
	return GC_is_disabled();
}

void _scl_thread_new(_scl_thread_t* t, scl_any(*f)(scl_any), scl_any arg) {
	*t = cxx_std_thread_new_with_args(f, arg);
}

void _scl_thread_wait_for(_scl_thread_t t) {
	cxx_std_thread_join(t);
	cxx_std_thread_delete(t);
}

void _scl_thread_detach(_scl_thread_t t) {
	cxx_std_thread_detach(t);
}

_scl_thread_t _scl_thread_current() {
	return cxx_std_thread_new();
}

void _scl_set_signal_handler(_scl_sigHandler handler, scl_int sig) {
	if (_scl_expect(sig < 0 || sig >= 32, 0)) {
		typedef struct Struct_InvalidSignalException {
			struct Struct_Exception self;
			scl_int sig;
		}* scl_InvalidSignalException;
		scl_Exception e = (scl_Exception) ALLOC(InvalidSignalException);
		virtual_call(e, "init(i;)V;", sig);

		scl_int8* p = (scl_int8*) _scl_alloc(64);
		snprintf(p, 64, "Invalid signal: " SCL_INT_FMT, sig);
		e->msg = str_of_exact(p);
		
		_scl_throw(e);
	}
	signal(sig, (void(*)(int)) handler);
}

void _scl_reset_signal_handler(scl_int sig) {
	if (_scl_expect(sig < 0 || sig >= 32, 0)) {
		typedef struct Struct_InvalidSignalException {
			struct Struct_Exception self;
			scl_int sig;
		}* scl_InvalidSignalException;
		scl_Exception e = (scl_Exception) ALLOC(InvalidSignalException);
		virtual_call(e, "init(i;)V;", sig);

		scl_int8* p = (scl_int8*) _scl_alloc(64);
		snprintf(p, 64, "Invalid signal: " SCL_INT_FMT, sig);
		e->msg = str_of_exact(p);
		
		_scl_throw(e);
	}
	signal(sig, (void(*)(int)) _scl_signal_handler);
}

void _scl_set_up_signal_handler(void) {
#if !defined(_WIN32)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-W#pragma-messages"
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

void _scl_exception_push(void) {
	// *(__cs.et++) = __cs.tp;
	// ++__cs.ex;
	// ++__cs.jmp;
}

void _scl_exception_drop(void) {
	// --__cs.et;
	// --__cs.ex;
	// --__cs.jmp;
}

scl_int8** _scl_platform_get_env(void) {
#if defined(WIN) && (_MSC_VER >= 1900)
	return *__p__environ();
#else
	extern scl_int8** environ;
	return environ;
#endif
}

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
			scl_CastError err = ALLOC(CastError);
			virtual_call(err, "init(s;)V;", str_of_exact(cmsg));
			_scl_throw(err);
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
		scl_CastError err = ALLOC(CastError);
		virtual_call(err, "init(s;)V;", str_of_exact(cmsg));
		_scl_throw(err);
	}
	return instance;
}

scl_int8* _scl_typename_or_else(scl_any instance, const scl_int8* else_) {
	if (_scl_is_instance_of(instance, SclObjectHash)) {
		return (scl_int8*) ((Struct*) instance)->statics->type_name;
	}
	return (scl_int8*) else_;
}

void _scl_stack_new(void) {
	// if (_scl_expect((__cs.tbp = __cs.tp = (const scl_int8**) system_allocate(sizeof(scl_int8*) * SCL_DEFAULT_STACK_FRAME_COUNT)) == 0, 0))
	// 	_scl_security_throw(EX_BAD_PTR, "Failed to allocate memory for trace stack!");

	// if (_scl_expect((__cs.ex_base = __cs.ex = (scl_any*) GC_malloc_uncollectable(sizeof(scl_any) * SCL_DEFAULT_STACK_FRAME_COUNT)) == 0, 0))
	// 	_scl_security_throw(EX_BAD_PTR, "Failed to allocate memory for exception stack!");

	// if (_scl_expect((__cs.et_base = __cs.et = (const scl_int8***) system_allocate(sizeof(scl_any) * SCL_DEFAULT_STACK_FRAME_COUNT)) == 0, 0))
	// 	_scl_security_throw(EX_BAD_PTR, "Failed to allocate memory for exception trace stack!");

	if (_scl_expect((__cs.trace = system_allocate(sizeof(scl_int8*) * SCL_DEFAULT_STACK_FRAME_COUNT)) == 0, 0))
		_scl_security_throw(EX_BAD_PTR, "Failed to allocate memory for trace stack!");
}

scl_int _scl_errno(void) {
	return errno;
}

scl_int8* _scl_errno_ptr(void) {
	return strerror(errno);
}

scl_str _scl_errno_str(void) {
	return str_of(_scl_errno_ptr());
}

void _scl_stack_free(void) {
	// system_free(__cs.tbp);
	// system_free(__cs.et_base);
	// GC_free(__cs.ex_base);
	// system_free(__cs.jmp_base);
	system_free(__cs.trace);
	memset(&__cs, 0, sizeof(__cs));
}

scl_int _scl_strncmp(scl_int8* str1, scl_int8* str2, scl_int count) {
	return strncmp(str1, str2, count);
}

scl_int8* _scl_strcpy(scl_int8* dest, scl_int8* src) {
	return strcpy(dest, src);
}

scl_any _scl_memset(scl_any ptr, scl_int32 val, scl_int len) {
	return memset(ptr, val, len);
}

scl_any _scl_memcpy(scl_any dest, scl_any src, scl_int n) {
	return memcpy(dest, src, n);
}

void Process$lock(volatile scl_any obj) {
	if (_scl_expect(!obj, 0)) return;
	cxx_std_recursive_mutex_lock(((volatile Struct*) obj)->mutex);
}

void Process$unlock(volatile scl_any obj) {
	if (_scl_expect(!obj, 0)) return;
	cxx_std_recursive_mutex_unlock(((volatile Struct*) obj)->mutex);
}

const scl_int8* GarbageCollector$getImplementation0(void) {
	return "Boehm GC";
}

scl_bool GarbageCollector$isPaused0(void) {
	return GC_is_disabled();
}

void GarbageCollector$setPaused0(scl_bool paused) {
	if (paused) {
		GC_disable();
	} else {
		GC_enable();
	}
}

void GarbageCollector$run0(void) {
	GC_gcollect();
}

scl_int GarbageCollector$heapSize(void) {
	scl_uint heapSize;
	GC_get_heap_usage_safe(
		&heapSize,
		nil,
		nil,
		nil,
		nil
	);
	return (scl_int) heapSize;
}

scl_int GarbageCollector$freeBytesEstimate(void) {
	scl_uint freeBytes;
	GC_get_heap_usage_safe(
		nil,
		&freeBytes,
		nil,
		nil,
		nil
	);
	return (scl_int) freeBytes;
}

scl_int GarbageCollector$bytesSinceLastCollect(void) {
	scl_uint bytesSinceLastCollect;
	GC_get_heap_usage_safe(
		nil,
		nil,
		&bytesSinceLastCollect,
		nil,
		nil
	);
	return (scl_int) bytesSinceLastCollect;
}

scl_int GarbageCollector$totalMemory(void) {
	scl_uint totalMemory;
	GC_get_heap_usage_safe(
		nil,
		nil,
		nil,
		&totalMemory,
		nil
	);
	return (scl_int) totalMemory;
}

scl_any _scl_new_array_by_size(scl_int num_elems, scl_int elem_size) {
	if (_scl_expect(num_elems < 1, 0)) {
		scl_InvalidArgumentException e = ALLOC(InvalidArgumentException);
		virtual_call(e, "init(s;)V;", str_of_exact("Array size must not be less than 1"));
		_scl_throw(e);
	}
	scl_any* arr = (scl_any*) _scl_alloc(num_elems * elem_size + sizeof(scl_int));
	_scl_get_memory_layout(arr)->is_array = 1;
	((scl_int*) arr)[0] = num_elems;
	return arr + 1;
}

scl_any* _scl_new_array(scl_int num_elems) {
	return (scl_any*) _scl_new_array_by_size(num_elems, sizeof(scl_any));
}

scl_int _scl_is_array(scl_any* arr) {
	if (_scl_expect(arr == nil, 0)) return 0;
	memory_layout_t* layout = _scl_get_memory_layout(arr - 1);
	if (_scl_expect(layout == nil, 0)) return 0;
	return layout->is_array;
}

void remove_at(scl_any** arr, scl_int* count, scl_int index) {
	if (_scl_expect(index < 0 || index >= *count, 0)) return;
	(*count)--;
	for (scl_int i = index; i < *count; i++) {
		arr[i] = arr[i + 1];
	}
}

scl_any* _scl_multi_new_array_by_size(scl_int dimensions, scl_int sizes[], scl_int elem_size) {
	if (_scl_expect(dimensions < 1, 0)) {
		scl_InvalidArgumentException e = ALLOC(InvalidArgumentException);
		virtual_call(e, "init(s;)V;", str_of_exact("Array dimensions must not be less than 1"));
		_scl_throw(e);
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

scl_any* _scl_multi_new_array(scl_int dimensions, scl_int sizes[]) {
	return _scl_multi_new_array_by_size(dimensions, sizes, sizeof(scl_any));
}

scl_int _scl_array_size_unchecked(scl_any* arr) {
	return *((scl_int*) arr - 1);
}

scl_int _scl_array_size(scl_any* arr) {
	if (_scl_expect(arr == nil, 0)) {
		scl_NullPointerException e = ALLOC(NullPointerException);
		virtual_call(e, "init(s;)V;", str_of_exact("nil pointer detected"));
		_scl_throw(e);
	}
	if (_scl_expect(!_scl_is_array(arr), 0)) {
		scl_InvalidArgumentException e = ALLOC(InvalidArgumentException);
		virtual_call(e, "init(s;)V;", str_of_exact("Array must be initialized with 'new[]'"));
		_scl_throw(e);
	}
	return *((scl_int*) arr - 1);
}

void _scl_array_check_bounds_or_throw(scl_any* arr, scl_int index) {
	if (_scl_expect(arr == nil, 0)) {
		scl_NullPointerException e = ALLOC(NullPointerException);
		virtual_call(e, "init(s;)V;", str_of_exact("nil pointer detected"));
		_scl_throw(e);
	}
	if (_scl_expect(!_scl_is_array(arr), 0)) {
		scl_InvalidArgumentException e = ALLOC(InvalidArgumentException);
		virtual_call(e, "init(s;)V;", str_of_exact("Array must be initialized with 'new[]')"));
		_scl_throw(e);
	}
	scl_int size = *((scl_int*) arr - 1);
	if (index < 0 || index >= size) {
		scl_IndexOutOfBoundsException e = ALLOC(IndexOutOfBoundsException);
		scl_int8* str = (scl_int8*) _scl_alloc(64);
		snprintf(str, 63, "Index " SCL_INT_FMT " out of bounds for array of size " SCL_INT_FMT, index, size);
		virtual_call(e, "init(s;)V;", str_of_exact(str));
		_scl_throw(e);
	}
}

scl_any* _scl_array_resize(scl_any* arr, scl_int new_size) {
	if (_scl_expect(arr == nil, 0)) {
		scl_NullPointerException e = ALLOC(NullPointerException);
		virtual_call(e, "init(s;)V;", str_of_exact("nil pointer detected"));
		_scl_throw(e);
	}
	if (_scl_expect(!_scl_is_array(arr), 0)) {
		scl_InvalidArgumentException e = ALLOC(InvalidArgumentException);
		virtual_call(e, "init(s;)V;", str_of_exact("Array must be initialized with 'new[]'"));
		_scl_throw(e);
	}
	if (_scl_expect(new_size < 1, 0)) {
		scl_InvalidArgumentException e = ALLOC(InvalidArgumentException);
		virtual_call(e, "init(s;)V;", str_of_exact("New array size must not be less than 1"));
		_scl_throw(e);
	}
	scl_any* new_arr = (scl_any*) _scl_realloc(arr - 1, new_size * sizeof(scl_any) + sizeof(scl_int));
	_scl_get_memory_layout(new_arr)->is_array = 1;
	((scl_int*) new_arr)[0] = new_size;
	return new_arr + 1;
}

scl_any* _scl_array_sort(scl_any* arr) {
	if (_scl_expect(arr == nil, 0)) {
		scl_NullPointerException e = ALLOC(NullPointerException);
		virtual_call(e, "init(s;)V;", str_of_exact("nil pointer detected"));
		_scl_throw(e);
	}
	if (_scl_expect(!_scl_is_array(arr), 0)) {
		scl_InvalidArgumentException e = ALLOC(InvalidArgumentException);
		virtual_call(e, "init(s;)V;", str_of_exact("Array must be initialized with 'new[]'"));
		_scl_throw(e);
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
		scl_NullPointerException e = ALLOC(NullPointerException);
		virtual_call(e, "init(s;)V;", str_of_exact("nil pointer detected"));
		_scl_throw(e);
	}
	if (_scl_expect(!_scl_is_array(arr), 0)) {
		scl_InvalidArgumentException e = ALLOC(InvalidArgumentException);
		virtual_call(e, "init(s;)V;", str_of_exact("Array must be initialized with 'new[]'"));
		_scl_throw(e);
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

scl_str int_to_string(scl_int i) {
	scl_int8* str = (scl_int8*) _scl_alloc(32);
	snprintf(str, 32, SCL_INT_FMT, i);
	scl_str s = str_of_exact(str);
	return s;
}

scl_str _scl_array_to_string(scl_any* arr) {
	if (_scl_expect(arr == nil, 0)) {
		scl_NullPointerException e = ALLOC(NullPointerException);
		virtual_call(e, "init(s;)V;", str_of_exact("nil pointer detected"));
		_scl_throw(e);
	}
	if (_scl_expect(!_scl_is_array(arr), 0)) {
		scl_InvalidArgumentException e = ALLOC(InvalidArgumentException);
		virtual_call(e, "init(s;)V;", str_of_exact("Array must be initialized with 'new[]'"));
		_scl_throw(e);
	}
	scl_int size = _scl_array_size_unchecked(arr);
	scl_str s = str_of_exact("[");
	for (scl_int i = 0; i < size; i++) {
		if (i) {
			s = (scl_str) virtual_call(s, "append(s;)s;", str_of_exact(", "));
		}
		scl_str tmp;
		if (_scl_is_instance_of(arr[i], SclObjectHash)) {
			tmp = (scl_str) virtual_call(arr[i], "toString()s;");
		} else {
			tmp = int_to_string((scl_int) arr[i]);
		}
		s = (scl_str) virtual_call(s, "append(s;)s;", tmp);
	}
	return (scl_str) virtual_call(s, "append(s;)s;", str_of_exact("]"));
}

const scl_int8* _scl_type_to_rt_sig(const scl_int8* type) {
	if (strequals(type, "any")) return "a;";
	if (strequals(type, "int") || strequals(type, "bool")) return "i;";
	if (strequals(type, "float")) return "f;";
	if (strequals(type, "str")) return "s;";
	if (strequals(type, "none")) return "V;";
	if (strequals(type, "lambda") || strstarts(type, "lambda(")) return "F;";
	if (strequals(type, "[int8]")) return "cs;";
	if (strequals(type, "[any]")) return "p;";
	size_t len = strlen(type);
	if (len > 2 && type[0] == '[' && type[len - 1] == ']') {
		scl_int8* tmp = (scl_int8*) _scl_alloc(len + 2);
		scl_int8* tmp2 = substr_of(type, len, 1, len - 1);
		snprintf(tmp, len + 2, "[%s", _scl_type_to_rt_sig(tmp2));
		free(tmp2);
		return tmp;
	}
	if (strequals(type, "int8")) return "int8;";
	if (strequals(type, "int16")) return "int16;";
	if (strequals(type, "int32")) return "int32;";
	if (strequals(type, "int64")) return "int64;";
	if (strequals(type, "uint8")) return "uint8;";
	if (strequals(type, "uint16")) return "uint16;";
	if (strequals(type, "uint32")) return "uint32;";
	if (strequals(type, "uint64")) return "uint64;";
	if (strequals(type, "uint")) return "u;";
	scl_int8* tmp = (scl_int8*) _scl_alloc(len + 3);
	snprintf(tmp, len + 3, "L%s;", type);
	return tmp;
}

scl_str _scl_type_array_to_rt_sig(scl_Array arr) {
	scl_str s = str_of_exact("");
	scl_int strHash = type_id("str");
	scl_int getHash = type_id("get");
	scl_int getSigHash = type_id("(i;)a;");
	scl_int appendHash = type_id("append");
	scl_int appendSigHash = type_id("(cs;)s;");
	for (scl_int i = 0; i < arr->count; i++) {
		scl_str type = (scl_str) virtual_call(arr, "get(i;)a;", i);
		_scl_assert(_scl_is_instance_of(type, strHash), "_scl_type_array_to_rt_sig: type is not a string");
		s = (scl_str) virtual_call(s, "append(cs;)s;", _scl_type_to_rt_sig(type->data));
	}
	return s;
}

scl_str _scl_types_to_rt_signature(scl_str returnType, scl_Array args) {
	const scl_int8* retType = _scl_type_to_rt_sig(returnType->data);
	scl_str argTypes = _scl_type_array_to_rt_sig(args);
	scl_str sig = str_of_exact("(");
	sig = (scl_str) virtual_call(sig, "append(s;)s;", argTypes);
	sig = (scl_str) virtual_call(sig, "append(cs;)s;", ")");
	return (scl_str) virtual_call(sig, "append(cs;)s;", retType);
}

void _scl_trace_remove(void* _) {
	--__cs.trace_index;
}

void _scl_unlock(void* lock_ptr) {
	volatile Struct* lock = *(volatile Struct**) lock_ptr;
	Process$unlock((scl_any) lock);
}

void _scl_throw(scl_any ex) {
	scl_uint* stack_top = (scl_uint*) &stack_top;
	struct GC_stack_base sb;
	GC_get_stack_base(&sb);
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

		struct exception_handler* handler = (struct exception_handler*) stack_top;
		__cs.trace_index = handler->trace_index;
		handler->exception = ex;
		handler->marker = 0;

		longjmp(handler->jmp, 666);
	}

	_scl_security_throw(EX_THROWN, "No exception handler found!");
}

void nativeTrace(void) {
	void* callstack[1024];
	int frames = backtrace(callstack, 1024);
	write(2, "Backtrace:\n", 11);
	backtrace_symbols_fd(callstack, frames, 2);
	write(2, "\n", 1);
}

static inline void printStackTraceOf(scl_Exception e) {
	for (scl_int i = e->stackTrace->count - 1; i >= 0; i--) {
		fprintf(stderr, "    %s\n", ((scl_str*) e->stackTrace->values)[i]->data);
	}

	fprintf(stderr, "\n");
	nativeTrace();
}

_scl_no_return
void _scl_runtime_catch(scl_any _ex) {
	scl_Exception ex = (scl_Exception) _ex;
	scl_str msg = ex->msg;

	fprintf(stderr, "Uncaught %s: %s\n", ex->rtFields.statics->type_name, msg->data);
	printStackTraceOf(ex);
	exit(EX_THROWN);
}

_scl_no_return
void* _scl_oom(scl_uint size) {
	typedef struct Struct_OutOfMemoryError {
		Struct _super;
		scl_str msg;
		scl_any stackTrace;
		scl_str errno_str;
	}* scl_OutOfMemoryError;

	GC_oom_func oldFunc = GC_get_oom_fn();

	scl_OutOfMemoryError err = ALLOC(OutOfMemoryError);
	if (err == nil) {
		fprintf(stderr, "Out of memory! Tried to allocate " SCL_UINT_FMT " bytes.\n", size);
		nativeTrace();
		exit(-1);
	}
	GC_set_oom_fn((GC_oom_func) oldFunc);

	virtual_call(err, "init(s;)V;", str_of_exact("Out of memory!"));
	_scl_throw(err);
}

static int setupCalled = 0;

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

	_scl_stack_new();

	_scl_exception_push();

	setupCalled = 1;
}

int _scl_run(int argc, scl_int8** argv, mainFunc entry, scl_int main_argc) {
#if! __has_attribute(constructor)
	// If we don't support constructor attributes, we need to call _scl_setup() manually
	_scl_setup();
#endif

	scl_any args = nil;
	if (_scl_expect(main_argc, 0)) {
		args = _scl_c_arr_to_scl_array((scl_any*) argv);
	}

	TRY {
		entry(args);
	} else {
		_scl_runtime_catch(_scl_exception_handler.exception);
	}
	return 0;
}

#if defined(__cplusplus)
}
#endif
