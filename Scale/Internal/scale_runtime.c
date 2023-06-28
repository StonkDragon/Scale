#include "scale_runtime.h"

const hash SclObjectHash = 0xC9CCFE34U; // SclObject
const hash toStringHash = 0xA3F55B73U; // toString
const hash toStringSigHash = 0x657D302EU; // ()s;
const hash initHash = 0x940997U; // init
const hash initSigHash = 0x7577EDU; // ()V;

// The Stack
tls _scl_stack_t		_stack;

// Scale Callstack
tls _scl_callstack_t	_callstack;

// this is used by try-catch
tls struct _exception_handling {
	scl_any*	exceptions;		// The exception
	jmp_buf*	jmp_buf;		// jump buffer used by longjmp()
	scl_int		jmp_buf_ptr;	// number specifying the current depth
	scl_int 	capacity;		// capacity of lists
	scl_int*	cs_pointer;		// callstack-pointer of this catch level
} _extable = {0};

#define unimplemented do { fprintf(stderr, "%s:%d: %s: Not Implemented\n", __FILE__, __LINE__, __FUNCTION__); exit(1) } while (0)

// generic struct
typedef struct Struct {
	scl_int		type;
	scl_int8*	type_name;
	scl_int		super;
	scl_int		size;
	mutex_t		mutex;
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
	scl_any values;
	scl_any count;
	scl_any capacity;
	scl_int initCapacity;
	scl_int pos;
}* scl_Array;

typedef struct Struct_ReadOnlyArray {
	struct Struct_Array self;
}* scl_ReadOnlyArray;

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

volatile Struct* runtimeModifierLock = nil;
volatile Struct* binarySearchLock = nil;

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
	return _scl_binary_search(allocated, allocated_count, ptr);
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
	_callstack.func[_callstack.ptr++] = "<runtime _scl_alloc(size: int): any>";

	// Make size be next biggest multiple of 8
	size = ((size + 7) >> 3) << 3;
	if (size == 0) size = 8;

	// Allocate the memory
	scl_any ptr = GC_malloc(size);
	GC_register_finalizer(ptr, (GC_finalization_proc) &_scl_pointer_destroy, nil, nil, nil);

	// Hard-throw if memory allocation failed
	if (_scl_expect(ptr == nil, 0)) {
		_scl_security_throw(EX_BAD_PTR, "allocate() failed!");
		_callstack.func[--_callstack.ptr] = NULL;
		return nil;
	}

	// Set memory to zero
	memset(ptr, 0, size);

	// Add the pointer to the table
	_scl_add_ptr(ptr, size);
	_callstack.func[--_callstack.ptr] = NULL;
	return ptr;
}

// Reallocates a pointer to a new size
// the memory will always have a size of a multiple of sizeof(scl_int)
scl_any _scl_realloc(scl_any ptr, scl_int size) {
	if (size == 0) {
		scl_NullPointerException* ex = NEW(NullPointerException, Exception);
		_scl_push()->v = ex;
		_scl_call_method_or_throw(ex, initHash, initSigHash, 0, "init", "()V;");
		ex->self.msg = str_of("realloc() called with size 0");
		_scl_throw(ex);
	}

	// Make size be next biggest multiple of sizeof(scl_int)
	size = ((size + 7) >> 3) << 3;
	if (size == 0) size = 8;

	int wasStruct = _scl_find_index_of_struct(ptr) != -1;

	// If this pointer points to an instance of a struct, quietly deallocate it
	// without calling SclObject:finalize() on it
	_scl_free_struct_no_finalize(ptr);
	
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
		typedef struct Struct_AssertError {
			Struct _structData;
			scl_str msg;
			struct Struct_Array* stackTrace;
			scl_str errno_str;
		} _scl_AssertError;

		scl_int8* cmsg = (scl_int8*) _scl_alloc(strlen(msg) * 8);
		va_list list;
		va_start(list, msg);
		vsprintf(cmsg, msg, list);
		va_end(list);

		sprintf(cmsg, "Assertion failed: %s\n", _scl_strdup(cmsg));
		_scl_AssertError* err = NEW(AssertError, Error);
		_scl_push()->v = str_of(cmsg);
		_scl_push()->v = err;
		_scl_call_method_or_throw(err, initHash, hash1("(s;)V;"), 0, "init", "(s;)V;");

		_scl_throw(err);
	}
}

void builtinUnreachable() {
	typedef struct Struct_UnreachableError {
		Struct _structData;
		scl_str msg;
		struct Struct_Array* stackTrace;
		scl_str errno_str;
	} _scl_UnreachableError;

	_scl_UnreachableError* err = NEW(UnreachableError, Error);
	_scl_push()->v = str_of("Unreachable!");
	_scl_push()->v = err;
	_scl_call_method_or_throw(err, initHash, hash1("(s;)V;"), 0, "init", "(s;)V;");

	_scl_throw(err);
}

// Hard-throw an exception
_scl_no_return void _scl_security_throw(int code, scl_int8* msg, ...) {
	printf("\n");

	va_list args;
	va_start(args, msg);
	
	char* str = system_allocate(strlen(msg) * 8);
	vsprintf(str, msg, args);
	printf("Exception: %s\n", str);

	va_end(args);

	if (errno) {
		printf("errno: %s\n", strerror(errno));
	}
	if (code != EX_STACK_OVERFLOW)
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

extern const scl_int _scl_internal_string_literals_count;
extern struct scaleString _scl_internal_string_literals[];

scl_int _scl_find_string_in_literals(scl_int8* str, scl_int len) {
	scl_int index = -1;

	for (scl_int i = 0; i < _scl_internal_string_literals_count; i++) {
		if (_scl_internal_string_literals[i]._len != len) continue;
		if (strncmp(_scl_internal_string_literals[i]._data, str, len) == 0) {
			index = i;
			break;
		}
	}
	return index;
}

scl_str _scl_create_string(scl_int8* data) {
	struct Struct_str {
		struct scaleString s;
	};

	scl_int len = strlen(data);
	scl_int index = _scl_find_string_in_literals(data, len);

	if (index != -1) {
		scl_str s = (scl_str) &(_scl_internal_string_literals[index]);
		if (!s->$__mutex__) {
			s->$__mutex__ = _scl_mutex_new();
			s->_hash = hash1len(data, len);
		}
		_scl_add_struct(s);
		return s;
	}

	scl_str self = NEW0(str);
	if (_scl_expect(self == nil, 0)) {
		_scl_security_throw(EX_BAD_PTR, "Failed to allocate memory for cstr '%s'\n", data);
		return nil;
	}
	self->_len = len;
	self->_hash = hash1len(data, len);
	self->_data = _scl_strndup(data, self->_len);
	return self;
}

extern int printingStacktrace;

void print_stacktrace() {
	if (_callstack.ptr > 1024) return;
	printingStacktrace = 1;

	printf("Stacktrace:\n");
	printf("Elements: " SCL_INT_FMT "\n", _callstack.ptr);
	for (scl_int i = 0; i < _callstack.ptr; i++) {
		printf("  %s\n", _callstack.func[i]);
	}

	printingStacktrace = 0;
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
	struct Struct_Array* array = NEW(ReadOnlyArray, Array);
	scl_int cap = 0;
	while (arr[cap] != nil) {
		cap++;
	}

	array->capacity = (scl_any) cap;
	array->initCapacity = cap;
	array->count = 0;
	array->values = _scl_alloc(cap * sizeof(scl_str));
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

const hash hash1len(const char* data, size_t len) {
	hash h = 7;
	for (size_t i = 0; i < len; i++) {
		h = h * 31 + data[i];
	}
	return h;
}

const hash hash1(const char* data) {
	return hash1len(data, strlen(data));
}

scl_uint _scl_rotl(const scl_uint value, scl_int shift) {
    return (value << shift) | (value >> ((sizeof(scl_uint) << 3) - shift));
}

scl_uint _scl_rotr(const scl_uint value, scl_int shift) {
    return (value >> shift) | (value << ((sizeof(scl_uint) << 3) - shift));
}

scl_int _scl_identity_hash(scl_any _obj) {
	if (!_scl_is_instance_of(_obj, SclObjectHash)) {
		return *(scl_int*) &_obj;
	}
	Struct* obj = (Struct*) _obj;
	Process$lock((volatile scl_any) obj);
	scl_int size = obj->size;
	scl_int hash = (*(scl_int*) &obj) % 17;
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

struct _scl_methodinfo {
  scl_any  	ptr;
  scl_int  	pure_name;
  scl_any  	actual_handle;
  scl_int8*	actual_name;
  scl_int	signature;
  scl_int8*	signature_str;
};

struct _scl_membertype {
  scl_int  	pure_name;
  scl_int8*	actual_name;
  scl_int8*	type_name;
  scl_int  	offset;
  scl_int	access_flags;
};

struct _scl_typeinfo {
  scl_int					type;
  scl_int					super;
  scl_int					alloc_size;
  scl_int8*					name;
  scl_int					memberscount;
  struct _scl_membertype*	members;
  scl_int					methodscount;
  struct _scl_methodinfo*	methods;
};

// Structs
extern struct _scl_typeinfo		_scl_types[];
extern scl_int 					_scl_types_count;

scl_int _scl_binary_search_typeinfo_index(struct _scl_typeinfo* types, scl_int count, hash type) {
	Process$lock((volatile scl_any) runtimeModifierLock);
	scl_int left = 0;
	scl_int right = count - 1;

	while (left <= right) {
		scl_int mid = (left + right) / 2;
		if (types[mid].type == type) {
			Process$unlock((volatile scl_any) runtimeModifierLock);
			return mid;
		} else if (types[mid].type < type) {
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}

	Process$unlock((volatile scl_any) runtimeModifierLock);
	return -1;
}

typedef struct Struct_Struct {
	Struct structData;
	scl_str typeName;
	scl_int typeId;
	struct Struct_Struct* super;
	scl_int binarySize;
	scl_int members_count;
	struct _scl_membertype* members;
	scl_bool isStatic;
} _scl_Struct;

typedef struct Struct_Variable {
	Struct structData;
} _scl_Variable;

extern _scl_Struct** structs;
extern scl_int structs_count;
extern scl_int structs_cap;

scl_int _scl_binary_search_struct_struct_index(_scl_Struct** structs, scl_int count, hash type) {
	Process$lock((volatile scl_any) runtimeModifierLock);
	scl_int left = 0;
	scl_int right = count - 1;

	while (left <= right) {
		scl_int mid = (left + right) / 2;
		if (structs[mid]->typeId == type) {
			Process$unlock((volatile scl_any) runtimeModifierLock);
			return mid;
		} else if (structs[mid]->typeId < type) {
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}

	Process$unlock((volatile scl_any) runtimeModifierLock);
	return -1;
}

scl_any _scl_get_struct_by_id(scl_int id) {
	scl_int index = _scl_binary_search_struct_struct_index(structs, structs_count, id);
	if (index >= 0) return structs[index];
	if (_scl_expect(id == 0, 0)) return nil;
	index = _scl_binary_search_typeinfo_index(_scl_types, _scl_types_count, id);
	if (_scl_expect(index < 0, 0)) {
		return nil;
	}
	struct _scl_typeinfo t = _scl_types[index];
	_scl_Struct* s = NEW0(Struct);
	s->binarySize = t.alloc_size;
	s->typeName = str_of(t.name);
	s->typeId = id;
	s->super = _scl_get_struct_by_id(t.super);
	s->isStatic = s->binarySize == 0;
	s->members = t.members;
	s->members_count = t.memberscount;
	insert_sorted((scl_any**) &structs, &structs_count, s, &structs_cap);
	return s;
}

// Marks unreachable execution paths
_scl_no_return void _scl_unreachable(char* msg) {
	// Uses compiler specific extensions if possible.
	// Even if no extension is used, undefined behavior is still raised by
	// an empty function body and the noreturn attribute.
#if defined(__GNUC__)
	__builtin_unreachable();
#elif defined(_MSC_VER) // MSVC
	__assume(false);
#endif
}

_scl_no_return void _scl_callstack_overflow(scl_int8* func) {
	_scl_security_throw(EX_STACK_OVERFLOW, "Call stack overflow in function '%s'", func);
}

// Returns a pointer to the typeinfo of the given struct
// nil if the struct does not exist
scl_any _scl_typeinfo_of(hash type) {
	scl_int index = _scl_binary_search_typeinfo_index(_scl_types, _scl_types_count, type);
	if (index >= 0) {
		return (scl_any) _scl_types + (index * sizeof(struct _scl_typeinfo));
	}
	return nil;
}

struct _scl_typeinfo* _scl_find_typeinfo_of(hash type) {
	return (struct _scl_typeinfo*) _scl_typeinfo_of(type);
}

// returns the method handle of a method on a struct, or a parent struct
scl_any _scl_get_method_on_type(hash type, hash method, hash signature) {
	_callstack.func[_callstack.ptr++] = "<runtime _scl_get_method_on_type(type: int32, method: int32, signature: int32): any>";
	struct _scl_typeinfo* p = _scl_find_typeinfo_of(type);
	while (p) {
		scl_int index = _scl_binary_search_method_index((scl_any*) p->methods, p->methodscount, method, signature);
		if (index >= 0) {
			_callstack.func[--_callstack.ptr] = nil;
			return p->methods[index].ptr;
		}
		p = _scl_find_typeinfo_of(p->super);
	}
	_callstack.func[--_callstack.ptr] = nil;
	return nil;
}

void _scl_call_method_or_throw(scl_any instance, hash method, hash signature, int on_super, scl_int8* method_name, scl_int8* signature_str) {
	if (!_scl_is_instance_of(instance, SclObjectHash)) {
		_scl_security_throw(EX_BAD_PTR, "Method call on non-object");
	}
	Struct* type = instance;
	scl_int typeID = type->type;
	scl_int8* typeName = type->type_name;
	if (on_super) {
		typeID = type->super;
		typeName = _scl_find_typeinfo_of(typeID)->name;
	}
	scl_any handle = _scl_get_method_on_type(typeID, method, signature);
	if (handle == nil) {
		_scl_security_throw(EX_BAD_PTR, "Method '%s%s' not found on type '%s'", method_name, signature_str, typeName);
	}
	((void (*)(void)) handle)();
}

scl_any _scl_get_method_handle(hash type, hash method, hash signature) {
	struct _scl_typeinfo* p = _scl_find_typeinfo_of(type);
	while (p) {
		scl_int index = _scl_binary_search_method_index((scl_any*) p->methods, p->methodscount, method, signature);
		if (index >= 0) {
			return p->methods[index].actual_handle;
		}
		p = _scl_find_typeinfo_of(p->super);
	}
	return nil;
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
scl_any _scl_alloc_struct(scl_int size, scl_int8* type_name, hash super) {
	_callstack.func[_callstack.ptr++] = "<runtime _scl_alloc_struct(size: int, typeName: [int8], super: int): any>";

	// Allocate the memory
	scl_any ptr = _scl_alloc(size);

	if (_scl_expect(ptr == nil, 0)) {
		_callstack.func[--_callstack.ptr] = NULL;
		return nil;
	}

	// Type name hash
	((Struct*) ptr)->type = hash1(type_name);

	// Type name
	((Struct*) ptr)->type_name = type_name;

	// Parent struct name hash
	((Struct*) ptr)->super = super;

	// Size (Currently only used by SclObject:clone())
	((Struct*) ptr)->size = size;

	// Callstack
	((Struct*) ptr)->mutex = _scl_mutex_new();

	_callstack.func[--_callstack.ptr] = NULL;
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
void _scl_free_struct_no_finalize(scl_any ptr) {
	scl_int i = _scl_find_index_of_struct(ptr);
	while (i != -1) {
		_scl_struct_map_remove(i);
		i = _scl_find_index_of_struct(ptr);
	}
}

// Removes an instance from the allocated table and call the finalzer
void _scl_free_struct(scl_any ptr) {
	if (_scl_expect(_scl_find_index_of_struct(ptr) == -1, 0)) {
		return;
	}

	scl_int i = _scl_find_index_of_struct(ptr);
	while (i != -1) {
		_scl_struct_map_remove(i);
		i = _scl_find_index_of_struct(ptr);
	}
}

// Returns true if the given type is a child of the other type
scl_int _scl_type_extends_type(struct _scl_typeinfo* type, struct _scl_typeinfo* extends) {
	if (_scl_expect(!type || !extends, 0)) return 0;
	do {
		if (type->type == extends->type) {
			return 1;
		}
		type = _scl_find_typeinfo_of(type->super);
	} while (type);
	return 0;
}

// Returns true, if the instance is of a given struct type
scl_int _scl_is_instance_of(scl_any ptr, hash typeId) {
	if (_scl_expect(!ptr, 0)) return 0;
	int isStruct = _scl_binary_search((scl_any*) instances, instances_count, ptr) != -1;
	if (_scl_expect(!isStruct, 0)) return 0;

	Struct* ptrStruct = (Struct*) ptr;

	if (ptrStruct->type == typeId) return 1;

	struct _scl_typeinfo* ptrType = _scl_typeinfo_of(ptrStruct->type);
	struct _scl_typeinfo* typeIdType = _scl_typeinfo_of(typeId);

	return _scl_type_extends_type(ptrType, typeIdType);
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

scl_int _scl_binary_search_method_index(scl_any* methods, scl_int count, hash id, hash sig) {
	struct _scl_methodinfo* methods_ = (struct _scl_methodinfo*) methods;

	Process$lock((volatile scl_any) binarySearchLock);
	for (scl_int i = 0; i < count; i++) {
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
		_scl_Exception e = NEW(InvalidSignalException, Exception);
		_scl_push()->i = sig;
		_scl_push()->v = e;
		_scl_call_method_or_throw(e, initHash, initSigHash, 0, "init", "()V;");

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
		_scl_Exception e = NEW(InvalidSignalException, Exception);
		_scl_push()->i = sig;
		_scl_push()->v = e;
		_scl_call_method_or_throw(e, initHash, initSigHash, 0, "init", "()V;");

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

_scl_frame_t* _scl_push() {
	_stack.ptr++;

	if (_stack.ptr >= _stack.cap) {
		_stack.cap += 64;
		_scl_frame_t* tmp = GC_realloc(_stack.data, sizeof(_scl_frame_t) * _stack.cap);
		if (_scl_expect(!tmp, 0)) {
			_scl_security_throw(EX_BAD_PTR, "realloc() failed");
		} else {
			_stack.data = tmp;
		}
	}

	return &(_stack.data[_stack.ptr - 1]);
}

_scl_frame_t* _scl_pop() {
#if !defined(SCL_FEATURE_UNSAFE_STACK_ACCESSES)
	if (_scl_expect(_stack.ptr <= 0, 0)) {
		_scl_security_throw(EX_STACK_UNDERFLOW, "pop() failed: Not enough data on the stack! Stack pointer was " SCL_INT_FMT, _stack.ptr);
	}
#endif

	return &(_stack.data[--_stack.ptr]);
}

_scl_frame_t* _scl_offset(scl_int offset) {
	return &(_stack.data[offset]);
}

_scl_frame_t* _scl_positive_offset(scl_int offset) {
	return &(_stack.data[_stack.ptr + offset]);
}

_scl_frame_t* _scl_top() {
#if !defined(SCL_FEATURE_UNSAFE_STACK_ACCESSES)
	if (_scl_expect(_stack.ptr <= 0, 0)) {
		_scl_security_throw(EX_STACK_UNDERFLOW, "top() failed: Not enough data on the stack! Stack pointer was " SCL_INT_FMT, _stack.ptr);
	}
#endif

	return &_stack.data[_stack.ptr - 1];
}

void _scl_popn(scl_int n) {
	_stack.ptr -= n;

#if !defined(SCL_FEATURE_UNSAFE_STACK_ACCESSES)
	if (_scl_expect(_stack.ptr < 0, 0)) {
		_scl_security_throw(EX_STACK_UNDERFLOW, "popn() failed: Not enough data on the stack! Stack pointer was " SCL_INT_FMT, _stack.ptr);
	}
#endif
}

void _scl_exception_push() {
	_extable.jmp_buf_ptr++;
	if (_extable.jmp_buf_ptr >= _extable.capacity) {
		_extable.capacity += 64;
		scl_any* tmp;

		tmp = system_realloc(_extable.cs_pointer, _extable.capacity * sizeof(scl_int));
		if (_scl_expect(tmp == nil, 0)) _scl_security_throw(EX_BAD_PTR, "realloc() failed");
		_extable.cs_pointer = (scl_int*) tmp;

		tmp = GC_realloc(_extable.exceptions, _extable.capacity * sizeof(scl_any));
		if (_scl_expect(tmp == nil, 0)) _scl_security_throw(EX_BAD_PTR, "realloc() failed");
		_extable.exceptions = tmp;

		tmp = system_realloc(_extable.jmp_buf, _extable.capacity * sizeof(jmp_buf));
		if (_scl_expect(tmp == nil, 0)) _scl_security_throw(EX_BAD_PTR, "realloc() failed");
		_extable.jmp_buf = (jmp_buf*) tmp;
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

void _scl_checked_cast(scl_any instance, hash target_type, scl_int8* target_type_name) {
	if (!_scl_is_instance_of(instance, target_type)) {
		typedef struct Struct_CastError {
			Struct _structData;
			scl_str msg;
			struct Struct_Array* stackTrace;
			scl_str errno_str;
		} _scl_CastError;

		scl_int8* cmsg = (scl_int8*) _scl_alloc(64 + strlen(((Struct*) instance)->type_name) + strlen(target_type_name));
		sprintf(cmsg, "Cannot cast instance of struct '%s' to type '%s'\n", ((Struct*) instance)->type_name, target_type_name);
		_scl_CastError* err = NEW(CastError, Error);
		_scl_push()->v = str_of(cmsg);
		_scl_push()->v = err;
		_scl_call_method_or_throw(err, initHash, hash1("(s;)V;"), 0, "init", "(s;)V;");
		_scl_throw(err);
	}
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
#define SCL_DEFAULT_STACK_FRAME_COUNT 16
#endif

void _scl_stack_new() {
	// These use C's malloc, keep it that way
	// They should NOT be affected by any future
	// stuff we might do with _scl_alloc()
	_stack.ptr = 0;
	_stack.cap = SCL_DEFAULT_STACK_FRAME_COUNT;
	_stack.data = GC_malloc_uncollectable(sizeof(_scl_frame_t) * _stack.cap);

	insert_sorted((scl_any**) &stacks, &stacks_count, &_stack, &stacks_cap);

	_extable.cs_pointer = 0;
	_extable.jmp_buf_ptr = 0;
	_extable.capacity = SCL_DEFAULT_STACK_FRAME_COUNT;
	_extable.cs_pointer = system_allocate(_extable.capacity * sizeof(scl_int));
	_extable.exceptions = GC_malloc_uncollectable(_extable.capacity * sizeof(scl_any));
	_extable.jmp_buf = system_allocate(_extable.capacity * sizeof(jmp_buf));
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

	system_free(_extable.cs_pointer);
	GC_free(_extable.exceptions);
	system_free(_extable.jmp_buf);
	_extable.cs_pointer = 0;
	_extable.jmp_buf_ptr = 0;
	_extable.capacity = 0;
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

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshift-count-overflow"
#endif
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
		_scl_push()->v = val;
		_scl_call_method_or_throw(val, toStringHash, toStringSigHash, 0, "toString", "()s;");
		s = _scl_pop()->s;
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
		_scl_push()->v = val;
		_scl_call_method_or_throw(val, toStringHash, toStringSigHash, 0, "toString", "()s;");
		s = _scl_pop()->s;
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
	return int$toHexString(*(scl_int*) &val);
}

scl_float float$fromBits(scl_any bits) {
	return *(scl_float*) &bits;
}

scl_bool Struct$setAccessible0(_scl_Struct* self, scl_bool accessible, scl_str name) {
	Process$lock((volatile scl_any) self);
	for (scl_int i = 0; i < (self)->members_count; i++) {
		struct _scl_membertype member = ((struct _scl_membertype*) (self)->members)[i];
		if (member.pure_name == (name)->_hash) {
			scl_int flags = ((struct _scl_membertype*) (self)->members)[i].access_flags;
			if (accessible) {
				flags &= 0b01111;
			} else {
				flags |= 0b10000;
			}
			((struct _scl_membertype*) (self)->members)[i].access_flags = flags;

			Process$unlock((volatile scl_any) self);
			return 1;
		}
	}

	Process$unlock((volatile scl_any) self);
	return 0;
}

void dumpStack() {
	printf("Dump:\n");
	for (ssize_t i = _stack.ptr - 1; i >= 0; i--) {
		scl_int v = _stack.data[i].i;
		printf("   %zd: 0x" SCL_INT_HEX_FMT ", " SCL_INT_FMT "\n", i, v, v);
	}
	printf("\n");
}

scl_any Library$self0() {
	scl_any lib = dlopen(nil, RTLD_LAZY);
	if (!lib) {
		scl_NullPointerException* e = NEW(NullPointerException, Exception);
		_scl_push()->v = e;
		_scl_call_method_or_throw(e, initHash, initSigHash, 0, "init", "()V;");
		e->self.msg = str_of("Failed to load library");
		_scl_throw(e);
	}
	return lib;
}

scl_any Library$getSymbol0(scl_any lib, scl_int8* name) {
	return dlsym(lib, name);
}

scl_any Library$open0(scl_int8* name) {
	scl_any lib = dlopen(name, RTLD_LAZY);
	if (!lib) {
		scl_NullPointerException* e = NEW(NullPointerException, Exception);
		_scl_push()->v = e;
		_scl_call_method_or_throw(e, initHash, initSigHash, 0, "init", "()V;");
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
	struct Struct_Array* arr = NEW(ReadOnlyArray, Array);
	
	arr->capacity = (scl_any) (_callstack.ptr);
	arr->initCapacity = _callstack.ptr;
	arr->count = 0;
	arr->values = _scl_alloc((_callstack.ptr) * sizeof(scl_str));
	
	for (scl_int i = 0; i < _callstack.ptr; i++) {
		((scl_str*) arr->values)[(scl_int) arr->count++] = _scl_create_string(_callstack.func[i]);
	}
	return arr;
}

scl_bool Process$gcEnabled() {
	return !GC_is_disabled();
}
void Process$lock0(mutex_t mutex) {
	_callstack.func[_callstack.ptr++] = "<runtime Process::lock0(mutex: mutex_t): none>";
	pthread_mutex_lock(mutex);
	_callstack.func[--_callstack.ptr] = NULL;
}
void Process$lock(volatile scl_any obj) {
	if (_scl_expect(!obj, 0)) return;
	Process$lock0(((volatile Struct*) obj)->mutex);
}
void Process$unlock0(mutex_t mutex) {
	_callstack.func[_callstack.ptr++] = "<runtime Process::unlock0(mutex: mutex_t): none>";
	pthread_mutex_unlock(mutex);
	_callstack.func[--_callstack.ptr] = NULL;
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

extern scl_Array Var_Thread$threads;
extern scl_Thread Var_Thread$mainThread;
static tls scl_Thread _currentThread = nil;

static scl_int8* subStringOf(scl_int8* str, size_t len, size_t beg, size_t end) {
	scl_int8* sub = _scl_alloc(len);
	strcpy(sub, str + beg);
	sub[end - beg] = '\0';
	return sub;
}

#define eq(a, b) (strcmp(a, b) == 0)
scl_int8* typeToRTSig(scl_int8* type) {
	if (eq(type, "any")) return "a;";
	if (eq(type, "int") || eq(type, "bool")) return "i;";
	if (eq(type, "float")) return "f;";
	if (eq(type, "str")) return "s;";
	if (eq(type, "none")) return "V;";
	if (eq(type, "[int8]")) return "cs;";
	if (eq(type, "[any]")) return "p;";
	size_t len = strlen(type);
	if (len > 2 && type[0] == '[' && type[len - 1] == ']') {
		scl_int8* tmp = _scl_alloc(len + 2);
		snprintf(tmp, len + 2, "[%s", typeToRTSig(subStringOf(type, len, 1, len - 1)));
		return tmp;
	}
	if (eq(type, "int8")) return "int8;";
	if (eq(type, "int16")) return "int16;";
	if (eq(type, "int32")) return "int32;";
	if (eq(type, "int64")) return "int64;";
	if (eq(type, "uint8")) return "uint8;";
	if (eq(type, "uint16")) return "uint16;";
	if (eq(type, "uint32")) return "uint32;";
	if (eq(type, "uint64")) return "uint64;";
	if (eq(type, "uint")) return "u;";
	scl_int8* tmp = _scl_alloc(len + 3);
	snprintf(tmp, len + 3, "L%s;", type);
	return tmp;
}

scl_str typeArrayToRTSig(scl_Array arr) {
	scl_str s = str_of("");
	scl_int strHash = hash1("str");
	scl_int getHash = hash1("get");
	scl_int getSigHash = hash1("(i;)a;");
	scl_int appendHash = hash1("append");
	scl_int appendSigHash = hash1("(cs;)s;");
	for (scl_int i = 0; i < arr->count; i++) {
		_scl_push()->v = arr;
		_scl_call_method_or_throw(arr, getHash, getSigHash, 0, "get", "(i;)a;");
		scl_str type = _scl_pop()->v;
		_scl_assert(_scl_is_instance_of(type, strHash), "typeArrayToRTSig: type is not a string");
		_scl_push()->v = typeToRTSig(type->_data);
		_scl_push()->v = s;
		_scl_call_method_or_throw(s, appendHash, appendSigHash, 0, "append", "(cs;)s;");
		s = _scl_pop()->v;
	}
}

scl_str typesToRTSignature(scl_str returnType, scl_Array args) {
	scl_int8* retType = typeToRTSig(returnType->_data);
	scl_str argTypes = typeArrayToRTSig(args);
	scl_str sig = str_of("(");
	_scl_push()->v = argTypes;
	_scl_push()->v = sig;
	_scl_call_method_or_throw(sig, hash1("append"), hash1("(s;)s;"), 0, "append", "(s;)s;");
	sig = _scl_pop()->v;
	_scl_push()->v = ")";
	_scl_push()->v = sig;
	_scl_call_method_or_throw(sig, hash1("append"), hash1("(cs;)s;"), 0, "append", "(cs;)s;");
	sig = _scl_pop()->v;
	_scl_push()->v = retType;
	_scl_push()->v = sig;
	_scl_call_method_or_throw(sig, hash1("append"), hash1("(cs;)s;"), 0, "append", "(cs;)s;");
	sig = _scl_pop()->v;
	return sig;
}

scl_str Struct$convertToSignature(scl_str returnType, scl_Array args) {
	return typesToRTSignature(returnType, args);
}

void Thread$run(scl_Thread self) {
	_callstack.func[_callstack.ptr++] = "<extern Thread:run(): none>";
	_currentThread = self;
	_scl_stack_new();

	_scl_push()->v = self;
	_scl_push()->v = Var_Thread$threads;
	_scl_call_method_or_throw(Var_Thread$threads, hash1("push"), hash1("(a;)V;"), 0, "push", "(a;)V;");
	
	self->function();

	_scl_push()->v = self;
	_scl_push()->v = Var_Thread$threads;
	_scl_call_method_or_throw(Var_Thread$threads, hash1("remove"), hash1("(a;)V;"), 0, "remove", "(a;)V;");
	
	_scl_stack_free();
	_currentThread = nil;
	_callstack.func[--_callstack.ptr] = NULL;
}

scl_int Thread$start0(scl_Thread self) {
	int ret = GC_pthread_create(&self->nativeThread, 0, (scl_any) Thread$run, self);
	return ret;
}

scl_int Thread$stop0(scl_Thread self) {
	int ret = GC_pthread_join(self->nativeThread, 0);
	return ret;
}

scl_any _scl_get_main_addr();

scl_Thread Thread$currentThread() {
	if (!_currentThread) {
		_currentThread = NEW0(Thread);
		_currentThread->name = str_of("Main Thread");
		_currentThread->nativeThread = pthread_self();
		_currentThread->function = _scl_get_main_addr();
	}
	return _currentThread;
}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

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
	if (_scl_is_instance_of(ex, hash1("Error"))) {
		_extable.jmp_buf_ptr = 0;
	} else {
		_extable.jmp_buf_ptr--;
		if (_extable.jmp_buf_ptr < 0) {
			_extable.jmp_buf_ptr = 0;
		}
	}
	_extable.exceptions[_extable.jmp_buf_ptr] = ex;
	_callstack.ptr = _extable.cs_pointer[_extable.jmp_buf_ptr];
	longjmp(_extable.jmp_buf[_extable.jmp_buf_ptr], 666);
}

// Returns a function pointer with the following signature:
// function main(args: Array, env: Array): int
typedef int(*mainFunc)(struct Struct_Array*, struct Struct_Array*);

// __init__ and __destroy__
typedef void(*genericFunc)();

// __init__
// last element is always nil
extern genericFunc _scl_internal_init_functions[];

// __destroy__
// last element is always nil
extern genericFunc _scl_internal_destroy_functions[];

void* _scl_oom(scl_uint size) {
	fprintf(stderr, "Out of memory! Tried to allocate " SCL_UINT_FMT " bytes.\n", size);
	return nil;
}

// Struct::structsCount: int
extern scl_int Struct$structsCount
	__asm(_scl_macro_to_string(__USER_LABEL_PREFIX__) "Var_Struct$structsCount");

int main(int argc, char** argv) {

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

	argv0 = argv[0];

	// Register signal handler for all available signals
	_scl_set_up_signal_handler();

	runtimeModifierLock = NEWOBJ();
	binarySearchLock = NEWOBJ();
	_scl_create_stack();

	// Convert argv and envp from native arrays to Scale arrays
	struct Struct_Array* args = nil;
	struct Struct_Array* env = nil;

#if !defined(SCL_MAIN_ARG_COUNT)
#define SCL_MAIN_ARG_COUNT 0
#endif
#if SCL_MAIN_ARG_COUNT > 0
	args = (struct Struct_Array*) _scl_c_arr_to_scl_array((scl_any*) argv);
#endif
#if SCL_MAIN_ARG_COUNT > 1
	env = (struct Struct_Array*) _scl_c_arr_to_scl_array((scl_any*) _scl_platform_get_env());
#endif

	// Get the address of the main function
	mainFunc _scl_main = (mainFunc) _scl_get_main_addr();
	_scl_exception_push();
	Var_Thread$mainThread = _currentThread = Thread$currentThread();

	Struct$structsCount = _scl_types_count;

	// Run __init__ functions
	if (_scl_internal_init_functions[0]) {
		_extable.jmp_buf_ptr = 1;
		if (setjmp(_extable.jmp_buf[_extable.jmp_buf_ptr - 1]) != 666) {
			for (int i = 0; _scl_internal_init_functions[i]; i++) {
				_callstack.ptr = 0;
				_scl_internal_init_functions[i]();
			}
		} else {
			scl_str msg = ((_scl_Exception) _extable.exceptions[_extable.jmp_buf_ptr])->msg;

			_scl_push()->v = _extable.exceptions[_extable.jmp_buf_ptr];
			_scl_call_method_or_throw(_extable.exceptions[_extable.jmp_buf_ptr], hash1("printStackTrace"), initSigHash, 0, "printStackTrace", "()V;");

			if (msg) {
				_scl_security_throw(EX_THROWN, "Uncaught exception: %s", msg->_data);
			} else {
				_scl_security_throw(EX_THROWN, "Uncaught exception");
			}
		}
	}

	int ret;
	_extable.jmp_buf_ptr = 1;
	_callstack.ptr = 0;
	if (setjmp(_extable.jmp_buf[_extable.jmp_buf_ptr - 1]) != 666) {

		// _scl_get_main_addr() returns nil if compiled with --no-main
#if defined(SCL_MAIN_RETURN_NONE)
		ret = 0;
		(_scl_main ? _scl_main(args, env) : 0);
#else
		ret = (_scl_main ? _scl_main(args, env) : 0);
#endif
	} else {

		scl_str msg = ((_scl_Exception) _extable.exceptions[_extable.jmp_buf_ptr])->msg;

		_scl_push()->v = _extable.exceptions[_extable.jmp_buf_ptr];
		_scl_call_method_or_throw(_extable.exceptions[_extable.jmp_buf_ptr], hash1("printStackTrace"), initSigHash, 0, "printStackTrace", "()V;");
		if (msg) {
			_scl_security_throw(EX_THROWN, "Uncaught exception: %s", msg->_data);
		} else {
			_scl_security_throw(EX_THROWN, "Uncaught exception");
		}
	}

	// Run finalization:
	// Run __destroy__ functions
	if (_scl_internal_destroy_functions[0]) {
		_extable.jmp_buf_ptr = 1;
		if (setjmp(_extable.jmp_buf[_extable.jmp_buf_ptr - 1]) != 666) {
			for (int i = 0; _scl_internal_destroy_functions[i]; i++) {
				_scl_internal_destroy_functions[i]();
			}
		} else {
			scl_str msg = ((_scl_Exception) _extable.exceptions[_extable.jmp_buf_ptr])->msg;

			_scl_push()->v = _extable.exceptions[_extable.jmp_buf_ptr];
			_scl_call_method_or_throw(_extable.exceptions[_extable.jmp_buf_ptr], hash1("printStackTrace"), initSigHash, 0, "printStackTrace", "()V;");
			if (msg) {
				_scl_security_throw(EX_THROWN, "Uncaught exception: %s", msg->_data);
			} else {
				_scl_security_throw(EX_THROWN, "Uncaught exception");
			}
		}
	}

	GC_deinit();
	return ret;
}
