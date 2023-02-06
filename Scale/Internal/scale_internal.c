#include "scale_internal.h"

#ifdef __cplusplus
#error C++ is not supported by Scale
#endif

/* Variables */

/* The Stack */
_scl_stack_t 		stack;

/* Scale Callstack */
_scl_callstack_t	callstk;

/* this is used by try-catch */
struct _exception_handling {
	void*	_scl_exception_table[STACK_SIZE];	/* The exception */
	jmp_buf	_scl_jmp_buf[STACK_SIZE];			/* jump buffer used by longjmp() */
	scl_int	_scl_jmp_buf_ptr;					/* number specifying the current depth */
	scl_int	_scl_cs_ptr[STACK_SIZE];			/* callstack-pointer of this catch level */
} exceptions = {0};

#define unimplemented do { fprintf(stderr, "%s:%d: %s: Not Implemented\n", __FILE__, __LINE__, __FUNCTION__); exit(1) } while (0)

#pragma region Memory

static scl_any alloced_ptrs[STACK_SIZE];	/* List of allocated pointers */
static scl_int alloced_ptrs_count = 0;		/* Length of the list */

void _scl_remove_ptr(scl_any ptr) {
	scl_int index;
	/* Finds all indices of the pointer in the table and removes them */
	while ((index = _scl_get_index_of_ptr(ptr)) != -1) {
		_scl_remove_ptr_at_index(index);
	}
}

/* Returns the next index of a given pointer in the allocated-pointers array */
/* Returns -1, if the pointer is not in the table */
scl_int _scl_get_index_of_ptr(scl_any ptr) {
	for (size_t i = 0; i < alloced_ptrs_count; i++) {
		if (alloced_ptrs[i] == ptr) {
			return i;
		}
	}
	return -1;
}

/* Removes the pointer at the given index and shift everything after left */
void _scl_remove_ptr_at_index(scl_int index) {
	for (scl_int i = index; i < alloced_ptrs_count - 1; i++) {
    	alloced_ptrs[i] = alloced_ptrs[i + 1];
    }
    alloced_ptrs_count--;
}

/* Adds a new pointer to the table */
/* Returns the next index of a given pointer in the allocated-pointers array */
void _scl_add_ptr(scl_any ptr) {
	for (scl_int i = 0; i < alloced_ptrs_count; i++) {
		if (alloced_ptrs[i] == 0 || alloced_ptrs[i] == ptr) {
			alloced_ptrs[i] = ptr;
			return;
		}
	}
	alloced_ptrs[alloced_ptrs_count++] = ptr;
}

/* Returns true if the pointer was allocated using _scl_alloc() */
scl_int _scl_check_allocated(scl_any ptr) {
	return _scl_get_index_of_ptr(ptr) != -1;
}

/* Allocates size bytes of memory */
/* the memory will always have a size of a multiple of sizeof(scl_int) */
scl_any _scl_alloc(size_t size) {

	// Make size be next biggest multiple of sizeof(scl_int)
	if (size % sizeof(scl_any) != 0) {
		size += size % sizeof(scl_any);
	}

	// Allocate the memory
	scl_any ptr = (scl_any) malloc(size);

	// Hard-throw if memory allocation failed
	if (!ptr) {
		_scl_security_throw(EX_BAD_PTR, "malloc() failed!");
		return NULL;
	}

	// Add the pointer to the table
	_scl_add_ptr(ptr);
	return ptr;
}

/* Reallocates a pointer to a new size */
/* the memory will always have a size of a multiple of sizeof(scl_int) */
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
	_scl_add_ptr(ptr);

	// Add the pointer back to our struct table, if it was a struct
	if (wasStruct)
		_scl_add_struct(ptr);
	return ptr;
}

/* Frees a pointer */
/* Will do nothing, if the pointer is NULL, or wasn't allocated with _scl_alloc() */
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

#pragma endregion

#pragma region Security

/* Assert, that 'b' is true */
void _scl_assert(scl_int b, scl_str msg) {
	if (!b) {
		printf("\n");
		printf("%s:" SCL_INT_FMT ":" SCL_INT_FMT ": ", callstk.data[callstk.ptr - 1].file, callstk.data[callstk.ptr - 1].line, callstk.data[callstk.ptr - 1].col);
		printf("Assertion failed: %s\n", msg);
		print_stacktrace();

		_scl_security_safe_exit(EX_ASSERTION_FAIL);
	}
}

/* Hard-throw an exception */
_scl_no_return void _scl_security_throw(int code, scl_str msg) {
	printf("\n");
	printf("Exception: %s\n", msg);
	if (errno) {
		printf("errno: %s\n", strerror(errno));
	}
	print_stacktrace();

	_scl_security_safe_exit(code);
}

/* exit() */
_scl_no_return void _scl_security_safe_exit(int code) {

	// Call finalizers on instances
	_scl_finalize();

	// exit
	exit(code);
}

/* Returns true, if the pointer is still accessible on the stack */
int find_below(scl_any ptr, scl_int index) {
	for (scl_int i = index; i >= 0; i--) {
		if (stack.data[i].v == ptr) {
			return 1;
		}
	}
	return 0;
}

/* Collect left-over garbage after a function is finished */
void _scl_cleanup_post_func(scl_int depth) {

	// Get the difference in stack sizes
	scl_int diff = stack.ptr - callstk.data[depth].begin_stack_size;

	// Get the stack size at the beginning of the function
	scl_int begin = callstk.data[depth].begin_stack_size;

	// BAD:
	// We assume that any pointers are inaccessible
	for (scl_int i = diff; diff > 0 && i >= 0; i--) {
		if (!find_below(stack.data[i].v, begin)) {

			// No need to check for proper allocation
			// _scl_free() does that already
			_scl_free(stack.data[i].v);
		}
		stack.ptr--;
	}
	if (stack.ptr < 0)
		stack.ptr = 0;
}

#pragma endregion

#pragma region Exceptions

static int printingStacktrace = 0;

void print_stacktrace() {
	printf("Stacktrace:\n");
	printingStacktrace = 1;
	for (signed long i = callstk.ptr - 1; i >= 0; i--) {
		if (callstk.data[i].file) {
			char* f = strrchr(callstk.data[i].file, '/');
			if (!f) f = callstk.data[i].file;
			else f++;
			printf("  %s -> %s:" SCL_INT_FMT ":" SCL_INT_FMT "\n", (scl_str) callstk.data[i].func, f, callstk.data[i].line, callstk.data[i].col);
		} else {
			printf("  %s -> (nil):" SCL_INT_FMT ":" SCL_INT_FMT "\n", (scl_str) callstk.data[i].func, callstk.data[i].line, callstk.data[i].col);
		}

	}
	printingStacktrace = 0;
	printf("\n");
}

/* final signal handler */
/* if we get here, something has gone VERY wrong */
void _scl_catch_final(int sig_num) {
	scl_str signalString;
	// Signals
	if (sig_num == -1) signalString = NULL;
#ifdef SIGABRT
	else if (sig_num == SIGABRT) signalString = "abort() called";
#endif
#ifdef SIGFPE
	else if (sig_num == SIGFPE) signalString = "Floating point exception";
#endif
#ifdef SIGILL
	else if (sig_num == SIGILL) signalString = "Illegal instruction";
#endif
#ifdef SIGINT
	else if (sig_num == SIGINT) signalString = "Software Interrupt (^C)";
#endif
#ifdef SIGSEGV
	else if (sig_num == SIGSEGV) signalString = "Invalid/Illegal Memory Access (Segmentation violation)";
#endif
#ifdef SIGBUS
	else if (sig_num == SIGBUS) signalString = "Unaccessible Memory Access (Bus error)";
#endif
#ifdef SIGTERM
	else if (sig_num == SIGTERM) signalString = "Software Termination";
#endif
	else signalString = "Unknown signal";

	printf("\n");

	printf("%s:" SCL_INT_FMT ":" SCL_INT_FMT ": Exception: %s\n", callstk.data[callstk.ptr - 1].file, callstk.data[callstk.ptr - 1].line, callstk.data[callstk.ptr - 1].col, signalString);
	if (errno) {
		printf("errno: %s\n", strerror(errno));
	}
	if (!printingStacktrace)
		print_stacktrace();
	printf("Stack:\n");
	for (scl_int i = stack.ptr - 1; i >= 0; i--) {
		scl_int v = stack.data[i].i;
		printf("   " SCL_INT_FMT ": 0x" SCL_INT_HEX_FMT ", " SCL_INT_FMT "\n", i, v, v);
	}
	printf("\n");

	_scl_security_safe_exit(sig_num);
}

#pragma endregion

#pragma region Stack Operations
struct scl_Array {
	scl_int $__type__;
	scl_str $__type_name__;
	scl_any $__super_list__;
	scl_any $__super_list_len__;
	scl_any values;
	scl_any count;
	scl_any capacity;
	scl_int pos;
};

/* Converts C's argc-argv construct to an Array-instance */
scl_any _scl_c_args_to_scl_args(scl_int argc, scl_str argv[]) {
	struct scl_Array* array = _scl_alloc_struct(sizeof(struct scl_Array), "Array", hash1("SclObject"));
	array->capacity = (scl_any) argc;
	array->count = 0;
	array->values = _scl_alloc(argc);
	for (scl_int i = 0; i < argc; i++) {
		((scl_any*) array->values)[(scl_int) array->count++] = argv[i];
	}
	return array;
}

/* Converts C's envp NULL-terminated list to an Array-instance */
scl_any _scl_c_envp_to_scl_env(scl_str envp[]) {
	struct scl_Array* array = _scl_alloc_struct(sizeof(struct scl_Array), "Array", hash1("SclObject"));
	scl_int cap = 0;
	while (envp[cap]) {
		envp += sizeof(scl_str*);
		cap++;
	}

	envp -= cap * sizeof(scl_str*);

	array->capacity = (scl_any) cap;
	array->count = 0;
	array->values = _scl_alloc(cap);
	for (scl_int i = 0; i < cap; i++) {
		((scl_any*) array->values)[(scl_int) array->count++] = envp[i];
	}
	return array;
}

inline _scl_frame_t ctrl_push_frame() {
	return stack.data[stack.ptr++];
}

inline _scl_frame_t ctrl_pop_frame() {
	return stack.data[--stack.ptr];
}

inline void ctrl_push_string(scl_str c) {
	stack.data[stack.ptr++].s = c;
}

inline void ctrl_push_double(scl_float d) {
	stack.data[stack.ptr++].f = d;
}

inline void ctrl_push_long(scl_int n) {
	stack.data[stack.ptr++].i = n;
}

inline void ctrl_push(scl_any n) {
	stack.data[stack.ptr++].v = n;
}

inline scl_int ctrl_pop_long() {
	return stack.data[--stack.ptr].i;
}

inline scl_float ctrl_pop_double() {
	return stack.data[--stack.ptr].f;
}

inline scl_str ctrl_pop_string() {
	return stack.data[--stack.ptr].s;
}

inline scl_any ctrl_pop() {
	return stack.data[--stack.ptr].v;
}

inline ssize_t ctrl_stack_size(void) {
	return stack.ptr;
}

#pragma endregion

#pragma region Struct and Reflection

hash hash1(char* data) {
    hash h = 7;
    for (int i = 0; i < strlen(data); i++) {
        h = h * 31 + data[i];
    }
    return h;
}

struct _scl_methodinfo {
  scl_int  __type__;
  scl_str  __type_name__;
  scl_int  __super__;
  scl_int  __size__;
  scl_int  name_hash;
  scl_str  name;
  scl_int  id;
  scl_any  ptr;
  scl_int  member_type;
  scl_int  arg_count;
  scl_str  return_type;
};

struct _scl_typeinfo {
  scl_int    			   __type__;
  scl_str    			   __type_name__;
  scl_int   			   __super__;
  scl_int    			   __size__;
  scl_int                  type;
  scl_str                  name;
  scl_int                  size;
  scl_int                  super;
  scl_int                  methodscount;
  struct _scl_methodinfo**  methods;
};

// Structs
extern struct _scl_typeinfo		_scl_internal_types[];
extern size_t 					_scl_internal_types_count;

// Functions
extern struct _scl_methodinfo*	_scl_internal_functions[];
extern size_t 					_scl_internal_functions_size;

// Methods
extern struct _scl_methodinfo*	_scl_internal_methods[];
extern size_t 					_scl_internal_methods_size;

// generic struct
struct sclstruct {
	scl_int  type;
	scl_str  type_name;
	scl_int  super;
	scl_int  size;
};

// table of instances
static struct sclstruct* allocated_structs[STACK_SIZE];
static size_t allocated_structs_count = 0;

// table of instances allocated with _scl_alloc_struct()
static struct sclstruct* mallocced_structs[STACK_SIZE];
static size_t mallocced_structs_count = 0;

/* Free all allocated instances */
void _scl_finalize() {
	for (size_t i = 0; i < mallocced_structs_count; i++) {
		if (mallocced_structs[i]) {
			_scl_free(mallocced_structs[i]);
		}
		mallocced_structs[i] = 0;
	}
}

/* Returns a pointer to the typeinfo of the given struct */
/* NULL if the struct does not exist */
void* _scl_typeinfo_of(hash type) {
	for (size_t i = 0; i < _scl_internal_types_count; i++) {
		if (_scl_internal_types[i].type == type) {
			return (void*) _scl_internal_types + (i * sizeof(struct _scl_typeinfo));
		}
	}
	return NULL;
}

struct _scl_typeinfo* _scl_find_typeinfo_of(hash type) {
	return (struct _scl_typeinfo*) _scl_typeinfo_of(type);
}

/* returns the method handle of a method on a struct, or a parent struct */
void* _scl_get_method_on_type(hash type, hash method) {
	struct _scl_typeinfo* p = _scl_find_typeinfo_of(type);
	while (p) {
		for (scl_int m = 0; m < p->methodscount; m++) {
			if (p->methods[m]->id == method) {
				return p->methods[m]->ptr;
			}
		}
		p = _scl_find_typeinfo_of(p->super);
	}
	return NULL;
}

/* adds an instance to the table of tracked instances */
scl_any _scl_add_struct(scl_any ptr) {
	for (size_t i = 0; i < allocated_structs_count; i++) {
		if (allocated_structs[i] == 0 || allocated_structs[i] == ptr) {
			allocated_structs[i] = ptr;
			return ptr;
		}
	}
	allocated_structs[allocated_structs_count++] = ptr;
	return ptr;
}

/* creates a new instance with a size of 'size' */
scl_any _scl_alloc_struct(size_t size, scl_str type_name, hash super) {

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

	// Add struct to allocated table
	for (size_t i = 0; i < mallocced_structs_count; i++) {
		if (mallocced_structs[i] == 0 || mallocced_structs[i] == ptr) {
			mallocced_structs[i] = ptr;
			return _scl_add_struct(ptr);
		}
	}
	mallocced_structs[mallocced_structs_count++] = ptr;

	return _scl_add_struct(ptr);
}

/* Removes an instance from the allocated table by index */
static void _scl_struct_map_remove(size_t index) {
    for (size_t i = index; i < allocated_structs_count - 1; i++) {
       allocated_structs[i] =allocated_structs[i + 1];
    }
    allocated_structs_count--;
}

/* Returns the next index of an instance in the allocated table */
/* Returns -1, if not in table */
size_t _scl_find_index_of_struct(scl_any ptr) {
	if (ptr == NULL) return -1;
	for (size_t i = 0; i < allocated_structs_count; i++) {
        if (allocated_structs[i] == ptr) {
            return i;
        }
    }

    return -1;
}

// SclObject:finalize()
void _scl_reflect_call_method_SclObject_function_finalize();

/* Removes an instance from the allocated table without calling its finalizer */
void _scl_free_struct_no_finalize(scl_any ptr) {
	size_t i = _scl_find_index_of_struct(ptr);
	while (i != -1) {
		_scl_struct_map_remove(i);
		i = _scl_find_index_of_struct(ptr);
	}
}

/* Removes an instance from the allocated table and call the finalzer */
void _scl_free_struct(scl_any ptr) {
	if (_scl_find_index_of_struct(ptr) != -1) {
		scl_any method = _scl_get_method_on_type(((struct sclstruct*) ptr)->type, hash1("finalize"));
		if (method) {
			stack.data[stack.ptr++].v = ptr;
			((void(*)()) method)();
		}
	}
	size_t i = _scl_find_index_of_struct(ptr);
	while (i != -1) {
		_scl_struct_map_remove(i);
		i = _scl_find_index_of_struct(ptr);
	}
}

/* Returns true if the given type is a child of the other type */
scl_int _scl_type_extends_type(struct _scl_typeinfo* type, struct _scl_typeinfo* extends) {
	do {
		if (type->type == extends->type) {
			return 1;
		}
		type = _scl_find_typeinfo_of(type->super);
	} while (type);
	return 0;
}

/* Returns true, if the instance is of a given struct type */
scl_int _scl_struct_is_type(scl_any ptr, hash typeId) {
	int isStruct = 0;
	for (size_t i = 0; i < allocated_structs_count; i++) {
		if (allocated_structs[i] == ptr) {
			isStruct = 1;
			break;
		}
	}
	if (!isStruct) return 0;

	struct sclstruct* ptrStruct = (struct sclstruct*) ptr;

	struct _scl_typeinfo* ptrType = _scl_typeinfo_of(ptrStruct->type);
	struct _scl_typeinfo* typeIdType = _scl_typeinfo_of(typeId);

	return _scl_type_extends_type(ptrType, typeIdType);
}

/* Reflectively calls a function */
void _scl_reflect_call(hash func) {
	for (size_t i = 0; i < _scl_internal_functions_size; i++) {
		if (_scl_internal_functions[i]->name_hash == func) {
			((void(*)()) _scl_internal_functions[i]->ptr)();
			return;
		}
	}
	_scl_security_throw(EX_REFLECT_ERROR, "Could not find function.");
}

/* Returns true, if the given function exists */
scl_int _scl_reflect_find(hash func) {
	for (size_t i = 0; i < _scl_internal_functions_size; i++) {
		if (_scl_internal_functions[i]->name_hash == func) {
			return 1;
		}
	}
	return 0;
}

/* Returns true, if the given method exists */
scl_int _scl_reflect_find_method(hash func) {
	for (size_t i = 0; i < _scl_internal_methods_size; i++) {
		if (_scl_internal_methods[i]->name_hash == func) {
			return 1;
		}
	}
	return 0;
}

/* Reflectively calls a method */
void _scl_reflect_call_method(hash func) {
	for (size_t i = 0; i < _scl_internal_methods_size; i++) {
		if (_scl_internal_methods[i]->name_hash == func) {
			((void(*)()) _scl_internal_methods[i]->ptr)();
			return;
		}
	}
	_scl_security_throw(EX_REFLECT_ERROR, "Could not find method.");
}

#pragma endregion

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
#if defined(SIGALRM)
	signal(SIGALRM, _scl_catch_final);
#endif
#if defined(SIGTERM)
	signal(SIGTERM, _scl_catch_final);
#endif
#if defined(SIGURG)
	signal(SIGURG, _scl_catch_final);
#endif
#if defined(SIGSTOP)
	signal(SIGSTOP, _scl_catch_final);
#endif
#if defined(SIGTSTP)
	signal(SIGTSTP, _scl_catch_final);
#endif
#if defined(SIGCONT)
	signal(SIGCONT, _scl_catch_final);
#endif
#if defined(SIGCHLD)
	signal(SIGCHLD, _scl_catch_final);
#endif
#if defined(SIGTTIN)
	signal(SIGTTIN, _scl_catch_final);
#endif
#if defined(SIGTTOU)
	signal(SIGTTOU, _scl_catch_final);
#endif
#if defined(SIGIO)
	signal(SIGIO, _scl_catch_final);
#endif
#if defined(SIGXCPU)
	signal(SIGXCPU, _scl_catch_final);
#endif
#if defined(SIGXFSZ)
	signal(SIGXFSZ, _scl_catch_final);
#endif
#if defined(SIGVTALRM)
	signal(SIGVTALRM, _scl_catch_final);
#endif
#if defined(SIGPROF)
	signal(SIGPROF, _scl_catch_final);
#endif
#if defined(SIGWINCH)
	signal(SIGWINCH, _scl_catch_final);
#endif
#if defined(SIGINFO)
	signal(SIGINFO, _scl_catch_final);
#endif
#if defined(SIGUSR1)
	signal(SIGUSR1, _scl_catch_final);
#endif
#if defined(SIGUSR2)
	signal(SIGUSR2, _scl_catch_final);
#endif
}

/* Returns a function pointer with the following signature: */
/* function main(args: Array, env: Array): int */
scl_any _scl_get_main_addr();
typedef int(*mainFunc)(struct scl_Array*, struct scl_Array*);

// __init__ and __destroy__
typedef void(*genericFunc)();

// __init__
// last element is always NULL
extern genericFunc init_functions[];

// __destroy__
// last element is always NULL
extern genericFunc destroy_functions[];

#ifndef SCL_COMPILER_NO_MAIN
_scl_no_return int _scl_native_main(int argc, char** argv, char** envp) __asm(_scl_macro_to_string(__USER_LABEL_PREFIX__) "main");
_scl_no_return int _scl_native_main(int argc, char** argv, char** envp) {

	// Register signal handler for all available signals
	_scl_set_up_signal_handler();

	callstk.data[0].file = "_scl_native_main";

	// Convert argv and envp from native arrays to Scale arrays
	struct scl_Array* args = (struct scl_Array*) _scl_c_args_to_scl_args(argc, argv);
	struct scl_Array* env = (struct scl_Array*) _scl_c_envp_to_scl_env(envp);

	// Get the address of the main function
	mainFunc _scl_main = (mainFunc) _scl_get_main_addr();
#else

/* Initialize as library */
_scl_constructor void _scl_load() {
#endif
	
	// Run __init__ functions
	for (int i = 0; init_functions[i]; i++) {
		init_functions[i]();
	}

#ifndef SCL_COMPILER_NO_MAIN

	// _scl_get_main_addr() returns NULL if compiled with --no-main
	int ret = (_scl_main ? _scl_main(args, env) : 0);

#else
}

/* Uninitialize library */
_scl_destructor void _scl_destroy() {
#endif

	// Run __destroy__ functions
	for (int i = 0; destroy_functions[i]; i++) {
		destroy_functions[i]();
	}

#ifndef SCL_COMPILER_NO_MAIN
	// Run finalization:
	// call finalizers on instances and free all allocated memory
	_scl_finalize();

	// We don't return
	exit(ret);
#endif
}
