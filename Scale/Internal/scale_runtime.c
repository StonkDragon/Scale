#include "scale_runtime.h"

const hash SclObjectHash = 0xC9CCFE34U;

// The Stack
__thread _scl_stack_t		_stack;

// Scale Callstack
__thread _scl_callstack_t	_callstack;

// this is used by try-catch
__thread struct _exception_handling {
	scl_any*	exceptions;		// The exception
	jmp_buf*	jmp_buf;		// jump buffer used by longjmp()
	scl_int		jmp_buf_ptr;	// number specifying the current depth
	scl_int 	capacity;		// capacity of lists
	scl_int*	cs_pointer;		// callstack-pointer of this catch level
} _extable = {0};

#define unimplemented do { fprintf(stderr, "%s:%d: %s: Not Implemented\n", __FILE__, __LINE__, __FUNCTION__); exit(1) } while (0)

static _scl_stack_t** stacks;
static scl_int stacks_count = 0;
static scl_int stacks_cap = 64;

static scl_any*	allocated;				// List of allocated pointers
static scl_int	allocated_count = 0;	// Length of the list
static scl_int	allocated_cap = 64;		// Capacity of the list

static scl_str*	strings;				// List of allocated string instances
static scl_int	strings_count = 0;		// Length of the list
static scl_int	strings_cap = 64;		// Capacity of the list

static scl_int*	memsizes;				// List of pointer sizes
static scl_int	memsizes_count = 0;		// Length of the list
static scl_int	memsizes_cap = 64;		// Capacity of the list

// generic struct
typedef struct Struct {
	scl_int		type;
	scl_int8*	type_name;
	scl_int		super;
	scl_int		size;
} Struct;

typedef struct Struct_Exception {
	Struct asSclObject;
	scl_str msg;
	struct Struct_Array* stackTrace;
	scl_str errno_str;
}* _scl_Exception;

// table of instances
static Struct** instances;
static scl_int instances_count = 0;
static scl_int instances_cap = 64;

// Inserts value into array at it's sorted position
// returns the index at which the value was inserted
// will not insert value, if it is already present in the array
static scl_int insert_sorted(scl_any** array, scl_int* size, scl_any value, scl_int* cap) {
	if (*array == NULL) {
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
	if ((*array)[i] == value) return i;

	if ((*size) + 1 >= (*cap)) {
		(*cap) += 64;
		(*array) = system_realloc((*array), (*cap) * sizeof(scl_any*));
	}

	scl_int j;
	for (j = *size; j > i; j--) {
		(*array)[j] = (*array)[j-1];
	}
	(*array)[i] = value;

	(*size)++;
	return i;
}

void _scl_remove_stack(_scl_stack_t* stack) {
	scl_int index;
	while ((index = _scl_stack_index(stack)) != -1) {
		_scl_remove_stack_at(index);
	}
}

scl_int _scl_stack_index(_scl_stack_t* stack) {
	return _scl_binary_search((scl_any*) stacks, stacks_count, stack);
}

void _scl_remove_stack_at(scl_int index) {
	for (scl_int i = index; i < stacks_count - 1; i++) {
		stacks[i] = stacks[i + 1];
	}
	stacks_count--;
}

void _scl_remove_ptr(scl_any ptr) {
	scl_int index;
	// Finds all indices of the pointer in the table and removes them
	while ((index = _scl_get_index_of_ptr(ptr)) != -1) {
		_scl_remove_ptr_at_index(index);
		memsizes[index] = 0;
	}
}

// Returns the next index of a given pointer in the allocated-pointers array
// Returns -1, if the pointer is not in the table
scl_int _scl_get_index_of_ptr(scl_any ptr) {
	return _scl_binary_search(allocated, allocated_count, ptr);
}

// Removes the pointer at the given index and shift everything after left
void _scl_remove_ptr_at_index(scl_int index) {
	for (scl_int i = index; i < allocated_count - 1; i++) {
		allocated[i] = allocated[i + 1];
	}
	allocated_count--;
}

scl_int _scl_sizeof(scl_any ptr) {
	scl_int index = _scl_get_index_of_ptr(ptr);
	if (index == -1) return -1;
	return memsizes[index];
}

// Adds a new pointer to the table
void _scl_add_ptr(scl_any ptr, scl_int size) {
	scl_int index = insert_sorted((scl_any**) &allocated, &allocated_count, ptr, &allocated_cap);
	memsizes_count++;
	if (index >= memsizes_cap) {
		memsizes_cap = index;
		memsizes = system_realloc(memsizes, memsizes_cap * sizeof(scl_any));
	} else if (memsizes_count >= memsizes_cap) {
		memsizes_cap += 64;
		memsizes = system_realloc(memsizes, memsizes_cap * sizeof(scl_any));
	}
	memsizes[index] = size;
}

// Returns true if the pointer was allocated using _scl_alloc()
scl_int _scl_check_allocated(scl_any ptr) {
	return _scl_get_index_of_ptr(ptr) != -1;
}

// Allocates size bytes of memory
// the memory will always have a size of a multiple of sizeof(scl_int)
scl_any _scl_alloc(scl_int size) {

	// Make size be next biggest multiple of 8
	size = ((size + 7) >> 3) << 3;

	// Allocate the memory
	scl_any ptr = system_allocate(size);

	// Hard-throw if memory allocation failed
	if (!ptr) {
		_scl_security_throw(EX_BAD_PTR, "allocate() failed!");
		return NULL;
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

	// Make size be next biggest multiple of sizeof(scl_int)
	if (size % sizeof(scl_any) != 0) {
		size += size % sizeof(scl_any);
	}

	int wasStruct = _scl_find_index_of_struct(ptr) != -1;

	// If this pointer points to an instance of a struct, quietly deallocate it
	// without calling SclObject:finalize() on it
	_scl_free_struct_no_finalize(ptr);
	
	// Remove the pointer from the table
	_scl_remove_ptr(ptr);

	// reallocate and reassign it
	// At this point it is unsafe to use the old pointer!
	ptr = system_realloc(ptr, size);

	// Hard-throw if memory allocation failed
	if (!ptr) {
		_scl_security_throw(EX_BAD_PTR, "realloc() failed!");
		return NULL;
	}

	// Add the pointer to the table
	_scl_add_ptr(ptr, size);

	// Add the pointer back to our struct table, if it was a struct
	if (wasStruct)
		_scl_add_struct(ptr);
	return ptr;
}

// Frees a pointer
// Will do nothing, if the pointer is NULL, or wasn't allocated with _scl_alloc()
void _scl_free(scl_any ptr) {
	if (ptr && _scl_check_allocated(ptr)) {
		// If this is an instance, call it's finalizer
		_scl_free_struct(ptr);

		// Remove the pointer from our table
		_scl_remove_ptr(ptr);

		// Finally, free the memory
		// accessing the pointer is now unsafe!
		system_free(ptr);
	}
}

void _ZN5Error4initEP3strP5Error(scl_any, scl_str);

// Assert, that 'b' is true
void _scl_assert(scl_int b, scl_int8* msg) {
	if (!b) {
		typedef struct Struct_AssertError {
			Struct _structData;
			scl_str msg;
			struct Struct_Array* stackTrace;
			scl_str errno_str;
		} _scl_AssertError;

		scl_int8* cmsg = (scl_int8*) _scl_alloc(strlen(msg) + 20);
		sprintf(cmsg, "Assertion failed: %s\n", msg);
		_scl_AssertError* err = NEW(AssertError, Error);
		_ZN5Error4initEP3strP5Error(err, str_of(cmsg));

		_scl_throw(err);
	}
}

// Hard-throw an exception
_scl_no_return void _scl_security_throw(int code, scl_int8* msg, ...) {
	remove("scl_trace.log");
	FILE* trace = fopen("scl_trace.log", "a");
	printf("\n");
	fprintf(trace, "\n");

	va_list args;
	va_start(args, msg);
	
	char* str = system_allocate(strlen(msg) * 8);
	vsprintf(str, msg, args);
	printf("Exception: %s\n", str);
	fprintf(trace, "Exception: %s\n", str);

	va_end(args);

	if (errno) {
		printf("errno: %s\n", strerror(errno));
		fprintf(trace, "errno: %s\n", strerror(errno));
	}
	print_stacktrace_with_file(trace);

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
	struct Struct_str {
		struct scaleString s;
	};
	assert(sizeof(struct Struct_str) == sizeof(struct scaleString));

	scl_str self = NEW0(str);
	if (self == NULL) {
		_scl_security_throw(EX_BAD_PTR, "Failed to allocate memory for cstr '%s'\n", data);
		return NULL;
	}
	if (_scl_find_index_of_struct(self) == -1) {
		_scl_security_throw(EX_BAD_PTR, "Could not create string instance for cstr '%s'. Index is: " SCL_INT_FMT "\n", data, _scl_find_index_of_struct(self));
		return NULL;
	}
	self->_len = strlen(data);
	self->_hash = hash1len(data, self->_len);
	self->_data = _scl_strndup(data, self->_len);
	return self;
}

static int printingStacktrace = 0;

void print_stacktrace() {
	printingStacktrace = 1;

	for (signed long i = _callstack.ptr - 1; i >= 0; i--) {
		printf("  %s\n", (scl_int8*) _callstack.func[i]);
	}

	printingStacktrace = 0;
	printf("\n");
}


void print_stacktrace_with_file(FILE* trace) {
	printingStacktrace = 1;

	if (_callstack.ptr == 0) {
		printingStacktrace = 0;
		return;
	}

	printf("Stacktrace:\n");
	fprintf(trace, "Stacktrace:\n");

	for (signed long i = _callstack.ptr - 1; i >= 0; i--) {
		printf("  %s\n", (scl_int8*) _callstack.func[i]);
		fprintf(trace, "  %s\n", (scl_int8*) _callstack.func[i]);
	}

	printingStacktrace = 0;
	printf("\n");
	fprintf(trace, "\n");
}

// final signal handler
// if we get here, something has gone VERY wrong
void _scl_default_signal_handler(scl_int sig_num) {
	scl_int8* signalString;
	// Signals
	if (sig_num == -1) signalString = NULL;
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

	remove("scl_trace.log");
	FILE* trace = fopen("scl_trace.log", "a");

	printf("Signal: %s\n", signalString);
	fprintf(trace, "Signal: %s\n", signalString);
	if (errno) {
		printf("errno: %s\n", strerror(errno));
		fprintf(trace, "errno: %s\n", strerror(errno));
	}
	if (!printingStacktrace)
		print_stacktrace_with_file(trace);
	printf("Stack address: %p\n", _stack.data);
	fprintf(trace, "Stack address: %p\n", _stack.data);
	if (_stack.ptr && _stack.ptr < _stack.cap) {
		printf("Stack:\n");
		fprintf(trace, "Stack:\n");
		printf("SP: " SCL_INT_FMT "\n", _stack.ptr);
		fprintf(trace, "SP: " SCL_INT_FMT "\n", _stack.ptr);
		for (scl_int i = _stack.ptr - 1; i >= 0; i--) {
			scl_int v = _stack.data[i].i;
			printf("   " SCL_INT_FMT ": 0x" SCL_INT_HEX_FMT ", " SCL_INT_FMT "\n", i, v, v);
			fprintf(trace, "   " SCL_INT_FMT ": 0x" SCL_INT_HEX_FMT ", " SCL_INT_FMT "\n", i, v, v);
		}
		printf("\n");
		fprintf(trace, "\n");
	}
	fprintf(trace, "Memory Dump:\n");
	hash strHash = hash1("str");
	for (scl_int i = 0; i < allocated_count; i++) {
		if (_scl_find_index_of_struct(allocated[i]) != -1) {
			fprintf(trace, "  Instance of struct '%s':\n", ((Struct*) allocated[i])->type_name);
		} else {
			fprintf(trace, "  " SCL_INT_FMT " bytes at %p:\n", memsizes[i], allocated[i]);
		}
		if (_scl_is_instance_of(allocated[i], strHash)) {
			fprintf(trace, "    String value: '%s'\n", ((scl_str) allocated[i])->_data);
		} else {
			fprintf(trace, "    |");
			for (scl_int col = 0; col < (memsizes[i] < 16 ? memsizes[i] : 16); col++) {
				fprintf(trace, "%02x|", (char) ((col & 0xFF) + (((scl_int) allocated[i]) & 0xF)));
			}
			fprintf(trace, "\n");
			fprintf(trace, "    |");
			for (scl_int col = 0; col < (memsizes[i] < 16 ? memsizes[i] : 16); col++) {
				fprintf(trace, "--|");
			}
			fprintf(trace, "\n");
			for (scl_int sz = 0; sz < memsizes[i]; sz += 16) {
				fprintf(trace, "    |");
				for (scl_int col = 0; col < (memsizes[i] < 16 ? memsizes[i] : 16); col++) {
					fprintf(trace, "%02x|", (*(char*) (allocated[i] + sz + col) & 0xFF));
				}
				fprintf(trace, "\n");
			}
		}
		fprintf(trace, "\n");
	}

	fclose(trace);
	exit(sig_num);
}

struct Struct_Array {
	scl_int $__type__;
	scl_int8* $__type_name__;
	scl_any $__super_list__;
	scl_any $__super_list_len__;
	scl_any values;
	scl_any count;
	scl_any capacity;
	scl_int initCapacity;
	scl_int pos;
};

struct Struct_ReadOnlyArray {
	struct Struct_Array self;
};

// Converts C NULL-terminated array to ReadOnlyArray-instance
scl_any _scl_c_arr_to_scl_array(scl_any arr[]) {
	struct Struct_Array* array = NEW(ReadOnlyArray, Array);
	scl_int cap = 0;
	while (arr[cap] != NULL) {
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

inline _scl_frame_t* ctrl_push_frame() {
	return &_stack.data[_stack.ptr++];
}

inline _scl_frame_t* ctrl_pop_frame() {
	return &_stack.data[--_stack.ptr];
}

inline void ctrl_push_string(scl_str c) {
	_scl_push()->s = c;
}

inline void ctrl_push_c_string(scl_int8* c) {
	_scl_push()->v = c;
}

inline void ctrl_push_double(scl_float d) {
	_scl_push()->f = d;
}

inline void ctrl_push_long(scl_int n) {
	_scl_push()->i = n;
}

inline void ctrl_push(scl_any n) {
	_scl_push()->v = n;
}

inline scl_int ctrl_pop_long() {
	return _scl_pop()->i;
}

inline scl_float ctrl_pop_double() {
	return _scl_pop()->f;
}

inline scl_str ctrl_pop_string() {
	return _scl_pop()->s;
}

inline scl_int8* ctrl_pop_c_string() {
	return _scl_pop()->v;
}

inline scl_any ctrl_pop() {
	return _scl_pop()->v;
}

inline scl_int ctrl_stack_size(void) {
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

inline scl_uint _scl_rotl(const scl_uint value, int shift) {
    return (value << shift) | (value >> ((sizeof(scl_uint) << 3) - shift));
}

inline scl_uint _scl_rotr(const scl_uint value, int shift) {
    return (value >> shift) | (value << ((sizeof(scl_uint) << 3) - shift));
}

scl_int _scl_identity_hash(scl_any _obj) {
	if (!_scl_is_instance_of(_obj, SclObjectHash)) {
		return *(scl_int*) &_obj;
	}
	Struct* obj = (Struct*) _obj;
	scl_int size = obj->size;
	scl_int hash = (*(scl_int*) &obj) % 17;
	for (scl_int i = 0; i < size; i++) {
		hash = _scl_rotl(hash, 5) ^ ((char*) obj)[i];
	}
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
};

struct _scl_typeinfo {
  scl_int					type;
  scl_int					super;
  scl_int					alloc_size;
  scl_int8*					name;
  scl_int					methodscount;
  struct _scl_methodinfo**	methods;
};

// Structs
extern struct _scl_typeinfo		_scl_types[];
extern scl_int 					_scl_types_count;

scl_int _scl_binary_search_typeinfo_index(struct _scl_typeinfo* types, scl_int count, hash type) {
	scl_int left = 0;
	scl_int right = count - 1;

	while (left <= right) {
		scl_int mid = (left + right) / 2;
		if (types[mid].type == type) {
			return mid;
		} else if (types[mid].type < type) {
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}

	return -1;
}

typedef struct Struct_Struct {
	Struct structData;
	scl_str typeName;
	scl_int typeId;
	struct Struct_Struct* super;
	scl_int binarySize;
	scl_bool isStatic;
} _scl_Struct;

static _scl_Struct** structs;
static scl_int structs_count = 0;
static scl_int structs_cap = 64;

scl_int _scl_binary_search_struct_struct_index(_scl_Struct** structs, scl_int count, hash type) {
	scl_int left = 0;
	scl_int right = count - 1;

	while (left <= right) {
		scl_int mid = (left + right) / 2;
		if (structs[mid]->typeId == type) {
			return mid;
		} else if (structs[mid]->typeId < type) {
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}

	return -1;
}

void _ZN9Exception4initEP9Exception(_scl_Exception);
void _ZN9SclObject4initEP9SclObject(Struct* self);

scl_any _scl_get_struct_by_id(scl_int id) {
	scl_int index = _scl_binary_search_struct_struct_index(structs, structs_count, id);
	if (index >= 0) return structs[index];
	if (id == 0) return NULL;
	index = _scl_binary_search_typeinfo_index(_scl_types, _scl_types_count, id);
	if (index < 0) {
		return NULL;
	}
	struct _scl_typeinfo t = _scl_types[index];
	_scl_Struct* s = NEW0(Struct);
	s->binarySize = t.alloc_size;
	s->typeName = str_of(t.name);
	s->typeId = id;
	s->super = _scl_get_struct_by_id(t.super);
	s->isStatic = s->binarySize == 0;
	insert_sorted((scl_any**) &structs, &structs_count, s, &structs_cap);
	return s;
}

typedef struct Struct_NullPointerException {
	struct Struct_Exception self;
} scl_NullPointerException;

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
// NULL if the struct does not exist
scl_any _scl_typeinfo_of(hash type) {
	scl_int index = _scl_binary_search_typeinfo_index(_scl_types, _scl_types_count, type);
	if (index >= 0) {
		return (scl_any) _scl_types + (index * sizeof(struct _scl_typeinfo));
	}
	return NULL;
}

struct _scl_typeinfo* _scl_find_typeinfo_of(hash type) {
	return (struct _scl_typeinfo*) _scl_typeinfo_of(type);
}

// returns the method handle of a method on a struct, or a parent struct
scl_any _scl_get_method_on_type(hash type, hash method) {
	struct _scl_typeinfo* p = _scl_find_typeinfo_of(type);
	while (p) {
		scl_int index = _scl_binary_search_method_index((void**) p->methods, p->methodscount, method);
		if (index >= 0) {
			return p->methods[index]->ptr;
		}
		p = _scl_find_typeinfo_of(p->super);
	}
	return NULL;
}

scl_any _scl_get_method_handle(hash type, hash method) {
	struct _scl_typeinfo* p = _scl_find_typeinfo_of(type);
	while (p) {
		scl_int index = _scl_binary_search_method_index((void**) p->methods, p->methodscount, method);
		if (index >= 0) {
			return p->methods[index]->actual_handle;
		}
		p = _scl_find_typeinfo_of(p->super);
	}
	return NULL;
}

// adds an instance to the table of tracked instances
scl_any _scl_add_struct(scl_any ptr) {
	insert_sorted((scl_any**) &instances, &instances_count, ptr, &instances_cap);
	return ptr;
}

// creates a new instance with a size of 'size'
scl_any _scl_alloc_struct(scl_int size, scl_int8* type_name, hash super) {

	// Allocate the memory
	scl_any ptr = _scl_alloc(size);

	if (ptr == NULL) return NULL;

	// Type name hash
	((Struct*) ptr)->type = hash1(type_name);

	// Type name
	((Struct*) ptr)->type_name = type_name;

	// Parent struct name hash
	((Struct*) ptr)->super = super;

	// Size (Currently only used by SclObject:clone())
	((Struct*) ptr)->size = size;

	// Add struct to allocated table
	scl_int index = insert_sorted((scl_any**) &instances, &instances_count, ptr, &instances_cap);
	if (index == -1) return NULL;
	return instances[index];
}

// Removes an instance from the allocated table by index
static void _scl_struct_map_remove(scl_int index) {
	for (scl_int i = index; i < instances_count - 1; i++) {
	   instances[i] = instances[i + 1];
	}
	instances_count--;
}

// Returns the next index of an instance in the allocated table
// Returns -1, if not in table
scl_int _scl_find_index_of_struct(scl_any ptr) {
	if (ptr == NULL) return -1;
	return _scl_binary_search((void**) instances, instances_count, ptr);
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
	if (_scl_find_index_of_struct(ptr) == -1)
		return;

	scl_int i = _scl_find_index_of_struct(ptr);
	while (i != -1) {
		_scl_struct_map_remove(i);
		i = _scl_find_index_of_struct(ptr);
	}
}

// Returns true if the given type is a child of the other type
scl_int _scl_type_extends_type(struct _scl_typeinfo* type, struct _scl_typeinfo* extends) {
	if (!type || !extends) return 0;
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
	int isStruct = _scl_binary_search((scl_any*) instances, instances_count, ptr) != -1;
	if (!isStruct) return 0;

	Struct* ptrStruct = (Struct*) ptr;

	if (ptrStruct->type == typeId) return 1;

	struct _scl_typeinfo* ptrType = _scl_typeinfo_of(ptrStruct->type);
	struct _scl_typeinfo* typeIdType = _scl_typeinfo_of(typeId);

	return _scl_type_extends_type(ptrType, typeIdType);
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

scl_int _scl_binary_search_method_index(scl_any* methods, scl_int count, hash id) {
	scl_int left = 0;
	scl_int right = count - 1;

	struct _scl_methodinfo** methods_ = (struct _scl_methodinfo**) methods;

	while (left <= right) {
		scl_int mid = (left + right) / 2;
		if (methods_[mid]->pure_name == id) {
			return mid;
		} else if (methods_[mid]->pure_name < id) {
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}

	return -1;
}

typedef void (*_scl_sigHandler)(scl_int);

void _scl_set_signal_handler(_scl_sigHandler handler, scl_int sig) {
	if (sig < 0 || sig >= 32) {
		struct Struct_InvalidSignalException {
			struct Struct_Exception self;
			scl_int sig;
		};
		_scl_Exception e = NEW(InvalidSignalException, Exception);
		_scl_push()->i = sig;
		_ZN9Exception4initEP9Exception(e);

		scl_int8* p = (scl_int8*) _scl_alloc(64);
		snprintf(p, 64, "Invalid signal: " SCL_INT_FMT, sig);
		e->msg = str_of(p);
		
		_scl_throw(e);
	}
	signal(sig, (void(*)(int)) handler);
}

void _scl_reset_signal_handler(scl_int sig) {
	if (sig < 0 || sig >= 32) {
		struct Struct_InvalidSignalException {
			struct Struct_Exception self;
			scl_int sig;
		};
		_scl_Exception e = NEW(InvalidSignalException, Exception);
		_scl_push()->i = sig;
		_ZN9Exception4initEP9Exception(e);

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
		_scl_frame_t* tmp = system_realloc(_stack.data, sizeof(_scl_frame_t) * _stack.cap);
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
		_scl_security_throw(EX_STACK_UNDERFLOW, "Not enough data on the stack!");
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
		_scl_security_throw(EX_STACK_UNDERFLOW, "Not enough data on the stack!");
	}
#endif

	return &_stack.data[_stack.ptr - 1];
}

void _scl_popn(scl_int n) {
	_stack.ptr -= n;

#if !defined(SCL_FEATURE_UNSAFE_STACK_ACCESSES)
	if (_scl_expect(_stack.ptr < 0, 0)) {
		_scl_security_throw(EX_STACK_UNDERFLOW, "Not enough data on the stack!");
	}
#endif
}

void _scl_exception_push() {
	_extable.jmp_buf_ptr++;
	if (_extable.jmp_buf_ptr >= _extable.capacity) {
		_extable.capacity += 64;
		scl_any* tmp;

		tmp = system_realloc(_extable.cs_pointer, _extable.capacity * sizeof(scl_int));
		if (!tmp) _scl_security_throw(EX_BAD_PTR, "realloc() failed");
		_extable.cs_pointer = (scl_int*) tmp;

		tmp = system_realloc(_extable.exceptions, _extable.capacity * sizeof(scl_any));
		if (!tmp) _scl_security_throw(EX_BAD_PTR, "realloc() failed");
		_extable.exceptions = tmp;

		tmp = system_realloc(_extable.jmp_buf, _extable.capacity * sizeof(jmp_buf));
		if (!tmp) _scl_security_throw(EX_BAD_PTR, "realloc() failed");
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

void _scl_check_not_nil_argument(scl_int val, scl_int8* name) {
	if (val == 0) {
		scl_int8* msg = (scl_int8*) system_allocate(sizeof(scl_int8) * strlen(name) + 64);
		snprintf(msg, 64 + strlen(name), "Argument %s is nil", name);
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
		_ZN5Error4initEP3strP5Error(err, str_of(cmsg));
		_scl_throw(err);
	}
}

void _scl_not_nil_cast(scl_int val, scl_int8* name) {
	if (val == 0) {
		scl_int8* msg = (scl_int8*) system_allocate(sizeof(scl_int8) * strlen(name) + 64);
		snprintf(msg, 64 + strlen(name), "Nil cannot be cast to non-nil type '%s'!", name);
		_scl_assert(0, msg);
	}
}

void _scl_struct_allocation_failure(scl_int val, scl_int8* name) {
	if (val == 0) {
		scl_int8* msg = (scl_int8*) system_allocate(sizeof(scl_int8) * strlen(name) + 64);
		snprintf(msg, 64 + strlen(name), "Failed to allocate memory for struct '%s'!", name);
		_scl_assert(0, msg);
	}
}

void _scl_nil_ptr_dereference(scl_int val, scl_int8* name) {
	if (val == 0) {
		scl_int8* msg = (scl_int8*) system_allocate(sizeof(scl_int8) * strlen(name) + 64);
		snprintf(msg, 64 + strlen(name), "Tried dereferencing nil pointer '%s'!", name);
		_scl_assert(0, msg);
	}
}

void _scl_check_not_nil_store(scl_int val, scl_int8* name) {
	if (val == 0) {
		scl_int8* msg = (scl_int8*) system_allocate(sizeof(scl_int8) * strlen(name) + 64);
		snprintf(msg, 64 + strlen(name), "Nil cannot be stored in non-nil variable '%s'!", name);
		_scl_assert(0, msg);
	}
}

void _scl_not_nil_return(scl_int val, scl_int8* name) {
	if (val == 0) {
		scl_int8* msg = (scl_int8*) system_allocate(sizeof(scl_int8) * strlen(name) + 64);
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
	_stack.data = system_allocate(sizeof(_scl_frame_t) * _stack.cap);

	insert_sorted((scl_any**) &stacks, &stacks_count, &_stack, &stacks_cap);

	_extable.cs_pointer = 0;
	_extable.jmp_buf_ptr = 0;
	_extable.capacity = SCL_DEFAULT_STACK_FRAME_COUNT;
	_extable.cs_pointer = system_allocate(_extable.capacity * sizeof(scl_int));
	_extable.exceptions = system_allocate(_extable.capacity * sizeof(scl_any));
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
	system_free(_stack.data);
	_stack.ptr = 0;
	_stack.cap = 0;
	_stack.data = 0;

	_scl_remove_stack(&_stack);

	system_free(_extable.cs_pointer);
	system_free(_extable.exceptions);
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

void _scl_throw(void* ex) {
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

// function Exception:printStackTrace(): none
void _ZN9Exception15printStackTraceEP9Exception(scl_any self);

// Returns a function pointer with the following signature:
// function main(args: Array, env: Array): int
scl_any _scl_get_main_addr();
typedef int(*mainFunc)(struct Struct_Array*, struct Struct_Array*);

// __init__ and __destroy__
typedef void(*genericFunc)();

// __init__
// last element is always NULL
extern genericFunc _scl_internal_init_functions[];

// __destroy__
// last element is always NULL
extern genericFunc _scl_internal_destroy_functions[];

// Struct::structsCount: int
extern scl_int Struct$structsCount
	__asm(_scl_macro_to_string(__USER_LABEL_PREFIX__) "Var_Struct$structsCount");

#if !defined(SCL_COMPILER_NO_MAIN)

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

	// Register signal handler for all available signals
	_scl_set_up_signal_handler();
	_scl_create_stack();

	// Convert argv and envp from native arrays to Scale arrays
	struct Struct_Array* args = NULL;
	struct Struct_Array* env = NULL;

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
#else

// Initialize as library
_scl_constructor void _scl_load() {

#if !defined(SCL_KNOWN_LITTLE_ENDIAN)
	// Endian-ness detection
	short word = 0x0001;
	char *byte = (char*) &word;
	if (!byte[0]) {
		fprintf(stderr, "Invalid byte order detected!");
		exit(-1);
	}
#endif

	_scl_create_stack();

#endif

	Struct$structsCount = _scl_types_count;

	// Run __init__ functions
	int init_jmp = setjmp(_extable.jmp_buf[_extable.jmp_buf_ptr - 1]);
	if (init_jmp != 666) {
		for (int i = 0; _scl_internal_init_functions[i]; i++) {
			_scl_internal_init_functions[i]();
		}
	} else {
		scl_str msg = ((_scl_Exception) _extable.exceptions[_extable.jmp_buf_ptr])->msg;

		_ZN9Exception15printStackTraceEP9Exception(_extable.exceptions[_extable.jmp_buf_ptr]);
		if (msg) {
			_scl_security_throw(EX_THROWN, "Uncaught exception: %s", msg->_data);
		} else {
			_scl_security_throw(EX_THROWN, "Uncaught exception");
		}
	}

#if !defined(SCL_COMPILER_NO_MAIN)

	int ret;
	_scl_exception_push();
	int main_jmp = setjmp(_extable.jmp_buf[_extable.jmp_buf_ptr - 1]);
	if (main_jmp != 666) {

		// _scl_get_main_addr() returns NULL if compiled with --no-main
#if defined(SCL_MAIN_RETURN_NONE)
		ret = 0;
		(_scl_main ? _scl_main(args, env) : 0);
#else
		ret = (_scl_main ? _scl_main(args, env) : 0);
#endif
	} else {

		scl_str msg = ((_scl_Exception) _extable.exceptions[_extable.jmp_buf_ptr])->msg;

		_ZN9Exception15printStackTraceEP9Exception(_extable.exceptions[_extable.jmp_buf_ptr]);
		if (msg) {
			_scl_security_throw(EX_THROWN, "Uncaught exception: %s", msg->_data);
		} else {
			_scl_security_throw(EX_THROWN, "Uncaught exception");
		}
	}

#else
}

// Uninitialize library
_scl_destructor void _scl_destroy() {
#endif

	// Run finalization:
	// Run __destroy__ functions
	int destroy_jmp = setjmp(_extable.jmp_buf[_extable.jmp_buf_ptr - 1]);
	if (destroy_jmp != 666) {
		for (int i = 0; _scl_internal_destroy_functions[i]; i++) {
			_scl_internal_destroy_functions[i]();
		}
	} else {
		scl_str msg = ((_scl_Exception) _extable.exceptions[_extable.jmp_buf_ptr])->msg;

		_ZN9Exception15printStackTraceEP9Exception(_extable.exceptions[_extable.jmp_buf_ptr]);
		if (msg) {
			_scl_security_throw(EX_THROWN, "Uncaught exception: %s", msg->_data);
		} else {
			_scl_security_throw(EX_THROWN, "Uncaught exception");
		}
	}

#if !defined(SCL_COMPILER_NO_MAIN)
	return ret;
#endif
}
