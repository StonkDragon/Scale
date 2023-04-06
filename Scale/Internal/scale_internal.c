#include "scale_internal.h"

// Variables

// The Stack
__thread _scl_stack_t 		_stack _scl_align;

// Scale Callstack
__thread _scl_callstack_t	_callstack _scl_align;

// this is used by try-catch
__thread struct _exception_handling {
	scl_any*	exceptions;		// The exception
	jmp_buf*	jmp_buf;		// jump buffer used by longjmp()
	scl_int		jmp_buf_ptr;	// number specifying the current depth
	scl_int 	capacity;		// capacity of lists
	scl_int*	cs_pointer;		// callstack-pointer of this catch level
} _extable _scl_align = {0};

#define unimplemented do { fprintf(stderr, "%s:%d: %s: Not Implemented\n", __FILE__, __LINE__, __FUNCTION__); exit(1) } while (0)

static scl_any*	allocated;				// List of allocated pointers
static scl_int	allocated_count = 0;	// Length of the list
static scl_int	allocated_cap = 64;		// Capacity of the list

static scl_str*	strings;				// List of allocated string instances
static scl_int	strings_count = 0;		// Length of the list
static scl_int	strings_cap = 64;		// Capacity of the list

static size_t*	memsizes;				// List of pointer sizes
static scl_int	memsizes_count = 0;		// Length of the list
static scl_int	memsizes_cap = 64;		// Capacity of the list

// generic struct
typedef struct Struct {
	scl_int		type;
	scl_int8*	type_name;
	scl_int		super;
	scl_int		size;
} Struct;

// table of instances
static Struct** instances;
static scl_int instances_count = 0;
static scl_int instances_cap = 64;

// Inserts value into array at it's sorted position
// returns the index at which the value was inserted
// will not insert value, if it is already present in the array
static scl_int insert_sorted(scl_any** array, scl_int* size, scl_any value, scl_int* cap) {
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
		(*array) = realloc((*array), (*cap) * sizeof(scl_any*));
	}

	scl_int j;
	for (j = *size; j > i; j--) {
		(*array)[j] = (*array)[j-1];
	}
	(*array)[i] = value;

	(*size)++;
	return i;
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

// Adds a new pointer to the table
void _scl_add_ptr(scl_any ptr, size_t size) {
	scl_int index = insert_sorted((scl_any**) &allocated, &allocated_count, ptr, &allocated_cap);
	memsizes_count++;
	if (index >= memsizes_cap) {
		memsizes_cap = index;
		memsizes = realloc(memsizes, memsizes_cap * sizeof(scl_any));
	} else if (memsizes_count >= memsizes_cap) {
		memsizes_cap += 64;
		memsizes = realloc(memsizes, memsizes_cap * sizeof(scl_any));
	}
	memsizes[index] = size;
}

// Returns true if the pointer was allocated using _scl_alloc()
scl_int _scl_check_allocated(scl_any ptr) {
	return _scl_get_index_of_ptr(ptr) != -1;
}

// Allocates size bytes of memory
// the memory will always have a size of a multiple of sizeof(scl_int)
scl_any _scl_alloc(size_t size) {

	// Make size be next biggest multiple of sizeof(scl_int)
	if (size % sizeof(scl_any) != 0) {
		size += size % sizeof(scl_any);
	}

	// Allocate the memory
	scl_any ptr = malloc(size);

	// Hard-throw if memory allocation failed
	if (!ptr) {
		_scl_security_throw(EX_BAD_PTR, "malloc() failed!");
		return NULL;
	}

	// Add the pointer to the table
	_scl_add_ptr(ptr, size);
	return ptr;
}

// Reallocates a pointer to a new size
// the memory will always have a size of a multiple of sizeof(scl_int)
scl_any _scl_realloc(scl_any ptr, size_t size) {

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
	ptr = realloc(ptr, size);

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
		free(ptr);
	}
}


// Assert, that 'b' is true
void _scl_assert(scl_int b, scl_int8* msg) {
	if (!b) {
		printf("\n");
		if (_callstack.data[_callstack.ptr - 1].file) {
			printf("%s:" SCL_INT_FMT ":" SCL_INT_FMT ": ", _callstack.data[_callstack.ptr - 1].file, _callstack.data[_callstack.ptr - 1].line, _callstack.data[_callstack.ptr - 1].col);
		}
		printf("Assertion failed: %s\n", msg);
		print_stacktrace();

		_scl_security_safe_exit(EX_ASSERTION_FAIL);
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
	
	char* str = malloc(strlen(msg) * 8);
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

// Collect left-over garbage after a function is finished
void _scl_cleanup_post_func(scl_int depth) {
	_stack.ptr = _callstack.data[depth].begin_stack_size;
}

scl_str _scl_create_string(scl_int8* data) {
	scl_str self = _scl_alloc_struct(sizeof(struct _scl_string), "str", hash1("SclObject"));
	if (self == NULL) {
		_scl_security_throw(EX_BAD_PTR, "Failed to allocate memory for cstr '%s'\n", data);
		return NULL;
	}
	self->_data = strdup(data);
	self->_len = strlen(self->_data);
	if (_scl_find_index_of_struct(self) == -1) {
		_scl_security_throw(EX_BAD_PTR, "Could not create string instance for cstr '%s'. Index is: " SCL_INT_FMT "\n", data, _scl_find_index_of_struct(self));
		return NULL;
	}
	return self;
}

static int printingStacktrace = 0;

void print_stacktrace() {
	printingStacktrace = 1;

#if !defined(_WIN32) && !defined(__wasm__)
	if (_callstack.ptr)
#endif
	printf("Stacktrace:\n");
	if (_callstack.ptr == 0) {
#if !defined(_WIN32) && !defined(__wasm__)
		printf("Native trace:\n");

		scl_any array[64];
		char** strings;
		int size, i;

		size = backtrace(array, 64);
		strings = backtrace_symbols(array, size);
		if (strings != NULL) {
			for (i = 1; i < size; i++)
				printf("  %s\n", strings[i]);
		}

		free(strings);
#else
		printf("  <empty>\n");
#endif
		return;
	}

	for (signed long i = _callstack.ptr - 1; i >= 0; i--) {
		if (_callstack.data[i].file) {
			char* f = strrchr(_callstack.data[i].file, '/');
			if (!f) f = _callstack.data[i].file;
			else f++;
			if (_callstack.data[i].line) {
				printf("  %s -> %s:" SCL_INT_FMT ":" SCL_INT_FMT "\n", (scl_int8*) _callstack.data[i].func, f, _callstack.data[i].line, _callstack.data[i].col);
			} else {
				printf("  %s -> %s\n", (scl_int8*) _callstack.data[i].func, f);
			}
		} else {
			if (_callstack.data[i].line) {
				printf("  %s:" SCL_INT_FMT ":" SCL_INT_FMT "\n", (scl_int8*) _callstack.data[i].func, _callstack.data[i].line, _callstack.data[i].col);
			} else {
				printf("  %s\n", (scl_int8*) _callstack.data[i].func);
			}
		}

	}
	printingStacktrace = 0;
	printf("\n");
}


void print_stacktrace_with_file(FILE* trace) {
	printingStacktrace = 1;

#if !defined(_WIN32) && !defined(__wasm__)
	if (_callstack.ptr)
#endif
	{
		printf("Stacktrace:\n");
		fprintf(trace, "Stacktrace:\n");
	}
#if defined(SCL_DEBUG) && !defined(_WIN32) && !defined(__wasm__)
	if (1) {
#else
	if (_callstack.ptr == 0) {
#endif
#if !defined(_WIN32) && !defined(__wasm__)
		printf("Native trace:\n");
		fprintf(trace, "Native trace:\n");

		scl_any array[64];
		char** strings;
		int size, i;

		size = backtrace(array, 64);
		strings = backtrace_symbols(array, size);
		if (strings != NULL) {
			for (i = 1; i < size; i++) {
				printf("  %s\n", strings[i]);
				fprintf(trace, "  %s\n", strings[i]);
			}
		}

		free(strings);
#else
		printf("  <empty>\n");
		fprintf(trace, "  <empty>\n");
#endif
		return;
	}

	for (signed long i = _callstack.ptr - 1; i >= 0; i--) {
		if (_callstack.data[i].file) {
			char* f = strrchr(_callstack.data[i].file, '/');
			if (!f) f = _callstack.data[i].file;
			else f++;
			if (_callstack.data[i].line) {
				printf("  %s -> %s:" SCL_INT_FMT ":" SCL_INT_FMT "\n", (scl_int8*) _callstack.data[i].func, f, _callstack.data[i].line, _callstack.data[i].col);
				fprintf(trace, "  %s -> %s:" SCL_INT_FMT ":" SCL_INT_FMT "\n", (scl_int8*) _callstack.data[i].func, f, _callstack.data[i].line, _callstack.data[i].col);
			} else {
				printf("  %s -> %s\n", (scl_int8*) _callstack.data[i].func, f);
				fprintf(trace, "  %s -> %s\n", (scl_int8*) _callstack.data[i].func, f);
			}
		} else {
			if (_callstack.data[i].line) {
				printf("  %s -> (nil):" SCL_INT_FMT ":" SCL_INT_FMT "\n", (scl_int8*) _callstack.data[i].func, _callstack.data[i].line, _callstack.data[i].col);
				fprintf(trace, "  %s -> (nil):" SCL_INT_FMT ":" SCL_INT_FMT "\n", (scl_int8*) _callstack.data[i].func, _callstack.data[i].line, _callstack.data[i].col);
			} else {
				printf("  %s -> (nil)\n", (scl_int8*) _callstack.data[i].func);
				fprintf(trace, "  %s -> (nil)\n", (scl_int8*) _callstack.data[i].func);
			}
		}
	}

	printingStacktrace = 0;
	printf("\n");
	fprintf(trace, "\n");
}

// final signal handler
// if we get here, something has gone VERY wrong
void _scl_catch_final(int sig_num) {
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

	if (_callstack.ptr == 0) {
		printf("<native code>: Exception: %s\n", signalString);
		fprintf(trace, "<native code>: Exception: %s\n", signalString);
	} else {
		printf("%s:" SCL_INT_FMT ":" SCL_INT_FMT ": Exception: %s\n", _callstack.data[_callstack.ptr - 1].file, _callstack.data[_callstack.ptr - 1].line, _callstack.data[_callstack.ptr - 1].col, signalString);
		fprintf(trace, "%s:" SCL_INT_FMT ":" SCL_INT_FMT ": Exception: %s\n", _callstack.data[_callstack.ptr - 1].file, _callstack.data[_callstack.ptr - 1].line, _callstack.data[_callstack.ptr - 1].col, signalString);
	}
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
	for (scl_int i = 0; i < allocated_count; i++) {
		if (_scl_find_index_of_struct(allocated[i]) != -1) {
			fprintf(trace, "  Instance of struct '%s':\n", ((Struct*) allocated[i])->type_name);
		} else {
			fprintf(trace, "  %zu bytes at %p:\n", memsizes[i], allocated[i]);
		}
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
		fprintf(trace, "\n");
	}

	fclose(trace);
	exit(sig_num);
}

struct scl_Array {
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

// Converts C NULL-terminated array to Array-instance
scl_any _scl_c_arr_to_scl_array(scl_any arr[]) {
	struct scl_Array* array = _scl_alloc_struct(sizeof(struct scl_Array), "Array", hash1("SclObject"));
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

inline ssize_t ctrl_stack_size(void) {
	return _stack.ptr;
}

void _scl_sleep(scl_int millis) {
	sleep(millis);
}

const hash hash1(const char* data) {
	hash h = 7;
	for (int i = 0; i < strlen(data); i++) {
		h = h * 31 + data[i];
	}
	return h;
}

struct _scl_methodinfo {
  scl_any  ptr;
  scl_int  pure_name;
};

struct _scl_typeinfo {
  scl_int					type;
  scl_int					super;
  scl_int					methodscount;
  struct _scl_methodinfo**	methods;
};

// Structs
extern struct _scl_typeinfo		_scl_types[];
extern size_t 					_scl_types_count;

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

// adds an instance to the table of tracked instances
scl_any _scl_add_struct(scl_any ptr) {
	insert_sorted((scl_any**) &instances, &instances_count, ptr, &instances_cap);
	return ptr;
}

// creates a new instance with a size of 'size'
scl_any _scl_alloc_struct(size_t size, scl_int8* type_name, hash super) {

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
static void _scl_struct_map_remove(size_t index) {
	for (size_t i = index; i < instances_count - 1; i++) {
	   instances[i] = instances[i + 1];
	}
	instances_count--;
}

// Returns the next index of an instance in the allocated table
// Returns -1, if not in table
size_t _scl_find_index_of_struct(scl_any ptr) {
	if (ptr == NULL) return -1;
	return _scl_binary_search((void**) instances, instances_count, ptr);
}

// Removes an instance from the allocated table without calling its finalizer
void _scl_free_struct_no_finalize(scl_any ptr) {
	size_t i = _scl_find_index_of_struct(ptr);
	while (i != -1) {
		_scl_struct_map_remove(i);
		i = _scl_find_index_of_struct(ptr);
	}
}

// Removes an instance from the allocated table and call the finalzer
void _scl_free_struct(scl_any ptr) {
	if (_scl_find_index_of_struct(ptr) == -1)
		return;

	size_t i = _scl_find_index_of_struct(ptr);
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
scl_int _scl_struct_is_type(scl_any ptr, hash typeId) {
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

void _scl_set_up_signal_handler() {
#if defined(SIGHUP)
	signal(SIGHUP, _scl_catch_final);
#endif
#if defined(SIGINT)
	signal(SIGINT, _scl_catch_final);
#endif
#if defined(SIGQUIT)
	signal(SIGQUIT, _scl_catch_final);
#endif
#if defined(SIGILL)
	signal(SIGILL, _scl_catch_final);
#endif
#if defined(SIGTRAP)
	signal(SIGTRAP, _scl_catch_final);
#endif
#if defined(SIGABRT)
	signal(SIGABRT, _scl_catch_final);
#endif
#if defined(SIGEMT)
	signal(SIGEMT, _scl_catch_final);
#endif
#if defined(SIGFPE)
	signal(SIGFPE, _scl_catch_final);
#endif
#if defined(SIGBUS)
	signal(SIGBUS, _scl_catch_final);
#endif
#if defined(SIGSEGV)
	signal(SIGSEGV, _scl_catch_final);
#endif
#if defined(SIGSYS)
	signal(SIGSYS, _scl_catch_final);
#endif
}

_scl_frame_t* _scl_push() {
	_stack.ptr++;

	if (_stack.ptr >= _stack.cap) {
		_stack.cap += 64;
		_scl_frame_t* tmp = realloc(_stack.data, sizeof(_scl_frame_t) * _stack.cap);
		if (!tmp) {
			_scl_security_throw(EX_BAD_PTR, "realloc() failed");
		} else {
			_stack.data = tmp;
		}
	}

	return &(_stack.data[_stack.ptr - 1]);
}

_scl_frame_t* _scl_pop() {
#if !defined(SCL_FEATURE_UNSAFE_STACK_ACCESSES)
	if (_stack.ptr <= 0) {
		_scl_security_throw(EX_STACK_UNDERFLOW, "Not enough data on the stack!");
	}
#endif

	return &(_stack.data[--_stack.ptr]);
}

_scl_frame_t* _scl_top() {
#if !defined(SCL_FEATURE_UNSAFE_STACK_ACCESSES)
	if (_stack.ptr <= 0) {
		_scl_security_throw(EX_STACK_UNDERFLOW, "Not enough data on the stack!");
	}
#endif

	return &_stack.data[_stack.ptr - 1];
}

void _scl_popn(scl_int n) {
	_stack.ptr -= n;

#if !defined(SCL_FEATURE_UNSAFE_STACK_ACCESSES)
	if (_stack.ptr < 0) {
		_scl_security_throw(EX_STACK_UNDERFLOW, "Not enough data on the stack!");
	}
#endif
}

void _scl_exception_push() {
	_extable.jmp_buf_ptr++;
	if (_extable.jmp_buf_ptr >= _extable.capacity) {
		_extable.capacity += 64;
		scl_any* tmp;

		tmp = realloc(_extable.cs_pointer, _extable.capacity * sizeof(scl_int));
		if (!tmp) _scl_security_throw(EX_BAD_PTR, "realloc() failed");
		_extable.cs_pointer = (scl_int*) tmp;

		tmp = realloc(_extable.exceptions, _extable.capacity * sizeof(scl_any));
		if (!tmp) _scl_security_throw(EX_BAD_PTR, "realloc() failed");
		_extable.exceptions = tmp;

		tmp = realloc(_extable.jmp_buf, _extable.capacity * sizeof(jmp_buf));
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
		scl_int8* msg = (scl_int8*) malloc(sizeof(scl_int8) * strlen(name) + 64);
		snprintf(msg, 64 + strlen(name), "Argument %s is nil", name);
		_scl_assert(0, msg);
	}
}

void _scl_not_nil_cast(scl_int val, scl_int8* name) {
	if (val == 0) {
		scl_int8* msg = (scl_int8*) malloc(sizeof(scl_int8) * strlen(name) + 64);
		snprintf(msg, 64 + strlen(name), "Nil cannot be cast to non-nil type '%s'!", name);
		_scl_assert(0, msg);
	}
}

void _scl_struct_allocation_failure(scl_int val, scl_int8* name) {
	if (val == 0) {
		scl_int8* msg = (scl_int8*) malloc(sizeof(scl_int8) * strlen(name) + 64);
		snprintf(msg, 64 + strlen(name), "Failed to allocate memory for struct '%s'!", name);
		_scl_assert(0, msg);
	}
}

void _scl_nil_ptr_dereference(scl_int val, scl_int8* name) {
	if (val == 0) {
		scl_int8* msg = (scl_int8*) malloc(sizeof(scl_int8) * strlen(name) + 64);
		snprintf(msg, 64 + strlen(name), "Tried dereferencing nil pointer '%s'!", name);
		_scl_assert(0, msg);
	}
}

void _scl_check_not_nil_store(scl_int val, scl_int8* name) {
	if (val == 0) {
		scl_int8* msg = (scl_int8*) malloc(sizeof(scl_int8) * strlen(name) + 64);
		snprintf(msg, 64 + strlen(name), "Nil cannot be stored in non-nil variable '%s'!", name);
		_scl_assert(0, msg);
	}
}

void _scl_not_nil_return(scl_int val, scl_int8* name) {
	if (val == 0) {
		scl_int8* msg = (scl_int8*) malloc(sizeof(scl_int8) * strlen(name) + 64);
		snprintf(msg, 64 + strlen(name), "Tried returning nil from function returning not-nil type '%s'!", name);
		_scl_assert(0, msg);
	}
}

#if !defined(SCL_DEFAULT_STACK_FRAME_COUNT)
#define SCL_DEFAULT_STACK_FRAME_COUNT 16
#endif

void _scl_create_stack() {
	// These use C's malloc, keep it that way
	// They should NOT be affected by any future
	// stuff we might do with _scl_alloc()
	_stack.ptr = 0;
	_stack.cap = SCL_DEFAULT_STACK_FRAME_COUNT;
	_stack.data = malloc(sizeof(_scl_frame_t) * _stack.cap);

	allocated = malloc(allocated_cap * sizeof(scl_any));
	memsizes = malloc(memsizes_cap * sizeof(size_t));
	instances = malloc(instances_cap * sizeof(Struct*));

	_extable.cs_pointer = 0;
	_extable.jmp_buf_ptr = 0;
	_extable.capacity = SCL_DEFAULT_STACK_FRAME_COUNT;
	_extable.cs_pointer = malloc(_extable.capacity * sizeof(scl_int));
	_extable.exceptions = malloc(_extable.capacity * sizeof(scl_any));
	_extable.jmp_buf = malloc(_extable.capacity * sizeof(jmp_buf));
}

// Returns a function pointer with the following signature:
// function main(args: Array, env: Array): int
scl_any _scl_get_main_addr();
typedef int(*mainFunc)(struct scl_Array*, struct scl_Array*);

// __init__ and __destroy__
typedef void(*genericFunc)();

// __init__
// last element is always NULL
extern genericFunc _scl_internal_init_functions[];

// __destroy__
// last element is always NULL
extern genericFunc _scl_internal_destroy_functions[];

#if !defined(SCL_COMPILER_NO_MAIN)
const char __SCL_LICENSE[] = "MIT License\n\nCopyright (c) 2023 StonkDragon\n\n";

#if defined(static_assert)
static_assert(sizeof(scl_int) == sizeof(scl_any), "Size of scl_int and scl_any do not match!");
static_assert(sizeof(scl_int) == sizeof(scl_float), "Size of scl_int and scl_float do not match!");
static_assert(sizeof(scl_any) == sizeof(scl_float), "Size of scl_any and scl_float do not match!");
#endif

_scl_no_return void _scl_native_main(int argc, char** argv) __asm(_scl_macro_to_string(__USER_LABEL_PREFIX__) "main");
_scl_no_return void _scl_native_main(int argc, char** argv) {
	#if !defined(static_assert)
	assert(sizeof(scl_int) == sizeof(scl_any) && "Size of scl_int and scl_any do not match!");
	assert(sizeof(scl_int) == sizeof(scl_float) && "Size of scl_int and scl_float do not match!");
	assert(sizeof(scl_any) == sizeof(scl_float) && "Size of scl_any and scl_float do not match!");
	#endif

	// Endian-ness detection
	short word = 0x0001;
	char *byte = (char*) &word;
	_scl_assert(byte[0], "Invalid byte order detected!");

	char* scl_print_licence;
	if ((scl_print_licence = getenv("SCL_PRINT_LICENSE_AND_EXIT"))) {
		printf("Scale License:\n%s", __SCL_LICENSE);
		exit(0);
	}

	// Register signal handler for all available signals
	_scl_set_up_signal_handler();

	_callstack.data[0].func = (scl_int8*) __FUNCTION__;

	_scl_create_stack();

	// Convert argv and envp from native arrays to Scale arrays
	struct scl_Array* args = (struct scl_Array*) _scl_c_arr_to_scl_array((scl_any*) argv);
	struct scl_Array* env = (struct scl_Array*) _scl_c_arr_to_scl_array((scl_any*) _scl_platform_get_env());

	// Get the address of the main function
	mainFunc _scl_main = (mainFunc) _scl_get_main_addr();
#else

// Initialize as library
_scl_constructor void _scl_load() {
	#if !defined(static_assert)
	assert(sizeof(scl_int) == sizeof(scl_any) && "Size of scl_int and scl_any do not match!");
	assert(sizeof(scl_int) == sizeof(scl_float) && "Size of scl_int and scl_float do not match!");
	assert(sizeof(scl_any) == sizeof(scl_float) && "Size of scl_any and scl_float do not match!");
	#endif

	// Endian-ness detection
	short word = 0x0001;
	char *byte = (char*) &word;
	_scl_assert(byte[0], "Invalid byte order detected!");

	_scl_create_stack();

#endif

	// Run __init__ functions
	for (int i = 0; _scl_internal_init_functions[i]; i++) {
		_scl_internal_init_functions[i]();
	}

#if !defined(SCL_COMPILER_NO_MAIN)

	int ret;
	int jmp = setjmp(_extable.jmp_buf[_extable.jmp_buf_ptr]);
	_extable.jmp_buf_ptr++;
	if (jmp != 666) {

		// _scl_get_main_addr() returns NULL if compiled with --no-main
#if defined(SCL_MAIN_RETURN_NONE)
		ret = 0;
		(_scl_main ? _scl_main(args, env) : 0);
#else
		ret = (_scl_main ? _scl_main(args, env) : 0);
#endif
	} else {

		// Scale exception reached here
		_scl_security_throw(EX_THROWN, "Uncaught exception");
	}

#else
}

// Uninitialize library
_scl_destructor void _scl_destroy() {
#endif

	// Run finalization:
	// Run __destroy__ functions
	for (int i = 0; _scl_internal_destroy_functions[i]; i++) {
		_scl_internal_destroy_functions[i]();
	}

#if !defined(SCL_COMPILER_NO_MAIN)
	// We don't return
	exit(ret);
#endif
}
