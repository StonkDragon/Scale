#include "scale_runtime.h"

const ID_t SclObjectHash = 0xC9CCFE34U; // SclObject
const ID_t toStringHash = 0xA3F55B73U; // toString
const ID_t toStringSigHash = 0x657D302EU; // ()s;
const ID_t initHash = 0x940997U; // init
const ID_t initSigHash = 0x7577EDU; // ()V;

// this is used by try-catch
tls struct _exception_handling {
	scl_any*	exception_table;	// The exception
	jmp_buf*	jump_table;			// jump buffer used by longjmp()
	scl_int		current_pointer;	// number specifying the current depth
	scl_int 	capacity;			// capacity of lists
	scl_int*	callstack_ptr;		// callstack-pointer of this catch level
} _extable = {0};

// The Stack
tls _scl_stack_t		_stack = {0};

// Scale Callstack
tls _scl_callstack_t	_callstack = {0};

#define unimplemented do { fprintf(stderr, "%s:%d: %s: Not Implemented\n", __FILE__, __LINE__, __FUNCTION__); exit(1) } while (0)

typedef struct Struct {
	StaticMembers*	statics;
	mutex_t			mutex;
} Struct;

typedef struct Struct_SclObject {
	Struct rtFields;
}* scl_SclObject;

typedef struct Struct_Exception {
	Struct rtFields;
	scl_str msg;
	struct Struct_Array* stackTrace;
	scl_str errno_str;
}* _scl_Exception;

typedef struct Struct_NullPointerException {
	struct Struct_Exception self;
} scl_NullPointerException;

typedef struct Struct_IllegalStateException {
	struct Struct_Exception self;
}* scl_IllegalStateException;

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

typedef struct Struct_Int {
	Struct rtFields;
	scl_int value;
}* scl_Int;

typedef struct Struct_Float {
	Struct rtFields;
	scl_float value;
}* scl_Float;

typedef struct Struct_AssertError {
	Struct rtFields;
	scl_str msg;
	struct Struct_Array* stackTrace;
	scl_str errno_str;
} scl_AssertError;

typedef struct Struct_UnreachableError {
	Struct rtFields;
	scl_str msg;
	struct Struct_Array* stackTrace;
	scl_str errno_str;
} scl_UnreachableError;

struct Struct_str {
	struct scale_string s;
};

extern _scl_stack_t**	stacks;
extern scl_int			stacks_count;
extern scl_int			stacks_cap;

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

volatile Struct*		runtimeModifierLock = nil;
volatile Struct*		binarySearchLock = nil;

// Inserts value into array at it's sorted position
// returns the index at which the value was inserted
// will not insert value, if it is already present in the array
scl_int insert_sorted(scl_any** array, scl_int* size, scl_any value, scl_int* cap) {
	Process$lock((volatile scl_any) runtimeModifierLock);
	if (*array == nil) {
		(*array) = system_allocate(sizeof(scl_any) * (*cap));
	}

	scl_int i = 0;
	if (*size) {
		for (i = 0; i < *size; i++) {
			if ((*array)[i] > value) {
				break;
			}
		}
	}
	if ((*array)[i] == value) return (Process$unlock((volatile scl_any) runtimeModifierLock), i);

	if ((*size) + 1 >= (*cap)) {
		(*cap) += 64;
		(*array) = system_realloc((*array), (*cap) * sizeof(scl_any*));
		if ((*array) == nil) {
			_scl_security_throw(EX_BAD_PTR, "realloc() failed");
			exit(1);
		}
	}

	scl_int j;
	for (j = *size; j > i; j--) {
		(*array)[j] = (*array)[j-1];
	}
	(*array)[i] = value;

	(*size)++;
	Process$unlock((volatile scl_any) runtimeModifierLock);
	return i;
}

void _scl_cleanup_stack_allocations() {
	for (scl_int i = 0; i < stackalloc_arrays_count; i++) {
		if (stackalloc_arrays[i] > (scl_any) &(_stack.data[_stack.ptr])) {
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
	if (stackalloc_arrays == nil) {
		stackalloc_arrays = system_allocate(sizeof(scl_any) * stackalloc_arrays_cap);
		stackalloc_array_sizes = system_allocate(sizeof(scl_int) * stackalloc_arrays_cap);
		stackalloc_arrays_count = 0;
	}
	if (stackalloc_arrays_count + 1 >= stackalloc_arrays_cap) {
		stackalloc_arrays_cap += 64;
		stackalloc_arrays = system_realloc(stackalloc_arrays, sizeof(scl_any) * stackalloc_arrays_cap);
		stackalloc_array_sizes = system_realloc(stackalloc_array_sizes, sizeof(scl_int) * stackalloc_arrays_cap);
	}
	stackalloc_arrays[stackalloc_arrays_count] = ptr;
	stackalloc_array_sizes[stackalloc_arrays_count] = size;
	stackalloc_arrays_count++;
	_stackallocations_since_cleanup++;
	if (_stackallocations_since_cleanup > 16) {
		_scl_cleanup_stack_allocations();
		_stackallocations_since_cleanup = 0;
	}
}

void _scl_stackalloc_check_bounds_or_throw(scl_any ptr, scl_int index) {
	for (scl_int i = 0; i < stackalloc_arrays_count; i++) {
		if (ptr == stackalloc_arrays[i]) {
			if (index >= 0 && index < stackalloc_array_sizes[i]) {
				return;
			}
			scl_IndexOutOfBoundsException e = ALLOC(IndexOutOfBoundsException);
			scl_int8* str = _scl_alloc(64);
			snprintf(str, 63, "Index " SCL_INT_FMT " out of bounds for array of size " SCL_INT_FMT, index, stackalloc_array_sizes[i]);
			virtual_call(e, "init(s;)V;", str_of(str));
			_scl_throw(e);
		}
	}
}

scl_int _scl_stackalloc_size(scl_any ptr) {
	for (scl_int i = 0; i < stackalloc_arrays_count; i++) {
		if (ptr == stackalloc_arrays[i]) {
			return stackalloc_array_sizes[i];
		}
	}
	return -1;
}

scl_int _scl_index_of_stackalloc(scl_any ptr) {
	for (scl_int i = 0; i < stackalloc_arrays_count; i++) {
		if (ptr == stackalloc_arrays[i]) {
			return i;
		}
	}
	return -1;
}

void _scl_remove_stack(_scl_stack_t* stack) {
	Process$lock((volatile scl_any) runtimeModifierLock);
	scl_int index;
	while ((index = _scl_stack_index(stack)) != -1) {
		_scl_remove_stack_at(index);
	}
	Process$unlock((volatile scl_any) runtimeModifierLock);
}

scl_int _scl_stack_index(_scl_stack_t* stack) {
	scl_int index = _scl_binary_search((scl_any*) stacks, stacks_count, stack);
	return index;
}

void _scl_remove_stack_at(scl_int index) {
	Process$lock((volatile scl_any) runtimeModifierLock);
	for (scl_int i = index; i < stacks_count - 1; i++) {
		stacks[i] = stacks[i + 1];
	}
	stacks_count--;
	Process$unlock((volatile scl_any) runtimeModifierLock);
}

void _scl_remove_ptr(scl_any ptr) {
	Process$lock((volatile scl_any) runtimeModifierLock);
	scl_int index;
	// Finds all indices of the pointer in the table and removes them
	while ((index = _scl_get_index_of_ptr(ptr)) != -1) {
		_scl_remove_ptr_at_index(index);
	}
	Process$unlock((volatile scl_any) runtimeModifierLock);
}

// Returns the next index of a given pointer in the allocated-pointers array
// Returns -1, if the pointer is not in the table
scl_int _scl_get_index_of_ptr(scl_any ptr) {
	return _scl_binary_search((scl_any*) allocated, allocated_count, ptr);
}

// Removes the pointer at the given index and shift everything after left
void _scl_remove_ptr_at_index(scl_int index) {
	Process$lock((volatile scl_any) runtimeModifierLock);
	for (scl_int i = index; i < allocated_count - 1; i++) {
		allocated[i] = allocated[i + 1];
		memsizes[i] = memsizes[i + 1];
	}
	allocated_count--;
	memsizes_count--;
	Process$unlock((volatile scl_any) runtimeModifierLock);
}

scl_int _scl_sizeof(scl_any ptr) {
	scl_int index = _scl_get_index_of_ptr(ptr);
	if (_scl_expect(index == -1, 0)) return -1;
	return memsizes[index];
}

scl_int insert_at(scl_any** array, scl_int* count, scl_int* cap, scl_int index, scl_any value) {
	Process$lock((volatile scl_any) runtimeModifierLock);
	if (*array == nil) {
		(*array) = system_allocate(sizeof(scl_any) * (*cap));
	}

	if ((*count) + 1 >= (*cap)) {
		(*cap) += 64;
		(*array) = system_realloc((*array), (*cap) * sizeof(scl_any*));
		if ((*array) == nil) {
			_scl_security_throw(EX_BAD_PTR, "realloc() failed");
			Process$unlock((volatile scl_any) runtimeModifierLock);
			return -1;
		}
	}

	scl_int j;
	for (j = *count; j > index; j--) {
		(*array)[j] = (*array)[j-1];
	}
	(*array)[index] = value;

	(*count)++;
	Process$unlock((volatile scl_any) runtimeModifierLock);
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
	*(_scl_callstack_push()) = "<runtime _scl_alloc(size: int): any>";

	// Make size be next biggest multiple of 8
	size = ((size + 7) >> 3) << 3;
	if (size == 0) size = 8;

	// Allocate the memory
	scl_any ptr = GC_malloc(size);
	GC_register_finalizer(ptr, (GC_finalization_proc) &_scl_pointer_destroy, nil, nil, nil);

	// Hard-throw if memory allocation failed
	if (_scl_expect(ptr == nil, 0)) {
		_scl_security_throw(EX_BAD_PTR, "allocate() failed!");
		_callstack.ptr--;
		return nil;
	}

	// Set memory to zero
	memset(ptr, 0, size);

	// Add the pointer to the table
	_scl_add_ptr(ptr, size);
	_callstack.ptr--;
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

		snprintf(cmsg, 22 + strlen(cmsg), "Assertion failed: %s\n", _scl_strdup(cmsg));
		scl_AssertError* err = ALLOC(AssertError);
		virtual_call(err, "init(s;)V;", str_of(cmsg));

		_scl_throw(err);
	}
}

void builtinUnreachable() {
	scl_UnreachableError* err = ALLOC(UnreachableError);
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

static struct Struct_Int _ints[256] = {};

scl_Int Int$valueOf(scl_int val) {
	if (val >= -128 && val <= 127) {
		return &_ints[val + 128];
	}
	scl_Int new = ALLOC(Int);
	new->value = val;
	return new;
}

scl_Float Float$valueOf(scl_float val) {
	scl_Float new = ALLOC(Float);
	new->value = val;
	return new;
}

scl_str _scl_create_string(scl_int8* data) {
	scl_str self = ALLOC(str);
	if (_scl_expect(self == nil, 0)) {
		_scl_security_throw(EX_BAD_PTR, "Failed to allocate memory for cstr '%s'\n", data);
		return nil;
	}
	self->_len = strlen(data);
	self->_hash = id_by_len(data, self->_len);
	self->_data = _scl_strndup(data, self->_len);
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

void print_stacktrace() {
	if (_callstack.ptr <= 1024) {
		printf("Stacktrace:\n");
		if (_callstack.ptr > 0) {
			for (scl_int i = 0; i < _callstack.ptr; i++) {
				printf("  %s\n", _callstack.func[i]);
			}
		}
	}
	printf("Native stacktrace:\n");
	nativeTrace();
	printf("\n");
}

// final signal handler
// if we get here, something has gone VERY wrong
void _scl_default_signal_handler(scl_int sig_num) {
	scl_int8* signalString;
	// Signals
	if (sig_num == -1) signalString = nil;
#if defined(SIGHUP)
	else if (sig_num == SIGHUP) signalString = "hangup";
#endif
#if defined(SIGINT)
	else if (sig_num == SIGINT) signalString = "interrupt";
#endif
#if defined(SIGQUIT)
	else if (sig_num == SIGQUIT) signalString = "quit";
#endif
#if defined(SIGILL)
	else if (sig_num == SIGILL) signalString = "illegal instruction";
#endif
#if defined(SIGTRAP)
	else if (sig_num == SIGTRAP) signalString = "trace trap";
#endif
#if defined(SIGABRT)
	else if (sig_num == SIGABRT) signalString = "abort()";
#endif
#if defined(SIGPOLL)
	else if (sig_num == SIGPOLL) signalString = "pollable event ([XSR] generated, not supported)";
#endif
#if defined(SIGIOT)
	else if (sig_num == SIGIOT) signalString = "compatibility";
#endif
#if defined(SIGFPE)
	else if (sig_num == SIGFPE) signalString = "floating point exception";
#endif
#if defined(SIGKILL)
	else if (sig_num == SIGKILL) signalString = "kill";
#endif
#if defined(SIGBUS)
	else if (sig_num == SIGBUS) signalString = "bus error";
#endif
#if defined(SIGSEGV)
	else if (sig_num == SIGSEGV) signalString = "segmentation violation";
#endif
#if defined(SIGSYS)
	else if (sig_num == SIGSYS) signalString = "bad argument to system call";
#endif
#if defined(SIGPIPE)
	else if (sig_num == SIGPIPE) signalString = "write on a pipe with no one to read it";
#endif
#if defined(SIGTERM)
	else if (sig_num == SIGTERM) signalString = "software termination signal from kill";
#endif
#if defined(SIGSTOP)
	else if (sig_num == SIGSTOP) signalString = "sendable stop signal not from tty";
#endif
#if defined(SIGTSTP)
	else if (sig_num == SIGTSTP) signalString = "stop signal from tty";
#endif
#if defined(SIGUSR1)
	else if (sig_num == SIGUSR1) signalString = "user defined signal 1";
#endif
#if defined(SIGUSR2)
	else if (sig_num == SIGUSR2) signalString = "user defined signal 2";
#endif
	else signalString = "Unknown signal";

	printf("\n");

	printf("Signal: %s\n", signalString);
	if (errno) {
		printf("errno: %s\n", strerror(errno));
	}
	print_stacktrace();
	printf("Stack address: %p\n", _stack.data);
	if (_stack.ptr && _stack.ptr < _stack.cap) {
		printf("Stack:\n");
		printf("SP: " SCL_INT_FMT "\n", _stack.ptr);
		for (scl_int i = _stack.ptr - 1; i >= 0; i--) {
			scl_int v = _stack.data[i].i;
			printf("   " SCL_INT_FMT ": 0x" SCL_INT_HEX_FMT ", " SCL_INT_FMT "\n", i, v, v);
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
	return _stack.ptr;
}

void _scl_sleep(scl_int millis) {
	sleep(millis);
}

const ID_t id_by_len(const char* data, size_t len) {
	ID_t h = 7;
	for (size_t i = 0; i < len; i++) {
		h = h * 31 + data[i];
	}
	return h;
}

const ID_t id(const char* data) {
	return id_by_len(data, strlen(data));
}

scl_uint _scl_rotl(const scl_uint value, scl_int shift) {
    return (value << shift) | (value >> ((sizeof(scl_uint) << 3) - shift));
}

scl_uint _scl_rotr(const scl_uint value, scl_int shift) {
    return (value >> shift) | (value << ((sizeof(scl_uint) << 3) - shift));
}

scl_int _scl_identity_hash(scl_any _obj) {
	if (!_scl_is_instance_of(_obj, SclObjectHash)) {
		return REINTERPRET_CAST(scl_int, _obj);
	}
	Struct* obj = (Struct*) _obj;
	Process$lock((volatile scl_any) obj);
	scl_int size = obj->statics->size;
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
	*(_scl_callstack_push()) = "<runtime _scl_get_method_on_type(type: int32, method: int32, signature: int32): any>";
	Struct* instance = (Struct*) type;
	scl_int index = -1;
	if (onSuper) {
		index = _scl_search_method_index((scl_any*) instance->statics->super_vtable, -1, method, signature);
	} else {
		index = _scl_search_method_index((scl_any*) instance->statics->vtable, -1, method, signature);
	}
	if (index >= 0) {
		_callstack.ptr--;
		return instance->statics->vtable[index].ptr;
	}
	_callstack.ptr--;
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

scl_any virtual_call_impl(scl_int onSuper, scl_any instance, scl_int8* methodIdentifier, va_list ap);

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

void virtual_call_impl0(scl_int onSuper, scl_any instance, scl_int8* methodIdentifier);

scl_any virtual_call_impl(scl_int onSuper, scl_any instance, scl_int8* methodIdentifier, va_list ap) {
	size_t methodLen = strlen(methodIdentifier);
	scl_int8* args = substr_of(methodIdentifier, methodLen, str_index_of(methodIdentifier, '(') + 1, str_last_index_of(methodIdentifier, ')'));
	scl_int8* returnType = substr_of(methodIdentifier, methodLen, str_last_index_of(methodIdentifier, ')') + 1, methodLen);
	scl_int argCount = str_num_of_occurences(args, ';');
	for (scl_int i = 0; i < argCount; i++) {
		_scl_push()->v = va_arg(ap, scl_any);
	}
	_scl_push()->v = instance;

	virtual_call_impl0(onSuper, instance, methodIdentifier);
	return strequals(returnType, "V;") ? nil : _scl_pop()->v;
}

void virtual_call_impl0(scl_int onSuper, scl_any instance, scl_int8* methodIdentifier) {
	size_t methodLen = strlen(methodIdentifier);
	scl_int8* methodName = substr_of(methodIdentifier, methodLen, 0, str_index_of(methodIdentifier, '('));
	scl_int8* signature = substr_of(methodIdentifier, methodLen, str_index_of(methodIdentifier, '('), methodLen);
	ID_t methodNameHash = id(methodName);
	ID_t signatureHash = id(signature);

	_scl_call_method_or_throw(instance, methodNameHash, signatureHash, onSuper, methodName, signature);
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

mutex_t _scl_mutex_new() {
	mutex_t m = GC_malloc(sizeof(pthread_mutex_t));
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(m, &attr);
	return m;
}

// creates a new instance with a size of 'size'
scl_any _scl_alloc_struct(scl_int size, const StaticMembers* statics) {
	*(_scl_callstack_push()) = "<runtime _scl_alloc_struct(size: int, typeName: [int8], super: int): any>";

	// Allocate the memory
	scl_any ptr = _scl_alloc(size);

	if (_scl_expect(ptr == nil, 0)) {
		_callstack.ptr--;
		return nil;
	}

	// Callstack
	((Struct*) ptr)->mutex = _scl_mutex_new();

	// Static members
	((Struct*) ptr)->statics = (StaticMembers*) statics;

	_callstack.ptr--;
	// Add struct to allocated table
	return _scl_add_struct(ptr);
}

// Removes an instance from the allocated table by index
void _scl_struct_map_remove(scl_int index) {
	Process$lock((volatile scl_any) runtimeModifierLock);
	for (scl_int i = index; i < instances_count - 1; i++) {
	   instances[i] = instances[i + 1];
	}
	instances_count--;
	Process$unlock((volatile scl_any) runtimeModifierLock);
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

	if (ptrStruct->statics->type == typeId) return 1; // cast to same type
	if (typeId == SclObjectHash) return 1; // cast to SclObject

	const ID_t* supers = ptrStruct->statics->supers;
	for (scl_int i = 0; supers[i]; i++) {
		if (supers[i] == typeId) return 1; // cast to super type
	}

	return 0; // invalid cast
}

scl_int _scl_binary_search(scl_any* arr, scl_int count, scl_any val) {
	Process$lock((volatile scl_any) binarySearchLock);
	scl_int left = 0;
	scl_int right = count - 1;

	while (left <= right) {
		scl_int mid = (left + right) / 2;
		if (arr[mid] == val) {
			Process$unlock((volatile scl_any) binarySearchLock);
			return mid;
		} else if (arr[mid] < val) {
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}

	Process$unlock((volatile scl_any) binarySearchLock);
	return -1;
}

scl_int _scl_search_method_index(scl_any* methods, const scl_int count, ID_t id, ID_t sig) {
	if (_scl_expect(methods == nil, 0)) return -1;
	struct _scl_methodinfo* methods_ = (struct _scl_methodinfo*) methods;

	Process$lock((volatile scl_any) binarySearchLock);
	for (scl_int i = 0; (count == -1 ? (*(scl_int*) &(methods_[i].ptr)) : i < count); i++) {
		if (methods_[i].pure_name == id && (sig == 0 || methods_[i].signature == sig)) {
			Process$unlock((volatile scl_any) binarySearchLock);
			return i;
		}
	}
	
	Process$unlock((volatile scl_any) binarySearchLock);
	return -1;
}

typedef void (*_scl_sigHandler)(scl_int);

void _scl_set_signal_handler(_scl_sigHandler handler, scl_int sig) {
	if (_scl_expect(sig < 0 || sig >= 32, 0)) {
		struct Struct_InvalidSignalException {
			struct Struct_Exception self;
			scl_int sig;
		};
		_scl_Exception e = ALLOC(InvalidSignalException);
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
		_scl_Exception e = ALLOC(InvalidSignalException);
		virtual_call(e, "init(i;)V;", sig);

		scl_int8* p = (scl_int8*) _scl_alloc(64);
		snprintf(p, 64, "Invalid signal: " SCL_INT_FMT, sig);
		e->msg = str_of(p);
		
		_scl_throw(e);
	}
	signal(sig, (void(*)(int)) _scl_default_signal_handler);
}

void _scl_set_up_signal_handler() {
#if defined(SIGINT)
	signal(SIGINT, (void (*)(int)) _scl_default_signal_handler);
#endif
#if defined(SIGILL)
	signal(SIGILL, (void (*)(int)) _scl_default_signal_handler);
#endif
#if defined(SIGTRAP)
	signal(SIGTRAP, (void (*)(int)) _scl_default_signal_handler);
#endif
#if defined(SIGABRT)
	signal(SIGABRT, (void (*)(int)) _scl_default_signal_handler);
#endif
#if defined(SIGBUS)
	signal(SIGBUS, (void (*)(int)) _scl_default_signal_handler);
#endif
#if defined(SIGSEGV)
	signal(SIGSEGV, (void (*)(int)) _scl_default_signal_handler);
#endif
}

void _scl_resize_stack() {
	_stack.cap += 64;
	_scl_frame_t* tmp = GC_realloc(_stack.data, sizeof(_scl_frame_t) * _stack.cap);
	if (_scl_expect(!tmp, 0)) {
		_scl_security_throw(EX_BAD_PTR, "realloc() failed");
	} else {
		_stack.data = tmp;
	}
}

void _scl_exception_push() {
	_extable.current_pointer++;
	if (_extable.current_pointer >= _extable.capacity) {
		_extable.capacity += 64;
		scl_any* tmp;

		tmp = system_realloc(_extable.callstack_ptr, _extable.capacity * sizeof(scl_int));
		if (_scl_expect(tmp == nil, 0)) _scl_security_throw(EX_BAD_PTR, "realloc() failed");
		_extable.callstack_ptr = (scl_int*) tmp;

		tmp = GC_realloc(_extable.exception_table, _extable.capacity * sizeof(scl_any));
		if (_scl_expect(tmp == nil, 0)) _scl_security_throw(EX_BAD_PTR, "realloc() failed");
		_extable.exception_table = tmp;

		tmp = system_realloc(_extable.jump_table, _extable.capacity * sizeof(jmp_buf));
		if (_scl_expect(tmp == nil, 0)) _scl_security_throw(EX_BAD_PTR, "realloc() failed");
		_extable.jump_table = (jmp_buf*) tmp;
	}
}

scl_int8** _scl_platform_get_env() {
	scl_int8** env;

#if defined(WIN) && (_MSC_VER >= 1900)
	env = *__p__environ();
#else
	extern char** environ;
	env = environ;
#endif

	return env;
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

#if !defined(SCL_DEFAULT_STACK_FRAME_COUNT)
#define SCL_DEFAULT_STACK_FRAME_COUNT 64
#endif

void _scl_stack_new() {
	// These use C's malloc, keep it that way
	// They should NOT be affected by any future
	// stuff we might do with _scl_alloc()
	_stack.ptr = 0;
	_stack.cap = SCL_DEFAULT_STACK_FRAME_COUNT;
	_stack.data = GC_malloc_uncollectable(sizeof(_scl_frame_t) * _stack.cap);

	insert_sorted((scl_any**) &stacks, &stacks_count, &_stack, &stacks_cap);

	_extable.callstack_ptr = 0;
	_extable.current_pointer = 0;
	_extable.capacity = SCL_DEFAULT_STACK_FRAME_COUNT;
	_extable.callstack_ptr = system_allocate(_extable.capacity * sizeof(scl_int));
	_extable.exception_table = GC_malloc_uncollectable(_extable.capacity * sizeof(scl_any));
	_extable.jump_table = system_allocate(_extable.capacity * sizeof(jmp_buf));
}

void _scl_debug_dump_stacks() {
	printf("Stacks:\n");
	for (scl_int i = 0; i < stacks_count; i++) {
		if (stacks[i]) {
			printf("  %p\n", stacks[i]);
			printf("  Elements: " SCL_INT_FMT "\n", (stacks[i])->ptr);
			printf("  Stack:\n");
			for (scl_int j = 0; j < stacks[i]->ptr; j++) {
				_scl_frame_t f = stacks[i]->data[j];
				printf("	" SCL_INT_FMT ": " SCL_PTR_HEX_FMT " " SCL_INT_FMT "\n", j, f.i, f.i);
			}
		}
	}
}

scl_int _scl_errno() {
	return errno;
}

scl_int8* _scl_errno_ptr() {
	return strerror(errno);
}

scl_str _scl_errno_str() {
	return str_of(_scl_errno_ptr());
}

void _scl_stack_free() {
	GC_free(_stack.data);
	_stack.ptr = 0;
	_stack.cap = 0;
	_stack.data = 0;

	_scl_remove_stack(&_stack);

	system_free(_extable.callstack_ptr);
	GC_free(_extable.exception_table);
	system_free(_extable.jump_table);
	_extable.callstack_ptr = 0;
	_extable.current_pointer = 0;
	_extable.capacity = 0;

	_callstack.ptr = 0;
	_callstack.cap = 0;
	system_free(_callstack.func);
	_callstack.func = 0;
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

#define TO(type, name) scl_ ## type int$to ## name (scl_int val) { return (scl_ ## type) (val & ((1ULL << (sizeof(scl_ ## type) * 8)) - 1)); }
#define VALID(type, name) scl_bool int$isValid ## name (scl_int val) { return val >= SCL_ ## type ## _MIN && val <= SCL_ ## type ## _MAX; }

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshift-count-overflow"
#endif

TO(int8, Int8)
TO(int16, Int16)
TO(int32, Int32)
TO(int64, Int64)
TO(int, Int)
TO(uint8, UInt8)
TO(uint16, UInt16)
TO(uint32, UInt32)
TO(uint64, UInt64)
TO(uint, UInt)

VALID(int8, Int8)
VALID(int16, Int16)
VALID(int32, Int32)
VALID(int64, Int64)
VALID(int, Int)
VALID(uint8, UInt8)
VALID(uint16, UInt16)
VALID(uint32, UInt32)
VALID(uint64, UInt64)
VALID(uint, UInt)

scl_str int$toString(scl_int val) {
	scl_int8* str = _scl_alloc(32);
	snprintf(str, 32, SCL_INT_FMT, val);
	scl_str s = str_of(str);
	return s;
}

scl_str int$toHexString(scl_int val) {
	scl_int8* str = _scl_alloc(19);
	snprintf(str, 19, SCL_INT_HEX_FMT, val);
	scl_str s = str_of(str);
	return s;
}

scl_str int8$toString(scl_int8 val) {
	scl_int8* str = _scl_alloc(5);
	snprintf(str, 5, "%c", val);
	scl_str s = str_of(str);
	return s;
}

void _scl_puts(scl_any val) {
	scl_str s;
	if (_scl_is_instance_of(val, SclObjectHash)) {
		s = virtual_call(val, "toString()s;");
	} else {
		s = int$toString((scl_int) val);
	}
	printf("%s\n", s->_data);
}

void _scl_puts_str(scl_str str) {
	printf("%s\n", str->_data);
}

void _scl_eputs(scl_any val) {
	scl_str s;
	if (_scl_is_instance_of(val, SclObjectHash)) {
		s = virtual_call(val, "toString()s;");
	} else {
		s = int$toString((scl_int) val);
	}
	fprintf(stderr, "%s\n", s->_data);
}

void _scl_write(scl_int fd, scl_str str) {
	write(fd, str->_data, str->_len);
}

scl_str _scl_read(scl_int fd, scl_int len) {
	scl_int8* buf = _scl_alloc(len + 1);
	read(fd, buf, len);
	scl_str str = str_of(buf);
	return str;
}

scl_int _scl_system(scl_str cmd) {
	return system(cmd->_data);
}

scl_str _scl_getenv(scl_str name) {
	scl_int8* val = getenv(name->_data);
	return val ? str_of(val) : nil;
}

scl_float _scl_time() {
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (scl_float) tv.tv_sec + (scl_float) tv.tv_usec / 1000000.0;
}

scl_str float$toString(scl_float val) {
	scl_int8* str = _scl_alloc(64);
	snprintf(str, 64, "%f", val);
	scl_str s = str_of(str);
	return s;
}

scl_str float$toPrecisionString(scl_float val) {
	scl_int8* str = _scl_alloc(256);
	snprintf(str, 256, "%.17f", val);
	scl_str s = str_of(str);
	return s;
}

scl_str float$toHexString(scl_float val) {
	return int$toHexString(REINTERPRET_CAST(scl_int, val));
}

scl_float float$fromBits(scl_any bits) {
	return REINTERPRET_CAST(scl_float, bits);
}

void dumpStack() {
	printf("Dump:\n");
	for (ssize_t i = 0; i < _stack.ptr; i++) {
		scl_int v = _stack.data[i].i;
		printf("   %zd: 0x" SCL_INT_HEX_FMT ", " SCL_INT_FMT "\n", i, v, v);
	}
	printf("\n");
}

scl_any Library$self0() {
	scl_any lib = dlopen(nil, RTLD_NOW | RTLD_GLOBAL);
	if (!lib) {
		scl_NullPointerException* e = ALLOC(NullPointerException);
		virtual_call(e, "init()V;");
		e->self.msg = str_of("Failed to load library");
		_scl_throw(e);
	}
	return lib;
}

scl_any Library$getSymbol0(scl_any lib, scl_int8* name) {
	return dlsym(lib, name);
}

scl_any Library$open0(scl_int8* name) {
	scl_any lib = dlopen(name, RTLD_NOW | RTLD_GLOBAL);
	if (!lib) {
		scl_NullPointerException* e = ALLOC(NullPointerException);
		virtual_call(e, "init()V;");
		e->self.msg = str_of("Failed to load library");
		_scl_throw(e);
	}
	return lib;
}

void Library$close0(scl_any lib) {
	dlclose(lib);
}

scl_str Library$dlerror0() {
	return str_of(dlerror());
}

char* argv0;

scl_int8* Library$progname() {
	return argv0;
}

struct Struct_Array* Process$stackTrace() {
	struct Struct_Array* arr = ALLOC(ReadOnlyArray);
	
	arr->capacity = _callstack.ptr;
	arr->initCapacity = _callstack.ptr;
	arr->count = 0;
	arr->values = _scl_new_array((_callstack.ptr) * sizeof(scl_str));
	
	for (scl_int i = 0; i < _callstack.ptr; i++) {
		((scl_str*) arr->values)[(scl_int) arr->count++] = _scl_create_string(_callstack.func[i]);
	}
	return arr;
}

scl_bool Process$gcEnabled() {
	return !GC_is_disabled();
}
void Process$lock0(mutex_t mutex) {
	pthread_mutex_lock(mutex);
}
void Process$lock(volatile scl_any obj) {
	if (_scl_expect(!obj, 0)) return;
	Process$lock0(((volatile Struct*) obj)->mutex);
}
void Process$unlock0(mutex_t mutex) {
	pthread_mutex_unlock(mutex);
}
void Process$unlock(volatile scl_any obj) {
	if (_scl_expect(!obj, 0)) return;
	Process$unlock0(((volatile Struct*) obj)->mutex);
}

scl_str GarbageCollector$getImplementation0() {
	return str_of("Boehm GC");
}

scl_bool GarbageCollector$isPaused0() {
	return GC_is_disabled();
}

void GarbageCollector$setPaused0(scl_bool paused) {
	if (paused) {
		GC_disable();
	} else {
		GC_enable();
	}
}

void GarbageCollector$run0() {
	GC_gcollect();
}

scl_int GarbageCollector$heapSize() {
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

scl_int GarbageCollector$freeBytesEstimate() {
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

scl_int GarbageCollector$bytesSinceLastCollect() {
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

scl_int GarbageCollector$totalMemory() {
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

typedef struct Struct_Thread {
	Struct rtFields;
	_scl_lambda function;
	pthread_t nativeThread;
	scl_str name;
}* scl_Thread;

typedef struct Struct_InvalidArgumentException {
	struct Struct_Exception self;
}* scl_InvalidArgumentException;

extern scl_Array Var_Thread$threads;
extern scl_Thread Var_Thread$mainThread;
static tls scl_Thread _currentThread = nil;

extern scl_any* arrays;
extern scl_int arrays_count;
extern scl_int arrays_capacity;

scl_any* _scl_new_array(scl_int num_elems) {
	if (_scl_expect(num_elems < 1, 0)) {
		scl_InvalidArgumentException e = ALLOC(InvalidArgumentException);
		virtual_call(e, "init(s;)V;", str_of("Array size must not be less than 1"));
		_scl_throw(e);
	}
	scl_any* arr = _scl_alloc(num_elems * sizeof(scl_any) + sizeof(scl_int));
	((scl_int*) arr)[0] = num_elems;
	if (arrays == nil) {
		arrays = system_allocate(sizeof(scl_any) * arrays_capacity);
	}
	arrays_count++;
	if (arrays_count >= arrays_capacity) {
		arrays_capacity += 64;
		arrays = system_realloc(arrays, sizeof(scl_any) * arrays_capacity);
	}
	arrays[arrays_count - 1] = arr;
	return arr + 1;
}

scl_int _scl_is_array(scl_any* arr) {
	if (_scl_expect(!arr, 0)) return 0;
	for (scl_int i = 0; i < arrays_count; i++) {
		if (arrays[i] == (arr - 1)) {
			return 1;
		}
	}
	return 0;
}

scl_any* _scl_multi_new_array(scl_int dimensions, scl_int sizes[dimensions]) {
	if (_scl_expect(dimensions < 1, 0)) {
		scl_InvalidArgumentException e = ALLOC(InvalidArgumentException);
		virtual_call(e, "init(s;)V;", str_of("Array dimensions must not be less than 1"));
		_scl_throw(e);
	}
	if (dimensions == 1) {
		return _scl_new_array(sizes[0]);
	}
	scl_any* arr = _scl_alloc(sizes[0] * sizeof(scl_any) + sizeof(scl_int));
	((scl_int*) arr)[0] = sizes[0];
	for (scl_int i = 1; i <= sizes[0]; i++) {
		arr[i] = _scl_multi_new_array(dimensions - 1, &(sizes[1]));
	}
	return arr + 1;
}

scl_int _scl_array_size_unchecked(scl_any* arr) {
	if (_scl_index_of_stackalloc(arr) >= 0) {
		return _scl_stackalloc_size(arr);
	}
	return *((scl_int*) arr - 1);
}

scl_int _scl_array_size(scl_any* arr) {
	if (_scl_index_of_stackalloc(arr) >= 0) {
		return _scl_stackalloc_size(arr);
	}
	if (_scl_expect(_scl_get_index_of_ptr(arr - 1) < 0, 0)) {
		scl_InvalidArgumentException e = ALLOC(InvalidArgumentException);
		virtual_call(e, "init(s;)V;", str_of("Array must be initialized with 'new[]'"));
		_scl_throw(e);
	}
	return *((scl_int*) arr - 1);
}

void _scl_array_check_bounds_or_throw(scl_any* arr, scl_int index) {
	if (_scl_index_of_stackalloc(arr) >= 0) {
		return _scl_stackalloc_check_bounds_or_throw(arr, index);
	}
	if (_scl_expect(_scl_get_index_of_ptr(arr - 1) < 0, 0)) {
		scl_InvalidArgumentException e = ALLOC(InvalidArgumentException);
		virtual_call(e, "init(s;)V;", str_of("Array must be initialized with 'new[]'"));
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
	if (_scl_expect(_scl_index_of_stackalloc(arr) >= 0, 0)) {
		scl_InvalidArgumentException e = ALLOC(InvalidArgumentException);
		virtual_call(e, "init(s;)V;", str_of("Cannot resize stack-allocated array"));
		_scl_throw(e);
	}
	if (_scl_expect(_scl_get_index_of_ptr(arr - 1) < 0, 0)) {
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
	((scl_int*) new_arr)[0] = new_size;
	return new_arr + 1;
}

// these args cannot be scl_any because warning would appear
static int compare(const void* a, const void* b) {
	return ((a < b) ? -1 : ((a > b) ? 1 : 0));
}

scl_any* _scl_array_sort(scl_any* arr) {
	if (_scl_expect(_scl_get_index_of_ptr(arr - 1) < 0 && _scl_index_of_stackalloc(arr) < 0, 0)) {
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
	if (_scl_expect(_scl_get_index_of_ptr(arr - 1) < 0 && _scl_index_of_stackalloc(arr) < 0, 0)) {
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

scl_str _scl_array_to_string(scl_any* arr) {
	if (_scl_expect(_scl_get_index_of_ptr(arr - 1) < 0 && _scl_index_of_stackalloc(arr) < 0, 0)) {
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
			tmp = int$toString((scl_int) arr[i]);
		}
		s = virtual_call(s, "append(s;)s;", tmp);
	}
	return virtual_call(s, "append(s;)s;", str_of("]"));
}

static scl_int8* substr_of(scl_int8* str, size_t len, size_t beg, size_t end) {
	scl_int8* sub = _scl_alloc(len);
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
		snprintf(tmp, len + 2, "[%s", _scl_type_to_rt_sig(substr_of(type, len, 1, len - 1)));
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
	*(_scl_callstack_push()) = "<runtime _scl_type_array_to_rt_sig(arr: [str]): str>";
	scl_str s = str_of("");
	scl_int strHash = id("str");
	scl_int getHash = id("get");
	scl_int getSigHash = id("(i;)a;");
	scl_int appendHash = id("append");
	scl_int appendSigHash = id("(cs;)s;");
	for (scl_int i = 0; i < arr->count; i++) {
		scl_str type = virtual_call(arr, "get(i;)a;", i);
		_scl_assert(_scl_is_instance_of(type, strHash), "_scl_type_array_to_rt_sig: type is not a string");
		s = virtual_call(s, "append(cs;)s;", _scl_type_to_rt_sig(type->_data));
	}
	_callstack.ptr--;
	return s;
}

scl_str _scl_types_to_rt_signature(scl_str returnType, scl_Array args) {
	scl_int8* retType = _scl_type_to_rt_sig(returnType->_data);
	scl_str argTypes = _scl_type_array_to_rt_sig(args);
	scl_str sig = str_of("(");
	sig = virtual_call(sig, "append(s;)s;", argTypes);
	sig = virtual_call(sig, "append(cs;)s;", ")");
	return virtual_call(sig, "append(cs;)s;", retType);
}

void Thread$run(scl_Thread self) {
	_scl_stack_new();
	*(_scl_callstack_push()) = "<extern Thread:run(): none>";
	_currentThread = self;

	virtual_call(Var_Thread$threads, "push(a;)V;", self);
	
	self->function();

	virtual_call(Var_Thread$threads, "remove(a;)V;", self);
	
	_currentThread = nil;
	_callstack.ptr--;
	_scl_stack_free();
}

scl_int Thread$start0(scl_Thread self) {
	int ret = GC_pthread_create(&self->nativeThread, 0, (scl_any) Thread$run, self);
	return ret;
}

scl_int Thread$stop0(scl_Thread self) {
	int ret = GC_pthread_join(self->nativeThread, 0);
	return ret;
}

scl_Thread Thread$currentThread() {
	if (!_currentThread) {
		_currentThread = ALLOC(Thread);
		_currentThread->name = str_of("Main Thread");
		_currentThread->nativeThread = pthread_self();
		_currentThread->function = nil;
	}
	return _currentThread;
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

scl_int8** _scl_callstack_push() {
	if (_callstack.func == nil) {
		_callstack.ptr = 0;
		_callstack.cap = 64;
		// can't use system_allocate here because it uses _scl_callstack_push
		// would be a recursive call
		_callstack.func = malloc(sizeof(scl_int8*) * _callstack.cap);
	}
	if ((scl_uint) _callstack.ptr > _callstack.cap) {
		_scl_security_throw(EX_STACK_OVERFLOW, "Callstack smashed! Pointer is: 0x" SCL_INT_HEX_FMT " Capacity is: 0x" SCL_INT_HEX_FMT "\n", _callstack.ptr, _callstack.cap);
	}
	_callstack.ptr++;
	if (_callstack.ptr >= _callstack.cap) {
		_callstack.cap += 64;
		// can't use system_realloc here because it uses _scl_callstack_push
		// would be a recursive call
		_callstack.func = realloc(_callstack.func, _callstack.cap * sizeof(scl_int8*));
		if (_callstack.func == nil) {
			_scl_security_throw(EX_BAD_PTR, "Callstack smashed! Callstack pointer is: %p Reallocated to size 0x" SCL_INT_HEX_FMT "\n", _callstack.func, _callstack.cap);
		}
	}
	return &_callstack.func[_callstack.ptr - 1];
}

void _scl_stack_resize_fit(scl_int sz) {
	_stack.ptr += sz;

	while (_scl_expect(_stack.ptr >= _stack.cap, 0)) {
		_scl_resize_stack();
	}
}

void _scl_create_stack() {
	// These use C's malloc, keep it that way
	// They should NOT be affected by any future
	// stuff we might do with _scl_alloc()
	stacks = system_allocate(stacks_cap * sizeof(_scl_stack_t*));

	_scl_stack_new();

	allocated = system_allocate(allocated_cap * sizeof(scl_any));
	memsizes = system_allocate(memsizes_cap * sizeof(scl_int));
	instances = system_allocate(instances_cap * sizeof(Struct*));
}

void _scl_throw(scl_any ex) {
	if (_scl_is_instance_of(ex, id("Error"))) {
		_extable.current_pointer = 0;
	} else {
		_extable.current_pointer--;
		if (_extable.current_pointer < 0) {
			_extable.current_pointer = 0;
		}
	}
	_extable.exception_table[_extable.current_pointer] = ex;
	_callstack.ptr = _extable.callstack_ptr[_extable.current_pointer];
	longjmp(_extable.jump_table[_extable.current_pointer], 666);
}

// Returns a function pointer with the following signature:
// function main(args: Array, env: Array): int
typedef void(*mainFunc)(struct Struct_Array*, struct Struct_Array*);

_scl_no_return
void* _scl_oom(scl_uint size) {
	fprintf(stderr, "Out of memory! Tried to allocate " SCL_UINT_FMT " bytes.\n", size);
	nativeTrace();
	exit(-1);
}

void nativeTrace() {
	void* callstack[1024];
	int i, frames = backtrace(callstack, 1024);
	char** strs = backtrace_symbols(callstack, frames);
	fprintf(stderr, "Native stack trace:\n");
	for (i = 0; i < frames; ++i) {
		fprintf(stderr, "%s\n", strs[i]);
	}
	fprintf(stderr, "\n");
	free(strs);
}

void printStackTraceOf(_scl_Exception e) {
	if (!e->stackTrace || e->stackTrace->count == 0) {
		nativeTrace();
		return;
	}
	for (scl_int i = e->stackTrace->count - 3; i >= 0; i--) {
		fprintf(stderr, "  %s\n", ((scl_str) ((scl_any*) e->stackTrace->values)[i])->_data);
	}
	fprintf(stderr, "\n");
}

static int setupCalled = 0;

_scl_constructor
void _scl_setup() {
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

	runtimeModifierLock = ALLOC(SclObject);
	binarySearchLock = ALLOC(SclObject);
	_scl_create_stack();

	extern const StaticMembers _scl_statics_Int;

	ID_t intHash = id("Int");
	for (scl_int i = -128; i < 127; i++) {
		_ints[i + 128] = (struct Struct_Int) {
			.rtFields = {
				.statics = (StaticMembers*) &_scl_statics_Int,
				.mutex = _scl_mutex_new(),
			},
			.value = i
		};
	}
	_scl_exception_push();

	Var_Thread$mainThread = _currentThread = Thread$currentThread();

	setupCalled = 1;
}

int _scl_run(int argc, char** argv, scl_any entry) {

	// Convert argv and envp from native arrays to Scale arrays
	struct Struct_Array* args = (struct Struct_Array*) _scl_c_arr_to_scl_array((scl_any*) argv);
	struct Struct_Array* env = (struct Struct_Array*) _scl_c_arr_to_scl_array((scl_any*) _scl_platform_get_env());

	argv0 = argv[0];

	_extable.current_pointer = 1;
	_callstack.ptr = 0;
	TRY {
		if (entry) {
			((mainFunc) entry)(args, env);
		}
	} else {
		if (_extable.exception_table[_extable.current_pointer] == nil) {
			_scl_security_throw(EX_BAD_PTR, "nil exception pointer");
		}
		scl_str msg = ((_scl_Exception) _extable.exception_table[_extable.current_pointer])->msg;

		fprintf(stderr, "Uncaught %s: %s\n", ((_scl_Exception) _extable.exception_table[_extable.current_pointer])->rtFields.statics->type_name, msg->_data);
		printStackTraceOf(_extable.exception_table[_extable.current_pointer]);
		if (errno) {
			fprintf(stderr, "errno: %s\n", strerror(errno));
		}
		exit(EX_THROWN);
	}
	return 0;
}
