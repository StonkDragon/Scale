#include "scale_internal.h"

#ifdef __cplusplus
#error C++ is not supported by Scale
#endif

// Variables

// The Stack
__thread _scl_stack_t 		_scl_internal_stack _scl_align;

// Scale Callstack
__thread _scl_callstack_t	_scl_internal_callstack _scl_align;

// this is used by try-catch
__thread struct _exception_handling {
	scl_any*	_scl_exception_table;	// The exception
	jmp_buf*	_scl_jmp_buf;			// jump buffer used by longjmp()
	scl_int		_scl_jmp_buf_ptr;		// number specifying the current depth
	scl_int 	_cap;					// capacity of lists
	scl_int*	_scl_cs_ptr;			// callstack-pointer of this catch level
} _scl_internal_exceptions _scl_align = {0};

#define unimplemented do { fprintf(stderr, "%s:%d: %s: Not Implemented\n", __FILE__, __LINE__, __FUNCTION__); exit(1) } while (0)

static scl_any*	alloced_ptrs;				// List of allocated pointers
static scl_int	alloced_ptrs_count = 0;		// Length of the list
static scl_int	alloced_ptrs_cap = 64;		// Capacity of the list

static size_t*	ptrs_size;					// List of pointer sizes
static scl_int	ptrs_size_count = 0;		// Length of the list
static scl_int	ptrs_size_cap = 64;			// Capacity of the list

// Inserts value into array at it's sorted position
// returns the index at which the value was inserted
// will not insert value, if it is already present in the array
scl_int _scl_sorted_insert(scl_any** array, scl_int* size, scl_any value, scl_int* cap) {
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
		(*cap) *= 2;
		(*array) = realloc((*array), *(cap) * sizeof((*array)[0]));
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
		ptrs_size[index] = 0;
	}
}

// Returns the next index of a given pointer in the allocated-pointers array
// Returns -1, if the pointer is not in the table
scl_int _scl_get_index_of_ptr(scl_any ptr) {
	return _scl_binary_search(alloced_ptrs, alloced_ptrs_count, ptr);
}

// Removes the pointer at the given index and shift everything after left
void _scl_remove_ptr_at_index(scl_int index) {
	for (scl_int i = index; i < alloced_ptrs_count - 1; i++) {
    	alloced_ptrs[i] = alloced_ptrs[i + 1];
    }
    alloced_ptrs_count--;
}

// Adds a new pointer to the table
void _scl_add_ptr(scl_any ptr, size_t size) {
	scl_int index = _scl_sorted_insert((scl_any**) &alloced_ptrs, &alloced_ptrs_count, ptr, &alloced_ptrs_cap);
	ptrs_size_count++;
	if (index >= ptrs_size_cap) {
		ptrs_size_cap = index;
		ptrs_size = realloc(ptrs_size, ptrs_size_cap * sizeof(scl_any));
	} else if (ptrs_size_count >= ptrs_size_cap) {
		ptrs_size_cap *= 2;
		ptrs_size = realloc(ptrs_size, ptrs_size_cap * sizeof(scl_any));
	}
	ptrs_size[index] = size;
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
		printf("%s:" SCL_INT_FMT ":" SCL_INT_FMT ": ", _scl_internal_callstack.data[_scl_internal_callstack.ptr - 1].file, _scl_internal_callstack.data[_scl_internal_callstack.ptr - 1].line, _scl_internal_callstack.data[_scl_internal_callstack.ptr - 1].col);
		printf("Assertion failed: %s\n", msg);
		print_stacktrace();

		_scl_security_safe_exit(EX_ASSERTION_FAIL);
	}
}

// Hard-throw an exception
_scl_no_return void _scl_security_throw(int code, scl_int8* msg) {
	remove("scl_trace.log");
	FILE* trace = fopen("scl_trace.log", "a");
	printf("\n");
	fprintf(trace, "\n");
	printf("Exception: %s\n", msg);
	fprintf(trace, "Exception: %s\n", msg);
	if (errno) {
		printf("errno: %s\n", strerror(errno));
		fprintf(trace, "errno: %s\n", strerror(errno));
	}
	print_stacktrace_with_file(trace);

	_scl_security_safe_exit(code);
}

// exit()
_scl_no_return void _scl_security_safe_exit(int code) {

	// Call finalizers on instances
	_scl_finalize();

	// exit
	exit(code);
}

// Collect left-over garbage after a function is finished
void _scl_cleanup_post_func(scl_int depth) {
	_scl_internal_stack.ptr = _scl_internal_callstack.data[depth].begin_stack_size;
}

scl_str _scl_create_string(scl_int8* data) {
	scl_str instance = _scl_alloc_struct(sizeof(struct _scl_string), "str", hash1("SclObject"));
	instance->_data = data;
	instance->_len = strlen(data);
	return instance;
}

static int printingStacktrace = 0;

void print_stacktrace() {
	printingStacktrace = 1;

#if !defined(_WIN32) && !defined(__wasm__)
	if (_scl_internal_callstack.ptr)
#endif
	printf("Stacktrace:\n");
	if (_scl_internal_callstack.ptr == 0) {
#if !defined(_WIN32) && !defined(__wasm__)
		printf("Native trace:\n");

        void* array[64];
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

	for (signed long i = _scl_internal_callstack.ptr - 1; i >= 0; i--) {
		if (_scl_internal_callstack.data[i].file) {
			char* f = strrchr(_scl_internal_callstack.data[i].file, '/');
			if (!f) f = _scl_internal_callstack.data[i].file;
			else f++;
			if (_scl_internal_callstack.data[i].line) {
				printf("  %s -> %s:" SCL_INT_FMT ":" SCL_INT_FMT "\n", (scl_int8*) _scl_internal_callstack.data[i].func, f, _scl_internal_callstack.data[i].line, _scl_internal_callstack.data[i].col);
			} else {
				printf("  %s -> %s\n", (scl_int8*) _scl_internal_callstack.data[i].func, f);
			}
		} else {
			if (_scl_internal_callstack.data[i].line) {
				printf("  %s -> (nil):" SCL_INT_FMT ":" SCL_INT_FMT "\n", (scl_int8*) _scl_internal_callstack.data[i].func, _scl_internal_callstack.data[i].line, _scl_internal_callstack.data[i].col);
			} else {
				printf("  %s -> (nil)\n", (scl_int8*) _scl_internal_callstack.data[i].func);
			}
		}

	}
	printingStacktrace = 0;
	printf("\n");
}


void print_stacktrace_with_file(FILE* trace) {
	printingStacktrace = 1;

#if !defined(_WIN32) && !defined(__wasm__)
	if (_scl_internal_callstack.ptr)
#endif
	{
		printf("Stacktrace:\n");
		fprintf(trace, "Stacktrace:\n");
	}
#if defined(SCL_DEBUG) && !defined(_WIN32) && !defined(__wasm__)
	if (true) {
#else
	if (_scl_internal_callstack.ptr == 0) {
#endif
#if !defined(_WIN32) && !defined(__wasm__)
		printf("Native trace:\n");
		fprintf(trace, "Native trace:\n");

        void* array[64];
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

	for (signed long i = _scl_internal_callstack.ptr - 1; i >= 0; i--) {
		if (_scl_internal_callstack.data[i].file) {
			char* f = strrchr(_scl_internal_callstack.data[i].file, '/');
			if (!f) f = _scl_internal_callstack.data[i].file;
			else f++;
			if (_scl_internal_callstack.data[i].line) {
				printf("  %s -> %s:" SCL_INT_FMT ":" SCL_INT_FMT "\n", (scl_int8*) _scl_internal_callstack.data[i].func, f, _scl_internal_callstack.data[i].line, _scl_internal_callstack.data[i].col);
				fprintf(trace, "  %s -> %s:" SCL_INT_FMT ":" SCL_INT_FMT "\n", (scl_int8*) _scl_internal_callstack.data[i].func, f, _scl_internal_callstack.data[i].line, _scl_internal_callstack.data[i].col);
			} else {
				printf("  %s -> %s\n", (scl_int8*) _scl_internal_callstack.data[i].func, f);
				fprintf(trace, "  %s -> %s\n", (scl_int8*) _scl_internal_callstack.data[i].func, f);
			}
		} else {
			if (_scl_internal_callstack.data[i].line) {
				printf("  %s -> (nil):" SCL_INT_FMT ":" SCL_INT_FMT "\n", (scl_int8*) _scl_internal_callstack.data[i].func, _scl_internal_callstack.data[i].line, _scl_internal_callstack.data[i].col);
				fprintf(trace, "  %s -> (nil):" SCL_INT_FMT ":" SCL_INT_FMT "\n", (scl_int8*) _scl_internal_callstack.data[i].func, _scl_internal_callstack.data[i].line, _scl_internal_callstack.data[i].col);
			} else {
				printf("  %s -> (nil)\n", (scl_int8*) _scl_internal_callstack.data[i].func);
				fprintf(trace, "  %s -> (nil)\n", (scl_int8*) _scl_internal_callstack.data[i].func);
			}
		}
	}

	printingStacktrace = 0;
	printf("\n");
	fprintf(trace, "\n");
}

// generic struct
struct sclstruct {
	scl_int	type;
	scl_int8*	type_name;
	scl_int	super;
	scl_int	size;
	scl_int	count;
};

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

	if (_scl_internal_callstack.ptr == 0) {
		printf("<native code>: Exception: %s\n", signalString);
		fprintf(trace, "<native code>: Exception: %s\n", signalString);
	} else {
		printf("%s:" SCL_INT_FMT ":" SCL_INT_FMT ": Exception: %s\n", _scl_internal_callstack.data[_scl_internal_callstack.ptr - 1].file, _scl_internal_callstack.data[_scl_internal_callstack.ptr - 1].line, _scl_internal_callstack.data[_scl_internal_callstack.ptr - 1].col, signalString);
		fprintf(trace, "%s:" SCL_INT_FMT ":" SCL_INT_FMT ": Exception: %s\n", _scl_internal_callstack.data[_scl_internal_callstack.ptr - 1].file, _scl_internal_callstack.data[_scl_internal_callstack.ptr - 1].line, _scl_internal_callstack.data[_scl_internal_callstack.ptr - 1].col, signalString);
	}
	if (errno) {
		printf("errno: %s\n", strerror(errno));
		fprintf(trace, "errno: %s\n", strerror(errno));
	}
	if (!printingStacktrace)
		print_stacktrace_with_file(trace);
	printf("Stack address: %p\n", _scl_internal_stack.data);
	fprintf(trace, "Stack address: %p\n", _scl_internal_stack.data);
	if (_scl_internal_stack.ptr && _scl_internal_stack.ptr < _scl_internal_stack.cap) {
		printf("Stack:\n");
		fprintf(trace, "Stack:\n");
		printf("SP: " SCL_INT_FMT "\n", _scl_internal_stack.ptr);
		fprintf(trace, "SP: " SCL_INT_FMT "\n", _scl_internal_stack.ptr);
		for (scl_int i = _scl_internal_stack.ptr - 1; i >= 0; i--) {
			scl_int v = _scl_internal_stack.data[i].i;
			printf("   " SCL_INT_FMT ": 0x" SCL_INT_HEX_FMT ", " SCL_INT_FMT "\n", i, v, v);
			fprintf(trace, "   " SCL_INT_FMT ": 0x" SCL_INT_HEX_FMT ", " SCL_INT_FMT "\n", i, v, v);
		}
		printf("\n");
		fprintf(trace, "\n");
	}
	fprintf(trace, "Memory Dump:\n");
	for (scl_int i = 0; i < alloced_ptrs_count; i++) {
		if (_scl_find_index_of_struct(alloced_ptrs[i]) != -1) {
			fprintf(trace, "  Instance of struct '%s':\n", ((struct sclstruct*) alloced_ptrs[i])->type_name);
		} else {
			fprintf(trace, "  %zu bytes at %p:\n", ptrs_size[i], alloced_ptrs[i]);
		}
		fprintf(trace, "    |");
		for (scl_int col = 0; col < (ptrs_size[i] < 16 ? ptrs_size[i] : 16); col++) {
			fprintf(trace, "%02x|", (char) ((col & 0xFF) + (((scl_int) alloced_ptrs[i]) & 0xF)));
		}
		fprintf(trace, "\n");
		fprintf(trace, "    |");
		for (scl_int col = 0; col < (ptrs_size[i] < 16 ? ptrs_size[i] : 16); col++) {
			fprintf(trace, "--|");
		}
		fprintf(trace, "\n");
		for (scl_int sz = 0; sz < ptrs_size[i]; sz += 16) {
			fprintf(trace, "    |");
			for (scl_int col = 0; col < (ptrs_size[i] < 16 ? ptrs_size[i] : 16); col++) {
				fprintf(trace, "%02x|", (*(char*) (alloced_ptrs[i] + sz + col) & 0xFF));
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
	array->count = 0;
	array->values = _scl_alloc(cap * sizeof(scl_int8*));
	for (scl_int i = 0; i < cap; i++) {
		((scl_any*) array->values)[(scl_int) array->count++] = _scl_create_string(arr[i]);
	}
	return array;
}

inline _scl_frame_t* ctrl_push_frame() {
	return &_scl_internal_stack.data[_scl_internal_stack.ptr++];
}

inline _scl_frame_t* ctrl_pop_frame() {
	return &_scl_internal_stack.data[--_scl_internal_stack.ptr];
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
	return _scl_internal_stack.ptr;
}


hash hash1(char* data) {
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
extern struct _scl_typeinfo		_scl_internal_types[];
extern size_t 					_scl_internal_types_count;

// table of instances
static struct sclstruct** allocated_structs;
static scl_int allocated_structs_count = 0;
static scl_int allocated_structs_cap = 64;

// table of instances allocated with _scl_alloc_struct()
static struct sclstruct** mallocced_structs;
static scl_int mallocced_structs_count = 0;
static scl_int mallocced_structs_cap = 64;

// Free all allocated instances
void _scl_finalize() {
	for (size_t i = 0; i < mallocced_structs_count; i++) {
		if (mallocced_structs[i]) {
			_scl_free(mallocced_structs[i]);
		}
		mallocced_structs[i] = 0;
	}
}

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
void* _scl_typeinfo_of(hash type) {
	scl_int index = _scl_binary_search_typeinfo_index(_scl_internal_types, _scl_internal_types_count, type);
	if (index >= 0) {
		return (void*) _scl_internal_types + (index * sizeof(struct _scl_typeinfo));
	}
	return NULL;
}

struct _scl_typeinfo* _scl_find_typeinfo_of(hash type) {
	return (struct _scl_typeinfo*) _scl_typeinfo_of(type);
}

// returns the method handle of a method on a struct, or a parent struct
void* _scl_get_method_on_type(hash type, hash method) {
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
	_scl_sorted_insert((scl_any**) &allocated_structs, &allocated_structs_count, ptr, &allocated_structs_cap);
	return ptr;
}

// SclObject:finalize()
void Method_SclObject_finalize(scl_any) __asm("m.SclObject.f.finalize");

// creates a new instance with a size of 'size'
scl_any _scl_alloc_struct(size_t size, scl_int8* type_name, hash super) {

	// Allocate the memory
	scl_any ptr = _scl_alloc(size);

	// Type name hash
	((struct sclstruct*) ptr)->type = hash1(type_name);

	// Type name
    ((struct sclstruct*) ptr)->type_name = type_name;

	// Parent struct name hash
    ((struct sclstruct*) ptr)->super = super;

	// Size (Currently only used by SclObject:clone())
    ((struct sclstruct*) ptr)->size = size;

	// Reference count
    ((struct sclstruct*) ptr)->count = 1;

	// Add struct to allocated table
	scl_int index = _scl_sorted_insert((scl_any**) &mallocced_structs, &mallocced_structs_count, ptr, &mallocced_structs_cap);
	allocated_structs_count++;
	return allocated_structs[index] = ptr;
}

// Removes an instance from the allocated table by index
static void _scl_struct_map_remove(size_t index) {
    for (size_t i = index; i < allocated_structs_count - 1; i++) {
       allocated_structs[i] = allocated_structs[i + 1];
    }
    allocated_structs_count--;
}

// Returns the next index of an instance in the allocated table
// Returns -1, if not in table
size_t _scl_find_index_of_struct(scl_any ptr) {
	if (ptr == NULL) return -1;
	return _scl_binary_search((void**) allocated_structs, allocated_structs_count, ptr);
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

	scl_any method = _scl_get_method_on_type(((struct sclstruct*) ptr)->type, hash1("finalize"));
	if (method) {
		_scl_push()->v = ptr;
		((void(*)()) method)();
	}
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
	int isStruct = _scl_binary_search((scl_any*) allocated_structs, allocated_structs_count, ptr) != -1;
	if (!isStruct) return 0;

	struct sclstruct* ptrStruct = (struct sclstruct*) ptr;

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
#if defined(SIGPOLL)
	signal(SIGPOLL, _scl_catch_final);
#endif
#if defined(SIGIOT)
	signal(SIGIOT, _scl_catch_final);
#endif
#if defined(SIGEMT)
	signal(SIGEMT, _scl_catch_final);
#endif
#if defined(SIGFPE)
	signal(SIGFPE, _scl_catch_final);
#endif
#if defined(SIGKILL)
	signal(SIGKILL, _scl_catch_final);
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
#if defined(SIGPIPE)
	signal(SIGPIPE, _scl_catch_final);
#endif
#if defined(SIGTERM)
	signal(SIGTERM, _scl_catch_final);
#endif
#if defined(SIGSTOP)
	signal(SIGSTOP, _scl_catch_final);
#endif
#if defined(SIGTSTP)
	signal(SIGTSTP, _scl_catch_final);
#endif
#if defined(SIGUSR1)
	signal(SIGUSR1, _scl_catch_final);
#endif
#if defined(SIGUSR2)
	signal(SIGUSR2, _scl_catch_final);
#endif
}

_scl_frame_t* _scl_push() {
	_scl_internal_stack.ptr++;

	if (_scl_internal_stack.ptr >= _scl_internal_stack.cap) {
		_scl_internal_stack.cap *= 2;
		_scl_frame_t* tmp = realloc(_scl_internal_stack.data, sizeof(_scl_frame_t) * _scl_internal_stack.cap);
		if (!tmp) {
			_scl_security_throw(EX_BAD_PTR, "realloc() failed");
		} else {
			_scl_internal_stack.data = tmp;
		}
	}

	_scl_frame_t* res = &(_scl_internal_stack.data[_scl_internal_stack.ptr - 1]);
	return res;
}

_scl_frame_t* _scl_pop() {
#if !defined(SCL_FEATURE_UNSAFE_STACK_ACCESSES)
	if (_scl_internal_stack.ptr <= 0) {
		_scl_security_throw(EX_STACK_UNDERFLOW, "Not enough data on the stack!");
	}
#endif

	_scl_internal_stack.ptr--;
	_scl_frame_t* res = &(_scl_internal_stack.data[_scl_internal_stack.ptr]);
	return res;
}

_scl_frame_t* _scl_top() {
#if !defined(SCL_FEATURE_UNSAFE_STACK_ACCESSES)
	if (_scl_internal_stack.ptr <= 0) {
		_scl_security_throw(EX_STACK_UNDERFLOW, "Not enough data on the stack!");
	}
#endif

	return &_scl_internal_stack.data[_scl_internal_stack.ptr - 1];
}

void _scl_popn(scl_int n) {
	_scl_internal_stack.ptr -= n;

#if !defined(SCL_FEATURE_UNSAFE_STACK_ACCESSES)
	if (_scl_internal_stack.ptr < 0) {
		_scl_security_throw(EX_STACK_UNDERFLOW, "Not enough data on the stack!");
	}
#endif
}

void _scl_exception_push() {
	_scl_internal_exceptions._scl_jmp_buf_ptr++;
	if (_scl_internal_exceptions._scl_jmp_buf_ptr >= _scl_internal_exceptions._cap) {
		_scl_internal_exceptions._cap *= 2;
		scl_any* tmp;

		tmp = realloc(_scl_internal_exceptions._scl_cs_ptr, _scl_internal_exceptions._cap * sizeof(scl_int));
		if (!tmp) _scl_security_throw(EX_BAD_PTR, "realloc() failed");
		_scl_internal_exceptions._scl_cs_ptr = (scl_int*) tmp;

		tmp = realloc(_scl_internal_exceptions._scl_exception_table, _scl_internal_exceptions._cap * sizeof(scl_any));
		if (!tmp) _scl_security_throw(EX_BAD_PTR, "realloc() failed");
		_scl_internal_exceptions._scl_exception_table = tmp;

		tmp = realloc(_scl_internal_exceptions._scl_jmp_buf, _scl_internal_exceptions._cap * sizeof(jmp_buf));
		if (!tmp) _scl_security_throw(EX_BAD_PTR, "realloc() failed");
		_scl_internal_exceptions._scl_jmp_buf = (jmp_buf*) tmp;
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

#if !defined(SCL_DEFAULT_STACK_FRAME_COUNT)
#define SCL_DEFAULT_STACK_FRAME_COUNT 16
#endif

#ifndef SCL_COMPILER_NO_MAIN
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

	_scl_internal_callstack.data[0].file = __FILE__;
	_scl_internal_callstack.data[0].func = (scl_int8*) __FUNCTION__;

	// These use C's malloc, keep it that way
	// They should NOT be affected by any future
	// stuff we might do with _scl_alloc()
	_scl_internal_stack.ptr = 0;
	_scl_internal_stack.cap = SCL_DEFAULT_STACK_FRAME_COUNT;
	_scl_internal_stack.data = malloc(sizeof(_scl_frame_t) * _scl_internal_stack.cap);

	alloced_ptrs = malloc(alloced_ptrs_cap * sizeof(scl_any));
	ptrs_size = malloc(ptrs_size_cap * sizeof(size_t));
	allocated_structs = malloc(allocated_structs_cap * sizeof(struct sclstruct*));
	mallocced_structs = malloc(mallocced_structs_cap * sizeof(struct sclstruct*));

	_scl_internal_exceptions._scl_cs_ptr = 0;
	_scl_internal_exceptions._scl_jmp_buf_ptr = 0;
	_scl_internal_exceptions._cap = SCL_DEFAULT_STACK_FRAME_COUNT;
	_scl_internal_exceptions._scl_cs_ptr = malloc(_scl_internal_exceptions._cap * sizeof(scl_int));
	_scl_internal_exceptions._scl_exception_table = malloc(_scl_internal_exceptions._cap * sizeof(scl_any));
	_scl_internal_exceptions._scl_jmp_buf = malloc(_scl_internal_exceptions._cap * sizeof(jmp_buf));

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

	// These use C's malloc, keep it that way
	// They should NOT be affected by any future
	// stuff we might do with _scl_alloc()
	_scl_internal_stack.ptr = 0;
	_scl_internal_stack.cap = SCL_DEFAULT_STACK_FRAME_COUNT;
	_scl_internal_stack.data = malloc(sizeof(_scl_frame_t) * _scl_internal_stack.cap);

	alloced_ptrs = malloc(alloced_ptrs_cap * sizeof(scl_any));
	ptrs_size = malloc(ptrs_size_cap * sizeof(size_t));
	allocated_structs = malloc(allocated_structs_cap * sizeof(struct sclstruct*));
	mallocced_structs = malloc(mallocced_structs_cap * sizeof(struct sclstruct*));

	_scl_internal_exceptions._scl_cs_ptr = 0;
	_scl_internal_exceptions._scl_jmp_buf_ptr = 0;
	_scl_internal_exceptions._cap = SCL_DEFAULT_STACK_FRAME_COUNT;
	_scl_internal_exceptions._scl_cs_ptr = malloc(_scl_internal_exceptions._cap * sizeof(scl_int));
	_scl_internal_exceptions._scl_exception_table = malloc(_scl_internal_exceptions._cap * sizeof(scl_any));
	_scl_internal_exceptions._scl_jmp_buf = malloc(_scl_internal_exceptions._cap * sizeof(jmp_buf));

#endif

	// Run __init__ functions
	for (int i = 0; _scl_internal_init_functions[i]; i++) {
		_scl_internal_init_functions[i]();
	}

#ifndef SCL_COMPILER_NO_MAIN

	int ret;
	int jmp = setjmp(_scl_internal_exceptions._scl_jmp_buf[_scl_internal_exceptions._scl_jmp_buf_ptr]);
	_scl_internal_exceptions._scl_jmp_buf_ptr++;
	if (jmp != 666) {

		// _scl_get_main_addr() returns NULL if compiled with --no-main
		ret = (_scl_main ? _scl_main(args, env) : 0);
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
	// call finalizers on instances and free all allocated memory
	_scl_finalize();

	// Run __destroy__ functions
	for (int i = 0; _scl_internal_destroy_functions[i]; i++) {
		_scl_internal_destroy_functions[i]();
	}

#ifndef SCL_COMPILER_NO_MAIN
	// We don't return
	exit(ret);
#endif
}
