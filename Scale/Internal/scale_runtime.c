#include "scale_runtime.h"

#if !defined(SCL_DEFAULT_STACK_FRAME_COUNT)
#define SCL_DEFAULT_STACK_FRAME_COUNT 4096
#endif

const ID_t SclObjectHash = 0x49CC1B82U; // SclObject

// The Stack
tls _scl_stack_t		_stack = {0};

#define unimplemented do { fprintf(stderr, "%s:%d: %s: Not Implemented\n", __FILE__, __LINE__, __FUNCTION__); exit(1) } while (0)

typedef struct Struct {
	// Actual function ptrs for this object
	_scl_lambda*	vtable;
	// Typeinfo for this object
	TypeInfo*	statics;
	// Mutex for this object
	mutex_t			mutex;
} Struct;

typedef struct Struct_Exception {
	Struct rtFields;
	scl_str msg;
	struct Struct_Array* stackTrace;
	scl_str errno_str;
}* scl_Exception;

#define EXCEPTION	((scl_Exception) *(_stack.ex))

typedef struct Struct_NullPointerException {
	struct Struct_Exception self;
} scl_NullPointerException;

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

extern scl_any*			arrays;
extern scl_int			arrays_count;
extern scl_int			arrays_capacity;

extern scl_any*			allocated;
extern scl_int			allocated_count;
extern scl_int			allocated_cap;

extern scl_int*			memsizes;
extern scl_int			memsizes_count;
extern scl_int			memsizes_cap;

extern Struct**			instances;
extern scl_int			instances_count;
extern scl_int			instances_cap;

extern tls scl_any*		stackalloc_arrays;
extern tls scl_int*		stackalloc_array_sizes;
extern tls scl_int		stackalloc_arrays_count;
extern tls scl_int		stackalloc_arrays_cap;

static tls scl_int		_stackallocations_since_cleanup = 0;

scl_int binary_next_smallest_or_equal_index(scl_any* arr, scl_int size, scl_any value) {
	scl_int low = 0;
	scl_int high = size - 1;
	scl_int mid = 0;
	while (low <= high) {
		mid = (low + high) / 2;
		if (arr[mid] < value) {
			low = mid + 1;
		} else if (arr[mid] > value) {
			high = mid - 1;
		} else {
			return mid;
		}
	}
	return high;
}

// Inserts value into array at it's sorted position
// returns the index at which the value was inserted
// will not insert value, if it is already present in the array
scl_int insert_sorted(scl_any** array, scl_int* size, scl_any value, scl_int* cap) {
	if (*array == nil) {
		(*array) = system_allocate(sizeof(scl_any) * (*cap));
	}

	scl_int i = 0;
	if (*size) {
		i = binary_next_smallest_or_equal_index((*array), (*size), value);
	}
	if ((*array)[i] == value) {
		return i;
	}

	if ((*size) + 1 >= (*cap)) {
		(*cap) += 64;
		(*array) = system_realloc((*array), (*cap) * sizeof(scl_any*));
		if ((*array) == nil) {
			_scl_security_throw(EX_BAD_PTR, "realloc() failed");
			exit(1);
		}
	}
	i++;
	// i is now index to insert at
	for (scl_int j = (*size); j > i; j--) {
		(*array)[j] = (*array)[j - 1];
	}
	(*array)[i] = value;
	(*size)++;

	return i;
}

scl_int insert_at(scl_any** array, scl_int* count, scl_int* cap, scl_int index, scl_any value);

void _scl_cleanup_stack_allocations(void) {
	for (scl_int i = 0; i < stackalloc_arrays_count; i++) {
		if (stackalloc_arrays[i] > _stack.sp->v) {
			for (scl_int j = i; j < stackalloc_arrays_count - 1; j++) {
				stackalloc_arrays[j] = stackalloc_arrays[j + 1];
				stackalloc_array_sizes[j] = stackalloc_array_sizes[j + 1];
			}
			stackalloc_arrays_count--;
			i--;
		}
	}
}

void _scl_add_stackallocation(scl_any ptr, scl_int size) {
	scl_int idx = insert_sorted(&stackalloc_arrays, &stackalloc_arrays_count, ptr, &stackalloc_arrays_cap);
	insert_at((scl_any**) &stackalloc_array_sizes, &stackalloc_arrays_count, &stackalloc_arrays_cap, idx, (scl_any) size);

	_stackallocations_since_cleanup++;
	if (_stackallocations_since_cleanup > 16) {
		_scl_cleanup_stack_allocations();
		_stackallocations_since_cleanup = 0;
	}
}

void _scl_stackalloc_check_bounds_or_throw(scl_any ptr, scl_int index) {
	scl_int arridx = _scl_binary_search(stackalloc_arrays, stackalloc_arrays_count, ptr);
	if (arridx < 0) {
		scl_IndexOutOfBoundsException e = ALLOC(IndexOutOfBoundsException);
		scl_int8* str = _scl_alloc(64);
		snprintf(str, 63, "Pointer " SCL_PTR_HEX_FMT " is not a stack allocated array", (scl_int) ptr);
		virtual_call(e, "init(s;)V;", str_of(str));
		_scl_throw(e);
	}
	if (index >= 0 && index < stackalloc_array_sizes[arridx]) {
		return;
	}
	scl_IndexOutOfBoundsException e = ALLOC(IndexOutOfBoundsException);
	scl_int8* str = _scl_alloc(64);
	snprintf(str, 63, "Index " SCL_INT_FMT " out of bounds for array of size " SCL_INT_FMT, index, stackalloc_array_sizes[arridx]);
	virtual_call(e, "init(s;)V;", str_of(str));
	_scl_throw(e);
}

scl_int _scl_stackalloc_size(scl_any ptr) {
	scl_int arridx = _scl_binary_search(stackalloc_arrays, stackalloc_arrays_count, ptr);
	if (arridx < 0) {
		return -1;
	}
	return stackalloc_array_sizes[arridx];
}

scl_int _scl_index_of_stackalloc(scl_any ptr) {
	return _scl_binary_search(stackalloc_arrays, stackalloc_arrays_count, ptr);
}

void _scl_remove_array(scl_any* arr);

void _scl_remove_ptr(scl_any ptr) {
	_scl_remove_array(ptr);
	scl_int index;
	// Finds all indices of the pointer in the table and removes them
	while ((index = _scl_get_index_of_ptr(ptr)) != -1) {
		_scl_remove_ptr_at_index(index);
	}
}

// Returns the next index of a given pointer in the allocated-pointers array
// Returns -1, if the pointer is not in the table
scl_int _scl_get_index_of_ptr(scl_any ptr) {
	return _scl_binary_search((scl_any*) allocated, allocated_count, ptr);
}

// Removes the pointer at the given index and shift everything after left
void _scl_remove_ptr_at_index(scl_int index) {
	for (scl_int i = index; i < allocated_count - 1; i++) {
		allocated[i] = allocated[i + 1];
		memsizes[i] = memsizes[i + 1];
	}
	allocated_count--;
	memsizes_count--;
}

scl_int _scl_sizeof(scl_any ptr) {
	scl_int index = _scl_get_index_of_ptr(ptr);
	if (_scl_expect(index == -1, 0)) return -1;
	return memsizes[index];
}

scl_int insert_at(scl_any** array, scl_int* count, scl_int* cap, scl_int index, scl_any value) {
	if (*array == nil) {
		(*array) = system_allocate(sizeof(scl_any) * (*cap));
	}

	if ((*count) + 1 >= (*cap)) {
		(*cap) += 64;
		(*array) = system_realloc((*array), (*cap) * sizeof(scl_any*));
		if ((*array) == nil) {
			_scl_security_throw(EX_BAD_PTR, "realloc() failed");
			return -1;
		}
	}

	scl_int j;
	for (j = *count; j > index; j--) {
		(*array)[j] = (*array)[j-1];
	}
	(*array)[index] = value;

	(*count)++;
	return index;
}

// Adds a new pointer to the table
void _scl_add_ptr(scl_any ptr, scl_int size) {
	scl_int index = insert_sorted((scl_any**) &allocated, &allocated_count, ptr, &allocated_cap);
	insert_at((scl_any**) &memsizes, &memsizes_count, &memsizes_cap, index, (scl_any) size);
}

// Returns true if the pointer was allocated using _scl_alloc()
scl_int _scl_check_allocated(scl_any ptr) {
	return _scl_get_index_of_ptr(ptr) != -1;
}

void _scl_pointer_destroy(scl_any ptr) {
	_scl_free_struct(ptr);
	_scl_remove_ptr(ptr);
}

// Allocates size bytes of memory
// the memory will always have a size of a multiple of sizeof(scl_int)
scl_any _scl_alloc(scl_int size) {

	// Make size be next biggest multiple of 8
	size = ((size + 7) >> 3) << 3;
	if (size == 0) size = 8;

	// Allocate the memory
	scl_any ptr = GC_malloc(size);
	GC_register_finalizer(ptr, (GC_finalization_proc) &_scl_pointer_destroy, nil, nil, nil);

	// Hard-throw if memory allocation failed
	if (_scl_expect(ptr == nil, 0)) {
		_scl_security_throw(EX_BAD_PTR, "allocate() failed!");
		return nil;
	}

	// Set memory to zero
	memset(ptr, 0, size);

	// Add the pointer to the table
	_scl_add_ptr(ptr, size);
	return ptr;
}

// Reallocates a pointer to a new size
// the memory will always have a size of a multiple of sizeof(scl_int)
scl_any _scl_realloc(scl_any ptr, scl_int size) {
	if (size == 0) {
		scl_NullPointerException* ex = ALLOC(NullPointerException);
		virtual_call(ex, "init()V;");
		ex->self.msg = str_of("realloc() called with size 0");
		_scl_throw(ex);
	}

	// Make size be next biggest multiple of sizeof(scl_int)
	size = ((size + 7) >> 3) << 3;
	if (size == 0) size = 8;

	int wasStruct = _scl_find_index_of_struct(ptr) != -1;

	// If this pointer points to an instance of a struct, quietly deallocate it
	// without calling SclObject:finalize() on it
	_scl_free_struct(ptr);
	// Remove the pointer from the table
	_scl_remove_ptr(ptr);

	// reallocate and reassign it
	// At this point it is unsafe to use the old pointer!
	ptr = GC_realloc(ptr, size);

	// Hard-throw if memory allocation failed
	if (_scl_expect(ptr == nil, 0)) {
		_scl_security_throw(EX_BAD_PTR, "realloc() failed!");
		return nil;
	}

	// Add the pointer to the table
	_scl_add_ptr(ptr, size);

	// Add the pointer back to our struct table, if it was a struct
	if (wasStruct)
		_scl_add_struct(ptr);
	return ptr;
}

// Frees a pointer
// Will do nothing, if the pointer is nil, or wasn't allocated with _scl_alloc()
void _scl_free(scl_any ptr) {
	if (ptr && _scl_check_allocated(ptr)) {
		// If this is an instance, call it's finalizer
		_scl_free_struct(ptr);

		// Remove the pointer from our table
		_scl_remove_ptr(ptr);

		// Finally, free the memory
		// accessing the pointer is now unsafe!
		GC_free(ptr);
	}
}

scl_int8* _scl_strdup(scl_int8*);

// Assert, that 'b' is true
void _scl_assert(scl_int b, scl_int8* msg, ...) {
	if (!b) {
		scl_int8* cmsg = (scl_int8*) _scl_alloc(strlen(msg) * 8);
		va_list list;
		va_start(list, msg);
		vsnprintf(cmsg, strlen(msg) * 8 - 1, msg, list);
		va_end(list);

		snprintf(cmsg, 22 + strlen(cmsg), "Assertion failed: %s", _scl_strdup(cmsg));
		scl_AssertError err = ALLOC(AssertError);
		virtual_call(err, "init(s;)V;", str_of(cmsg));

		_scl_throw(err);
	}
}

void builtinUnreachable(void) {
	scl_UnreachableError err = ALLOC(UnreachableError);
	virtual_call(err, "init(s;)V;", str_of("Unreachable!"));

	_scl_throw(err);
}

// Hard-throw an exception
_scl_no_return void _scl_security_throw(int code, scl_int8* msg, ...) {
	printf("\n");

	va_list args;
	va_start(args, msg);
	char* str = malloc(strlen(msg) * 8);
	vsnprintf(str, strlen(msg) * 8, msg, args);
	printf("Exception: %s\n", str);

	va_end(args);

	if (errno) {
		printf("errno: %s\n", strerror(errno));
	}
	print_stacktrace();

	_scl_security_safe_exit(code);
}

// exit()
_scl_no_return void _scl_security_safe_exit(int code) {

	// exit
	exit(code);
}

scl_int8* _scl_strndup(scl_int8* str, scl_int len) {
	scl_int8* new = _scl_alloc(len + 1);
	strncpy(new, str, len);
	return new;
}

scl_int8* _scl_strdup(scl_int8* str) {
	scl_int len = strlen(str);
	return _scl_strndup(str, len);
}

scl_str _scl_create_string(scl_int8* data) {
	scl_str self = ALLOC(str);
	if (_scl_expect(self == nil, 0)) {
		_scl_security_throw(EX_BAD_PTR, "Failed to allocate memory for cstr '%s'\n", data);
		return nil;
	}
	self->length = strlen(data);
	self->hash = id(data);
	self->data = _scl_strndup(data, self->length);
	return self;
}

scl_any _scl_cvarargs_to_array(va_list args, scl_int count) {
	scl_Array arr = ALLOC(ReadOnlyArray);
	_scl_struct_allocation_failure(REINTERPRET_CAST(scl_int, arr), "ReadOnlyArray");
	virtual_call(arr, "init(i;)V;", count);
	for (scl_int i = 0; i < count; i++) {
		arr->values[i] = va_arg(args, scl_any);
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

void _scl_handle_signal(scl_int sig_num, siginfo_t* sig_info, void* ucontext) {
	scl_int8* signalString;
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
		if (
			_stack.sp > (_stack.bp + SCL_DEFAULT_STACK_FRAME_COUNT) ||
			_stack.tp > (_stack.tbp + SCL_DEFAULT_STACK_FRAME_COUNT)
		) {
			signalString = "stack overflow";
		} else {
			signalString = "segmentation violation";
		}
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

// final signal handler
// if we get here, something has gone VERY wrong
void _scl_default_signal_handler(scl_int sig_num) {
	scl_int8* signalString;
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
		if (
			_stack.sp > (_stack.bp + SCL_DEFAULT_STACK_FRAME_COUNT) ||
			_stack.tp > (_stack.tbp + SCL_DEFAULT_STACK_FRAME_COUNT)
		) {
			signalString = "stack overflow";
		} else {
			signalString = "segmentation violation";
		}
	} else {
		signalString = "other signal";
	}

	printf("\n");

	printf("Signal: %s\n", signalString);
	if (errno) {
		printf("errno: %s\n", strerror(errno));
	}
	print_stacktrace();
	printf("Stack address: %p\n", _stack.bp);
	if (_stack.sp) {
		printf("Stack:\n");
		_scl_frame_t* frame = _stack.sp;
		while (frame > _stack.bp) {
			frame--;
			printf("   " SCL_INT_FMT ": 0x" SCL_INT_HEX_FMT ", " SCL_INT_FMT "\n", (frame - _stack.bp) / sizeof(_scl_frame_t), frame->i, frame->i);
		}
		printf("\n");
	}

	exit(sig_num);
}

// Converts C nil-terminated array to ReadOnlyArray-instance
scl_any _scl_c_arr_to_scl_array(scl_any arr[]) {
	struct Struct_Array* array = ALLOC(ReadOnlyArray);
	scl_int cap = 0;
	while (arr[cap] != nil) {
		cap++;
	}

	array->capacity = cap;
	array->initCapacity = cap;
	array->count = 0;
	array->values = _scl_new_array(cap * sizeof(scl_str));
	for (scl_int i = 0; i < cap; i++) {
		((scl_str*) array->values)[(scl_int) array->count++] = _scl_create_string(arr[i]);
	}
	return array;
}

scl_int _scl_stack_size(void) {
	return (_stack.sp - _stack.bp) / sizeof(_scl_frame_t);
}

void _scl_sleep(scl_int millis) {
	sleep(millis);
}

#if !defined(__pure2)
#define __pure2
#endif

__pure2 const ID_t id(const char* data) {
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
		hash = _scl_rotl(hash, 5) ^ ((char*) obj)[i];
	}
	Process$unlock((volatile scl_any) obj);
	return hash;
}

scl_any _scl_atomic_clone(scl_any ptr) {
	scl_int size = _scl_sizeof(ptr);
	scl_any clone = _scl_alloc(size);

	if (_scl_is_instance_of(ptr, SclObjectHash)) {
		_scl_add_struct(clone);
	}

	memcpy(clone, ptr, size);
	return clone;
}

// returns the method handle of a method on a struct, or a parent struct
scl_any _scl_get_method_on_type(scl_any type, ID_t method, ID_t signature, int onSuper) {
	if (_scl_expect(!_scl_is_instance_of(type, SclObjectHash), 0)) {
		_scl_security_throw(EX_CAST_ERROR, "Tried getting method on non-struct type (%p)", type);
	}
	Struct* instance = (Struct*) type;
	scl_int index = -1;
	if (onSuper) {
		index = _scl_search_method_index((scl_any*) instance->statics->super->vtable, -1, method, signature);
	} else {
		index = _scl_search_method_index((scl_any*) instance->statics->vtable, -1, method, signature);
	}
	if (index >= 0) {
		return instance->statics->vtable[index].rt_ptr;
	}
	return nil;
}

static scl_int8* substr_of(scl_int8* str, size_t len, size_t beg, size_t end);

static size_t str_index_of(scl_int8* str, char c) {
	size_t i = 0;
	while (str[i] != c) {
		i++;
	}
	return i;
}

static size_t str_last_index_of(scl_int8* str, char c) {
	size_t i = strlen(str) - 1;
	while (str[i] != c) {
		i--;
	}
	return i;
}

static size_t str_num_of_occurences(scl_int8* str, char c) {
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

static inline scl_any virtual_call_impl(scl_int onSuper, scl_any instance, scl_int8* methodIdentifier, va_list ap);

scl_any virtual_call(scl_any instance, scl_int8* methodIdentifier, ...) {
	va_list ap;
	va_start(ap, methodIdentifier);
	scl_any result = virtual_call_impl(0, instance, methodIdentifier, ap);
	va_end(ap);
	return result;
}

scl_any virtual_call_super(scl_any instance, scl_int8* methodIdentifier, ...) {
	va_list ap;
	va_start(ap, methodIdentifier);
	scl_any result = virtual_call_impl(1, instance, methodIdentifier, ap);
	va_end(ap);
	return result;
}

scl_float virtual_callf(scl_any instance, scl_int8* methodIdentifier, ...) {
	va_list ap;
	va_start(ap, methodIdentifier);
	scl_any result = virtual_call_impl(0, instance, methodIdentifier, ap);
	va_end(ap);
	return REINTERPRET_CAST(scl_float, result);
}

scl_float virtual_call_superf(scl_any instance, scl_int8* methodIdentifier, ...) {
	va_list ap;
	va_start(ap, methodIdentifier);
	scl_any result = virtual_call_impl(1, instance, methodIdentifier, ap);
	va_end(ap);
	return REINTERPRET_CAST(scl_float, result);
}

void dumpStack();

#define strequals(a, b) (strcmp(a, b) == 0)
#define strstarts(a, b) (strncmp((a), (b), strlen((b))) == 0)

static inline scl_any virtual_call_impl(scl_int onSuper, scl_any instance, scl_int8* methodIdentifier, va_list ap) {
	size_t methodLen = strlen(methodIdentifier);
	scl_int8* methodName = substr_of(methodIdentifier, methodLen, 0, str_index_of(methodIdentifier, '('));
	scl_int8* signature = substr_of(methodIdentifier, methodLen, str_index_of(methodIdentifier, '('), methodLen);
	ID_t methodNameHash = id(methodName);
	ID_t signatureHash = id(signature);
	scl_int8* args = substr_of(methodIdentifier, methodLen, str_index_of(methodIdentifier, '(') + 1, str_last_index_of(methodIdentifier, ')'));
	scl_int argCount = str_num_of_occurences(args, ';');
	free(args);
	for (scl_int i = 0; i < argCount; i++) {
		(_stack.sp++)->v = va_arg(ap, scl_any);
	}
	(_stack.sp++)->v = instance;

	_scl_lambda m = _scl_get_method_on_type(instance, methodNameHash, signatureHash, onSuper);
	if (_scl_expect(m == nil, 0)) {
		_scl_security_throw(EX_BAD_PTR, "Method '%s%s' not found on type '%s'", methodName, signature, ((Struct*) instance)->statics->type_name);
	}
	free(methodName);
	free(signature);
	scl_int8* returnType = substr_of(methodIdentifier, methodLen, str_last_index_of(methodIdentifier, ')') + 1, methodLen);
	int tmp = strequals(returnType, "V;");
	free(returnType);

	m();
	return tmp ? nil : (--_stack.sp)->v;
}

void _scl_call_method_or_throw(scl_any instance_, ID_t method, ID_t signature, int on_super, scl_int8* method_name, scl_int8* signature_str) {
	if (_scl_expect(!_scl_is_instance_of(instance_, SclObjectHash), 0)) {
		_scl_security_throw(EX_BAD_PTR, "Method call on non-object");
	}
	Struct* instance = (Struct*) instance_; 
	_scl_lambda m = _scl_get_method_on_type(instance, method, signature, on_super);
	if (_scl_expect(m == nil, 0)) {
		_scl_security_throw(EX_BAD_PTR, "Method '%s%s' not found on type '%s'", method_name, signature_str, instance->statics->type_name);
	}
	m();
}

// adds an instance to the table of tracked instances
scl_any _scl_add_struct(scl_any ptr) {
	scl_int index = insert_sorted((scl_any**) &instances, &instances_count, ptr, &instances_cap);
	return index == -1 ? nil : instances[index];
}

mutex_t _scl_mutex_new(void) {
	mutex_t m = GC_malloc(sizeof(pthread_mutex_t));
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(m, &attr);
	return m;
}

// creates a new instance with a size of 'size'
scl_any _scl_alloc_struct(scl_int size, const TypeInfo* statics) {

	// Allocate the memory
	scl_any ptr = _scl_alloc(size);

	if (_scl_expect(ptr == nil, 0)) {
		return nil;
	}

	// Callstack
	((Struct*) ptr)->mutex = _scl_mutex_new();

	// Static members
	((Struct*) ptr)->statics = (TypeInfo*) statics;
	
	// Vtable
	((Struct*) ptr)->vtable = (_scl_lambda*) statics->vtable_fast;

	// Add struct to allocated table
	return _scl_add_struct(ptr);
}

// Removes an instance from the allocated table by index
void _scl_struct_map_remove(scl_int index) {
	for (scl_int i = index; i < instances_count - 1; i++) {
	   instances[i] = instances[i + 1];
	}
	instances_count--;
}

// Returns the next index of an instance in the allocated table
// Returns -1, if not in table
scl_int _scl_find_index_of_struct(scl_any ptr) {
	if (_scl_expect(ptr == nil, 0)) return -1;
	return _scl_binary_search((scl_any*) instances, instances_count, ptr);
}

// Removes an instance from the allocated table without calling its finalizer
void _scl_free_struct(scl_any ptr) {
	scl_int i = _scl_find_index_of_struct(ptr);
	while (i != -1) {
		_scl_struct_map_remove(i);
		i = _scl_find_index_of_struct(ptr);
	}
}

// Returns true, if the instance is of a given struct type
scl_int _scl_is_instance_of(scl_any ptr, ID_t typeId) {
	if (_scl_expect(ptr == nil, 0)) return 0; // ptr is null

	int isStruct = _scl_binary_search((scl_any*) instances, instances_count, ptr) != -1;

	if (_scl_expect(!isStruct, 0)) return 0; // ptr is not an instance and cast to non primitive type attempted

	Struct* ptrStruct = (Struct*) ptr;

	if (typeId == SclObjectHash) return 1; // cast to SclObject

	// check super types
	for (const TypeInfo* super = ptrStruct->statics; super != nil; super = super->super) {
		if (super->type == typeId) return 1;
	}

	return 0; // invalid cast
}

scl_int _scl_binary_search(scl_any* arr, scl_int count, scl_any val) {
	scl_int left = 0;
	scl_int right = count - 1;

	while (left <= right) {
		scl_int mid = (left + right) / 2;
		if (arr[mid] == val) {
			return mid;
		} else if (arr[mid] < val) {
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}

	return -1;
}

scl_int _scl_search_method_index(scl_any* methods, const scl_int count, ID_t id, ID_t sig) {
	if (_scl_expect(methods == nil, 0)) return -1;
	struct _scl_methodinfo* methods_ = (struct _scl_methodinfo*) methods;

	for (scl_int i = 0; (count == -1 ? (*(scl_int*) &(methods_[i].ptr)) : i < count); i++) {
		if (methods_[i].pure_name == id && (sig == 0 || methods_[i].signature == sig)) {
			return i;
		}
	}
	return -1;
}

typedef void (*_scl_sigHandler)(scl_int);

int _scl_gc_is_disabled(void) {
	return GC_is_disabled();
}

int _scl_gc_pthread_create(pthread_t* t, const pthread_attr_t* a, scl_any(*f)(scl_any), scl_any arg) {
	return GC_pthread_create(t, a, f, arg);
}

int _scl_gc_pthread_join(pthread_t t, scl_any* r) {
	return GC_pthread_join(t, r);
}

void _scl_set_signal_handler(_scl_sigHandler handler, scl_int sig) {
	if (_scl_expect(sig < 0 || sig >= 32, 0)) {
		struct Struct_InvalidSignalException {
			struct Struct_Exception self;
			scl_int sig;
		};
		scl_Exception e = ALLOC(InvalidSignalException);
		virtual_call(e, "init(i;)V;", sig);

		scl_int8* p = (scl_int8*) _scl_alloc(64);
		snprintf(p, 64, "Invalid signal: " SCL_INT_FMT, sig);
		e->msg = str_of(p);
		
		_scl_throw(e);
	}
	signal(sig, (void(*)(int)) handler);
}

void _scl_reset_signal_handler(scl_int sig) {
	if (_scl_expect(sig < 0 || sig >= 32, 0)) {
		struct Struct_InvalidSignalException {
			struct Struct_Exception self;
			scl_int sig;
		};
		scl_Exception e = ALLOC(InvalidSignalException);
		virtual_call(e, "init(i;)V;", sig);

		scl_int8* p = (scl_int8*) _scl_alloc(64);
		snprintf(p, 64, "Invalid signal: " SCL_INT_FMT, sig);
		e->msg = str_of(p);
		
		_scl_throw(e);
	}
	signal(sig, (void(*)(int)) _scl_default_signal_handler);
}

void _scl_set_up_signal_handler(void) {
	static struct sigaction act = {
		.sa_sigaction = (void(*)(int, siginfo_t*, void*)) _scl_handle_signal,
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
}

void _scl_resize_stack(void) {}

void _scl_exception_push(void) {
	++_stack.et;
	++_stack.ex;
	++_stack.jmp;
	*(_stack.sp_save++) = _stack.sp;
}

void _scl_exception_drop(void) {
	--_stack.et;
	--_stack.ex;
	--_stack.jmp;
	--_stack.sp_save;
}

scl_int8** _scl_platform_get_env(void) {
#if defined(WIN) && (_MSC_VER >= 1900)
	return *__p__environ();
#else
	extern char** environ;
	return environ;
#endif
}

void _scl_check_layout_size(scl_any ptr, scl_int layoutSize, scl_int8* layout) {
	scl_int size = _scl_sizeof(ptr);
	if (size < layoutSize) {
		scl_int8* msg = (scl_int8*) GC_malloc(sizeof(scl_int8) * strlen(layout) + 256);
		snprintf(msg, 256 + strlen(layout), "Layout '%s' requires more memory than the pointer has available (required: " SCL_INT_FMT " found: " SCL_INT_FMT ")", layout, layoutSize, size);
		_scl_assert(0, msg);
	}
}

void _scl_check_not_nil_argument(scl_int val, scl_int8* name) {
	if (val == 0) {
		scl_int8* msg = (scl_int8*) GC_malloc(sizeof(scl_int8) * strlen(name) + 64);
		snprintf(msg, 64 + strlen(name), "Argument '%s' is nil", name);
		_scl_assert(0, msg);
	}
}

#define as(type, val) ((scl_ ## type) _scl_checked_cast((scl_any) val, hash_of(#type), #type))

scl_any _scl_checked_cast(scl_any instance, ID_t target_type, scl_int8* target_type_name) {
	if (!_scl_is_instance_of(instance, target_type)) {
		typedef struct Struct_CastError {
			Struct rtFields;
			scl_str msg;
			struct Struct_Array* stackTrace;
			scl_str errno_str;
		} _scl_CastError;

		scl_int size;
		scl_int8* cmsg;
		if (instance == nil) {
			size = 96 + strlen(target_type_name);
			cmsg = (scl_int8*) _scl_alloc(size);
			snprintf(cmsg, size - 1, "Cannot cast nil to type '%s'\n", target_type_name);
			_scl_CastError* err = ALLOC(CastError);
			virtual_call(err, "init(s;)V;", str_of(cmsg));
			_scl_throw(err);
		}

		int isStruct = _scl_binary_search((scl_any*) instances, instances_count, instance) != -1;

		if (isStruct) {
			size = 64 + strlen(((Struct*) instance)->statics->type_name) + strlen(target_type_name);
			cmsg = (scl_int8*) _scl_alloc(size);
			snprintf(cmsg, size - 1, "Cannot cast instance of struct '%s' to type '%s'\n", ((Struct*) instance)->statics->type_name, target_type_name);
		} else {
			size = 96 + strlen(target_type_name);
			cmsg = (scl_int8*) _scl_alloc(size);
			snprintf(cmsg, size - 1, "Cannot cast non-object to type '%s'\n", target_type_name);
		}
		_scl_CastError* err = ALLOC(CastError);
		virtual_call(err, "init(s;)V;", str_of(cmsg));
		_scl_throw(err);
	}
	return instance;
}

void _scl_not_nil_cast(scl_int val, scl_int8* name) {
	if (val == 0) {
		scl_int8* msg = (scl_int8*) GC_malloc(sizeof(scl_int8) * strlen(name) + 64);
		snprintf(msg, 64 + strlen(name), "Nil cannot be cast to non-nil type '%s'!", name);
		_scl_assert(0, msg);
	}
}

void _scl_struct_allocation_failure(scl_int val, scl_int8* name) {
	if (val == 0) {
		scl_int8* msg = (scl_int8*) GC_malloc(sizeof(scl_int8) * strlen(name) + 64);
		snprintf(msg, 64 + strlen(name), "Failed to allocate memory for struct '%s'!", name);
		_scl_assert(0, msg);
	}
}

void _scl_nil_ptr_dereference(scl_int val, scl_int8* name) {
	if (val == 0) {
		scl_int8* msg = (scl_int8*) GC_malloc(sizeof(scl_int8) * strlen(name) + 64);
		snprintf(msg, 64 + strlen(name), "Tried dereferencing nil pointer '%s'!", name);
		_scl_assert(0, msg);
	}
}

void _scl_check_not_nil_store(scl_int val, scl_int8* name) {
	if (val == 0) {
		scl_int8* msg = (scl_int8*) GC_malloc(sizeof(scl_int8) * strlen(name) + 64);
		snprintf(msg, 64 + strlen(name), "Nil cannot be stored in non-nil variable '%s'!", name);
		_scl_assert(0, msg);
	}
}

void _scl_not_nil_return(scl_int val, scl_int8* name) {
	if (val == 0) {
		scl_int8* msg = (scl_int8*) GC_malloc(sizeof(scl_int8) * strlen(name) + 64);
		snprintf(msg, 64 + strlen(name), "Tried returning nil from function returning not-nil type '%s'!", name);
		_scl_assert(0, msg);
	}
}

void _scl_stack_new(void) {
	// These use C's malloc, keep it that way
	// They should NOT be affected by any future
	// stuff we might do with _scl_alloc()
	if (_scl_expect((_stack.bp = _stack.sp = GC_malloc_uncollectable(sizeof(_scl_frame_t) * SCL_DEFAULT_STACK_FRAME_COUNT)) == 0, 0))			_scl_security_throw(EX_BAD_PTR, "Failed to allocate memory for stack!");
	if (_scl_expect((_stack.tbp = _stack.tp = system_allocate(sizeof(scl_int8*) * SCL_DEFAULT_STACK_FRAME_COUNT)) == 0, 0))						_scl_security_throw(EX_BAD_PTR, "Failed to allocate memory for trace stack!");

	if (_scl_expect((_stack.ex_base = _stack.ex = GC_malloc_uncollectable(sizeof(scl_any) * SCL_DEFAULT_STACK_FRAME_COUNT)) == 0, 0))			_scl_security_throw(EX_BAD_PTR, "Failed to allocate memory for exception stack!");
	if (_scl_expect((_stack.et_base = _stack.et = system_allocate(sizeof(scl_any) * SCL_DEFAULT_STACK_FRAME_COUNT)) == 0, 0))					_scl_security_throw(EX_BAD_PTR, "Failed to allocate memory for exception trace stack!");
	if (_scl_expect((_stack.jmp_base = _stack.jmp = system_allocate(sizeof(jmp_buf) * SCL_DEFAULT_STACK_FRAME_COUNT)) == 0, 0))					_scl_security_throw(EX_BAD_PTR, "Failed to allocate memory for exception jump stack!");
	if (_scl_expect((_stack.sp_save_base = _stack.sp_save = system_allocate(sizeof(_scl_frame_t*) * SCL_DEFAULT_STACK_FRAME_COUNT)) == 0, 0))	_scl_security_throw(EX_BAD_PTR, "Failed to allocate memory for stack pointer save stack!");
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
	GC_free(_stack.bp);
	system_free(_stack.tbp);

	system_free(_stack.et_base);
	GC_free(_stack.ex_base);
	system_free(_stack.jmp_base);
	
	memset(&_stack, 0, sizeof(_stack));
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

// Region: intrinsics

void Process$lock(volatile scl_any obj) {
	if (_scl_expect(!obj, 0)) return;
	pthread_mutex_lock(((volatile Struct*) obj)->mutex);
}

void Process$unlock(volatile scl_any obj) {
	if (_scl_expect(!obj, 0)) return;
	pthread_mutex_unlock(((volatile Struct*) obj)->mutex);
}

scl_int8* GarbageCollector$getImplementation0(void) {
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
		virtual_call(e, "init(s;)V;", str_of("Array size must not be less than 1"));
		_scl_throw(e);
	}
	scl_any* arr = _scl_alloc(num_elems * elem_size + sizeof(scl_int));
	((scl_int*) arr)[0] = num_elems;
	insert_sorted(&arrays, &arrays_count, arr, &arrays_capacity);
	return arr + 1;
}

scl_any* _scl_new_array(scl_int num_elems) {
	return _scl_new_array_by_size(num_elems, sizeof(scl_any));
}

scl_int _scl_is_array(scl_any* arr) {
	if (_scl_expect(!arr, 0)) return 0;
	return _scl_binary_search(arrays, arrays_count, arr - 1) != -1;
}

void remove_at(scl_any** arr, scl_int* count, scl_int index) {
	if (_scl_expect(index < 0 || index >= *count, 0)) return;
	(*count)--;
	for (scl_int i = index; i < *count; i++) {
		arr[i] = arr[i + 1];
	}
}

void _scl_remove_array(scl_any* arr) {
	if (_scl_expect(!arr, 0)) return;
	scl_int index = _scl_binary_search(arrays, arrays_count, arr - 1);
	if (_scl_expect(index == -1, 0)) return;
	remove_at(&arrays, &arrays_count, index);
}

scl_any* _scl_multi_new_array_by_size(scl_int dimensions, scl_int sizes[dimensions], scl_int elem_size) {
	if (_scl_expect(dimensions < 1, 0)) {
		scl_InvalidArgumentException e = ALLOC(InvalidArgumentException);
		virtual_call(e, "init(s;)V;", str_of("Array dimensions must not be less than 1"));
		_scl_throw(e);
	}
	if (dimensions == 1) {
		return _scl_new_array_by_size(sizes[0], elem_size);
	}
	scl_any* arr = _scl_alloc(sizes[0] * sizeof(scl_any) + sizeof(scl_int));
	((scl_int*) arr)[0] = sizes[0];
	for (scl_int i = 1; i <= sizes[0]; i++) {
		arr[i] = _scl_multi_new_array_by_size(dimensions - 1, &(sizes[1]), elem_size);
	}
	return arr + 1;
}

scl_any* _scl_multi_new_array(scl_int dimensions, scl_int sizes[dimensions]) {
	return _scl_multi_new_array_by_size(dimensions, sizes, sizeof(scl_any));
}

scl_int _scl_array_size_unchecked(scl_any* arr) {
	if (_scl_index_of_stackalloc(arr) >= 0) {
		return _scl_stackalloc_size(arr);
	}
	return *((scl_int*) arr - 1);
}

scl_int _scl_array_size(scl_any* arr) {
	if (_scl_expect(arr == nil, 0)) {
		scl_NullPointerException* e = ALLOC(NullPointerException);
		virtual_call(e, "init(s;)V;", str_of("nil pointer detected"));
		_scl_throw(e);
	}
	if (_scl_index_of_stackalloc(arr) >= 0) {
		return _scl_stackalloc_size(arr);
	}
	if (_scl_expect(!_scl_is_array(arr), 0)) {
		scl_InvalidArgumentException e = ALLOC(InvalidArgumentException);
		virtual_call(e, "init(s;)V;", str_of("Array must be initialized with 'new[]'"));
		_scl_throw(e);
	}
	return *((scl_int*) arr - 1);
}

void _scl_array_check_bounds_or_throw(scl_any* arr, scl_int index) {
	if (_scl_expect(arr == nil, 0)) {
		scl_NullPointerException* e = ALLOC(NullPointerException);
		virtual_call(e, "init(s;)V;", str_of("nil pointer detected"));
		_scl_throw(e);
	}
	if (_scl_index_of_stackalloc(arr) >= 0) {
		return _scl_stackalloc_check_bounds_or_throw(arr, index);
	}
	if (_scl_expect(!_scl_is_array(arr), 0)) {
		scl_InvalidArgumentException e = ALLOC(InvalidArgumentException);
		virtual_call(e, "init(s;)V;", str_of("Array must be initialized with 'new[]')"));
		_scl_throw(e);
	}
	scl_int size = *((scl_int*) arr - 1);
	if (index < 0 || index >= size) {
		scl_IndexOutOfBoundsException e = ALLOC(IndexOutOfBoundsException);
		scl_int8* str = _scl_alloc(64);
		snprintf(str, 63, "Index " SCL_INT_FMT " out of bounds for array of size " SCL_INT_FMT, index, size);
		virtual_call(e, "init(s;)V;", str_of(str));
		_scl_throw(e);
	}
}

scl_any* _scl_array_resize(scl_any* arr, scl_int new_size) {
	if (_scl_expect(arr == nil, 0)) {
		scl_NullPointerException* e = ALLOC(NullPointerException);
		virtual_call(e, "init(s;)V;", str_of("nil pointer detected"));
		_scl_throw(e);
	}
	if (_scl_expect(_scl_index_of_stackalloc(arr) >= 0, 0)) {
		scl_InvalidArgumentException e = ALLOC(InvalidArgumentException);
		virtual_call(e, "init(s;)V;", str_of("Cannot resize stack-allocated array"));
		_scl_throw(e);
	}
	if (_scl_expect(!_scl_is_array(arr), 0)) {
		scl_InvalidArgumentException e = ALLOC(InvalidArgumentException);
		virtual_call(e, "init(s;)V;", str_of("Array must be initialized with 'new[]'"));
		_scl_throw(e);
	}
	if (_scl_expect(new_size < 1, 0)) {
		scl_InvalidArgumentException e = ALLOC(InvalidArgumentException);
		virtual_call(e, "init(s;)V;", str_of("New array size must not be less than 1"));
		_scl_throw(e);
	}
	scl_int size = *((scl_int*) arr - 1);
	scl_any* new_arr = _scl_realloc(arr - 1, new_size * sizeof(scl_any) + sizeof(scl_int));
	insert_sorted(&arrays, &arrays_count, new_arr, &arrays_capacity);
	((scl_int*) new_arr)[0] = new_size;
	return new_arr + 1;
}

scl_any* _scl_array_sort(scl_any* arr) {
	if (_scl_expect(arr == nil, 0)) {
		scl_NullPointerException* e = ALLOC(NullPointerException);
		virtual_call(e, "init(s;)V;", str_of("nil pointer detected"));
		_scl_throw(e);
	}
	if (_scl_expect(!_scl_is_array(arr) && _scl_index_of_stackalloc(arr) < 0, 0)) {
		scl_InvalidArgumentException e = ALLOC(InvalidArgumentException);
		virtual_call(e, "init(s;)V;", str_of("Array must be initialized with 'new[]'"));
		_scl_throw(e);
	}
	scl_int size = _scl_array_size_unchecked(arr);
	// binary sort
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
		scl_NullPointerException* e = ALLOC(NullPointerException);
		virtual_call(e, "init(s;)V;", str_of("nil pointer detected"));
		_scl_throw(e);
	}
	if (_scl_expect(!_scl_is_array(arr) && _scl_index_of_stackalloc(arr) < 0, 0)) {
		scl_InvalidArgumentException e = ALLOC(InvalidArgumentException);
		virtual_call(e, "init(s;)V;", str_of("Array must be initialized with 'new[]'"));
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
	scl_int8* str = _scl_alloc(32);
	snprintf(str, 32, SCL_INT_FMT, i);
	scl_str s = str_of(str);
	return s;
}

scl_str _scl_array_to_string(scl_any* arr) {
	if (_scl_expect(arr == nil, 0)) {
		scl_NullPointerException* e = ALLOC(NullPointerException);
		virtual_call(e, "init(s;)V;", str_of("nil pointer detected"));
		_scl_throw(e);
	}
	if (_scl_expect(!_scl_is_array(arr) && _scl_index_of_stackalloc(arr) < 0, 0)) {
		scl_InvalidArgumentException e = ALLOC(InvalidArgumentException);
		virtual_call(e, "init(s;)V;", str_of("Array must be initialized with 'new[]'"));
		_scl_throw(e);
	}
	scl_int size = _scl_array_size_unchecked(arr);
	scl_str s = str_of("[");
	for (scl_int i = 0; i < size; i++) {
		if (i) {
			s = virtual_call(s, "append(s;)s;", str_of(", "));
		}
		scl_str tmp;
		if (_scl_is_instance_of(arr[i], SclObjectHash)) {
			tmp = virtual_call(arr[i], "toString()s;");
		} else {
			tmp = int_to_string((scl_int) arr[i]);
		}
		s = virtual_call(s, "append(s;)s;", tmp);
	}
	return virtual_call(s, "append(s;)s;", str_of("]"));
}

static scl_int8* substr_of(scl_int8* str, size_t len, size_t beg, size_t end) {
	scl_int8* sub = malloc(len);
	strcpy(sub, str + beg);
	sub[end - beg] = '\0';
	return sub;
}

scl_int8* _scl_type_to_rt_sig(scl_int8* type) {
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
		scl_int8* tmp = _scl_alloc(len + 2);
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
	scl_int8* tmp = _scl_alloc(len + 3);
	snprintf(tmp, len + 3, "L%s;", type);
	return tmp;
}

scl_str _scl_type_array_to_rt_sig(scl_Array arr) {
	scl_str s = str_of("");
	scl_int strHash = id("str");
	scl_int getHash = id("get");
	scl_int getSigHash = id("(i;)a;");
	scl_int appendHash = id("append");
	scl_int appendSigHash = id("(cs;)s;");
	for (scl_int i = 0; i < arr->count; i++) {
		scl_str type = virtual_call(arr, "get(i;)a;", i);
		_scl_assert(_scl_is_instance_of(type, strHash), "_scl_type_array_to_rt_sig: type is not a string");
		s = virtual_call(s, "append(cs;)s;", _scl_type_to_rt_sig(type->data));
	}
	return s;
}

scl_str _scl_types_to_rt_signature(scl_str returnType, scl_Array args) {
	scl_int8* retType = _scl_type_to_rt_sig(returnType->data);
	scl_str argTypes = _scl_type_array_to_rt_sig(args);
	scl_str sig = str_of("(");
	sig = virtual_call(sig, "append(s;)s;", argTypes);
	sig = virtual_call(sig, "append(cs;)s;", ")");
	return virtual_call(sig, "append(cs;)s;", retType);
}

scl_int8** _scl_callstack_push(void) {
	return _stack.tp++;
}

void _scl_stack_resize_fit(scl_int sz) {
	_stack.sp += sz;
}

void _scl_create_stack(void) {
	// These use C's malloc, keep it that way
	// They should NOT be affected by any future
	// stuff we might do with _scl_alloc()
	_scl_stack_new();

	allocated = system_allocate(allocated_cap * sizeof(scl_any));
	memsizes = system_allocate(memsizes_cap * sizeof(scl_int));
	instances = system_allocate(instances_cap * sizeof(Struct*));
}

static void _scl_unwind(scl_any ex) {
	if (_scl_is_instance_of(ex, id("Error"))) {
		_stack.jmp = _stack.jmp_base;
		_stack.ex = _stack.ex_base;
		_stack.et = _stack.et_base;
		_stack.sp = *(_stack.sp_save_base);
	} else {
		--_stack.jmp;
		--_stack.ex;
		--_stack.et;
		_stack.sp = *(--_stack.sp_save);
	}
	if (_scl_expect(_stack.jmp < _stack.jmp_base, 0)) {
		_stack.jmp = _stack.jmp_base;
		_stack.ex = _stack.ex_base;
		_stack.et = _stack.et_base;
		_stack.sp = *(_stack.sp_save_base);
	}
	*(_stack.ex) = ex;
	_stack.tp = *(_stack.et);
}

void _scl_throw(scl_any ex) {
	_scl_unwind(ex);
	longjmp(*(_stack.jmp), 666);
}

void nativeTrace(void) {
	void* callstack[1024];
	int frames = backtrace(callstack, 1024);
	write(STDERR_FILENO, "Backtrace:\n", 11);
	backtrace_symbols_fd(callstack, frames, STDERR_FILENO);
	write(STDERR_FILENO, "\n", 1);
}

static inline void printStackTraceOf(scl_Exception e) {
	for (scl_int i = e->stackTrace->count - 1; i >= 0; i--) {
		fprintf(stderr, "  %s\n", ((scl_str*) e->stackTrace->values)[i]->data);
	}

	fprintf(stderr, "\n");
	nativeTrace();
}

_scl_no_return
void _scl_runtime_catch() {
	if (EXCEPTION == nil) {
		_scl_security_throw(EX_BAD_PTR, "nil exception pointer");
	}
	scl_str msg = EXCEPTION->msg;

	fprintf(stderr, "Uncaught %s: %s\n", EXCEPTION->rtFields.statics->type_name, msg->data);
	printStackTraceOf(EXCEPTION);
	if (errno) {
		fprintf(stderr, "errno: %s\n", strerror(errno));
	}
	exit(EX_THROWN);
}

_scl_no_return
void* _scl_oom(scl_uint size) {
	fprintf(stderr, "Out of memory! Tried to allocate " SCL_UINT_FMT " bytes.\n", size);
	nativeTrace();
	exit(-1);
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
	char *byte = (char*) &word;
	if (!byte[0]) {
		fprintf(stderr, "Invalid byte order detected!");
		exit(-1);
	}
#endif

	GC_INIT();
	GC_set_oom_fn((GC_oom_func) &_scl_oom);

	// Register signal handler for all available signals
	_scl_set_up_signal_handler();

	_scl_create_stack();

	_scl_exception_push();

	setupCalled = 1;
}

int _scl_run(int argc, char** argv, mainFunc entry) {
	assert(_stack.bp);
	assert(_stack.tbp);

	// Convert argv and envp from native arrays to Scale arrays
	scl_any args = _scl_c_arr_to_scl_array((scl_any*) argv);
	scl_any env = _scl_c_arr_to_scl_array((scl_any*) _scl_platform_get_env());

	extern scl_int8* argv0;

	argv0 = argv[0];

	*(_stack.et - 1) = _stack.tp = _stack.tbp;
	TRY {
		entry(args, env);
	} else {
		_scl_runtime_catch();
	}
	return 0;
}
