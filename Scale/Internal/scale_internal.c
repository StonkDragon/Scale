#include "scale_internal.h"

#ifdef __cplusplus
#error C++ is not supported by Scale
#endif

/* Variables */
_scl_stack_t 		stack;
_scl_callstack_t	callstk;
struct _exception_handling {
	void*	_scl_exception_table[STACK_SIZE];
	jmp_buf	_scl_jmp_buf[STACK_SIZE];
	scl_int	_scl_jmp_buf_ptr;
	scl_int	_scl_cs_ptr[STACK_SIZE];
} exceptions = {0};

#define unimplemented do { fprintf(stderr, "%s:%d: %s: Not Implemented\n", __FILE__, __LINE__, __FUNCTION__); exit(1) } while (0)

#pragma region Memory

static scl_any alloced_ptrs[STACK_SIZE];
static scl_int alloced_ptrs_count = 0;

void _scl_remove_ptr(scl_any ptr) {
	scl_int index;
	while ((index = _scl_get_index_of_ptr(ptr)) != -1) {
		_scl_remove_ptr_at_index(index);
	}
}

scl_int _scl_get_index_of_ptr(scl_any ptr) {
	for (size_t i = 0; i < alloced_ptrs_count; i++) {
		if (alloced_ptrs[i] == ptr) {
			return i;
		}
	}
	return -1;
}

void _scl_remove_ptr_at_index(scl_int index) {
	for (scl_int i = index; i < alloced_ptrs_count - 1; i++) {
    	alloced_ptrs[i] = alloced_ptrs[i + 1];
    }
    alloced_ptrs_count--;
}

void _scl_add_ptr(scl_any ptr) {
	for (scl_int i = 0; i < alloced_ptrs_count; i++) {
		if (alloced_ptrs[i] == 0 || alloced_ptrs[i] == ptr) {
			alloced_ptrs[i] = ptr;
			return;
		}
	}
	alloced_ptrs[alloced_ptrs_count++] = ptr;
}

scl_int _scl_check_allocated(scl_any ptr) {
	return _scl_get_index_of_ptr(ptr) != -1;
}

scl_any _scl_alloc(size_t size) {
	if (size % sizeof(scl_any) != 0) {
		size += size % sizeof(scl_any);
	}
	scl_any ptr = (scl_any) malloc(size);
	if (!ptr) {
		_scl_security_throw(EX_BAD_PTR, "malloc() failed!");
		return NULL;
	}
	_scl_add_ptr(ptr);
	return ptr;
}

scl_any _scl_realloc(scl_any ptr, size_t size) {
	if (size % sizeof(scl_any) != 0) {
		size += size % sizeof(scl_any);
	}
	_scl_free_struct_no_finalize(ptr);
	_scl_remove_ptr(ptr);
	ptr = realloc(ptr, size);
	if (!ptr) {
		_scl_security_throw(EX_BAD_PTR, "realloc() failed!");
		return NULL;
	}
	_scl_add_ptr(ptr);
	_scl_add_struct(ptr);
	return ptr;
}

void _scl_free(scl_any ptr) {
	if (ptr && _scl_check_allocated(ptr)) {
		_scl_free_struct(ptr);
		_scl_remove_ptr(ptr);
		free(ptr);
	}
}

#pragma endregion

#pragma region Security

void _scl_assert(scl_int b, scl_str msg) {
	if (!b) {
		printf("\n");
		printf("%s:" SCL_INT_FMT ":" SCL_INT_FMT ": ", callstk.data[callstk.ptr - 1].file, callstk.data[callstk.ptr - 1].line, callstk.data[callstk.ptr - 1].col);
		printf("Assertion failed: %s\n", msg);
		print_stacktrace();

		_scl_security_safe_exit(EX_ASSERTION_FAIL);
	}
}

_scl_no_return
void _scl_security_throw(int code, scl_str msg) {
	printf("\n");
	printf("Exception: %s\n", msg);
	if (errno) {
		printf("errno: %s\n", strerror(errno));
	}
	print_stacktrace();

	_scl_security_safe_exit(code);
}

_scl_no_return
void _scl_security_safe_exit(int code) {
	_scl_finalize();
	exit(code);
}

int find_below(scl_any ptr, scl_int index) {
	for (scl_int i = index; i >= 0; i--) {
		if (stack.data[i].v == ptr) {
			return 1;
		}
	}
	return 0;
}

void _scl_cleanup_post_func(scl_int depth) {
	scl_int diff = stack.ptr - callstk.data[depth].begin_stack_size;
	scl_int begin = callstk.data[depth].begin_stack_size;
	for (scl_int i = diff; diff > 0 && i >= 0; i--) {
		if (!find_below(stack.data[i].v, begin)) {
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

void process_signal(int sig_num) {
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

void ctrl_push_args(scl_int argc, scl_str argv[]) {
	struct Array {
		scl_int $__type__;
		scl_str $__type_name__;
		scl_any $__super_list__;
		scl_any $__super_list_len__;
		scl_any values;
		scl_any count;
		scl_any capacity;
	};
	struct Array* array = _scl_alloc_struct(sizeof(struct Array), "Array", hash1("SclObject"));
	array->capacity = (scl_any) argc;
	array->count = 0;
	array->values = _scl_alloc(argc);
	for (scl_int i = 0; i < argc; i++) {
		((scl_any*) array->values)[(scl_int) array->count++] = argv[i];
	}
	stack.data[stack.ptr++].v = array;
}

inline _scl_frame_t _scl_push_frame() {
	return stack.data[stack.ptr++];
}

inline _scl_frame_t _scl_pop_frame() {
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

extern struct _scl_typeinfo		_scl_internal_types[];
extern size_t 					_scl_internal_types_count;
extern struct _scl_methodinfo*	_scl_internal_functions[];
extern size_t 					_scl_internal_functions_size;
extern struct _scl_methodinfo*	_scl_internal_methods[];
extern size_t 					_scl_internal_methods_size;

struct sclstruct {
	scl_int  type;
	scl_str  type_name;
	scl_int  super;
	scl_int  size;
};

static struct sclstruct* allocated_structs[STACK_SIZE];
static size_t allocated_structs_count = 0;
static struct sclstruct* mallocced_structs[STACK_SIZE];
static size_t mallocced_structs_count = 0;

void _scl_finalize() {
	for (size_t i = 0; i < mallocced_structs_count; i++) {
		if (mallocced_structs[i]) {
			_scl_free(mallocced_structs[i]);
		}
		mallocced_structs[i] = 0;
	}
}

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

scl_any _scl_alloc_struct(size_t size, scl_str type_name, hash super) {
	scl_any ptr = _scl_alloc(size);
	((struct sclstruct*) ptr)->type = hash1(type_name);
    ((struct sclstruct*) ptr)->type_name = type_name;
    ((struct sclstruct*) ptr)->super = super;
    ((struct sclstruct*) ptr)->size = size;

	for (size_t i = 0; i < mallocced_structs_count; i++) {
		if (mallocced_structs[i] == 0 || mallocced_structs[i] == ptr) {
			mallocced_structs[i] = ptr;
			return _scl_add_struct(ptr);
		}
	}
	mallocced_structs[mallocced_structs_count++] = ptr;

	return _scl_add_struct(ptr);
}

static void _scl_struct_map_remove(size_t index) {
    for (size_t i = index; i < allocated_structs_count - 1; i++) {
       allocated_structs[i] =allocated_structs[i + 1];
    }
    allocated_structs_count--;
}

size_t _scl_find_index_of_struct(scl_any ptr) {
	if (ptr == NULL) return -1;
	for (size_t i = 0; i < allocated_structs_count; i++) {
        if (allocated_structs[i] == ptr) {
            return i;
        }
    }

    return -1;
}

void _scl_reflect_call_method_SclObject_function_finalize();

void _scl_free_struct_no_finalize(scl_any ptr) {
	size_t i = _scl_find_index_of_struct(ptr);
	while (i != -1) {
		_scl_struct_map_remove(i);
		i = _scl_find_index_of_struct(ptr);
	}
}

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

scl_int _scl_type_extends_type(struct _scl_typeinfo* type, struct _scl_typeinfo* extends) {
	do {
		if (type->type == extends->type) {
			return 1;
		}
		type = _scl_find_typeinfo_of(type->super);
	} while (type);
	return 0;
}

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

void _scl_reflect_call(hash func) {
	for (size_t i = 0; i < _scl_internal_functions_size; i++) {
		if (_scl_internal_functions[i]->name_hash == func) {
			((void(*)()) _scl_internal_functions[i]->ptr)();
			return;
		}
	}
	_scl_security_throw(EX_REFLECT_ERROR, "Could not find function.");
}

scl_int _scl_reflect_find(hash func) {
	for (size_t i = 0; i < _scl_internal_functions_size; i++) {
		if (_scl_internal_functions[i]->name_hash == func) {
			return 1;
		}
	}
	return 0;
}

scl_int _scl_reflect_find_method(hash func) {
	for (size_t i = 0; i < _scl_internal_methods_size; i++) {
		if (_scl_internal_methods[i]->name_hash == func) {
			return 1;
		}
	}
	return 0;
}

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
