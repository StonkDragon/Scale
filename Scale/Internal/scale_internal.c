#include "scale_internal.h"

#ifdef __cplusplus
#error C++ is not supported by Scale
#endif

/* Variables */
scl_stack_t  stack = {0, {0}};
scl_stack_t	 callstk = {0, {0}};

#define unimplemented do { fprintf(stderr, "%s:%d: %s: Not Implemented\n", __FILE__, __LINE__, __FUNCTION__); exit(1) } while (0)

#pragma region Memory

scl_value scl_alloc(size_t size) {
	scl_value ptr = malloc(size);
	if (!ptr) {
		scl_security_throw(EX_BAD_PTR, "malloc() failed!");
		return NULL;
	}
	return ptr;
}

scl_value scl_realloc(scl_value ptr, size_t size) {
	ptr = realloc(ptr, size);
	if (!ptr) {
		scl_security_throw(EX_BAD_PTR, "realloc() failed!");
		return NULL;
	}
	return ptr;
}

void scl_free(scl_value ptr) {
	free(ptr);
}

#pragma endregion

#pragma region Security

void scl_security_throw(int code, scl_str msg) {
	printf("\n");
	printf("Exception: %s\n", msg);
	if (errno) {
		printf("errno: %s\n", strerror(errno));
	}
	print_stacktrace();

	scl_security_safe_exit(code);
}

void scl_security_safe_exit(int code) {
	exit(code);
}

#pragma endregion

#pragma region Exceptions

void print_stacktrace() {
	printf("Stacktrace:\n");
	for (int i = callstk.ptr - 1; i >= 0; i--) {
		printf("  %s\n", (scl_str) callstk.data[i].i);
	}
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
	printf("Exception: %s\n", signalString);
	if (errno) {
		printf("errno: %s\n", strerror(errno));
	}
	print_stacktrace();
	printf("Stack:\n");
	for (ssize_t i = stack.ptr - 1; i >= 0; i--) {
		long long v = (long long) stack.data[i].i;
		printf("   %zd: 0x%016llx, %lld\n", i, v, v);
	}
	printf("\n");

	scl_security_safe_exit(sig_num);
}

#pragma endregion

#pragma region Stack Operations

void ctrl_push_args(scl_int argc, scl_str argv[]) {
	struct Array {
		scl_int $__type__;
		scl_str $__type_name__;
		scl_value $__lock__;
		scl_value values;
		scl_value count;
		scl_value capacity;
	};
	struct Array* array = scl_alloc_struct(sizeof(struct Array), "Array");
	array->capacity = (scl_value) argc;
	array->count = 0;
	array->values = scl_alloc(argc);
	for (scl_int i = 0; i < argc; i++) {
		((scl_value*) array->values)[(scl_int) array->count++] = argv[i];
	}
	stack.data[stack.ptr++].v = array;
}

void ctrl_push_string(scl_str c) {
	stack.data[stack.ptr++].s = c;
}

void ctrl_push_double(scl_float d) {
	stack.data[stack.ptr++].f = d;
}

void ctrl_push_long(scl_int n) {
	stack.data[stack.ptr++].i = n;
}

void ctrl_push(scl_value n) {
	stack.data[stack.ptr++].v = n;
}

scl_int ctrl_pop_long() {
	return stack.data[--stack.ptr].i;
}

scl_float ctrl_pop_double() {
	return stack.data[--stack.ptr].f;
}

scl_str ctrl_pop_string() {
	return stack.data[--stack.ptr].s;
}

scl_value ctrl_pop() {
	return stack.data[--stack.ptr].v;
}

ssize_t ctrl_stack_size(void) {
	return stack.ptr;
}

#pragma endregion

#pragma region Struct

typedef unsigned long long hash;

static hash hash1(char* data) {
    hash h = 7;
    for (int i = 0; i < strlen(data); i++) {
        h = h * 31 + data[i];
    }
    return h;
}

scl_value scl_alloc_struct(size_t size, scl_str type_name) {
	scl_value ptr = scl_alloc(size);
	((struct {scl_int type; scl_str type_name; scl_value lock;}*) ptr)->type = hash1(type_name);
    ((struct {scl_int type; scl_str type_name; scl_value lock;}*) ptr)->type_name = type_name;
    ((struct {scl_int type; scl_str type_name; scl_value lock;}*) ptr)->lock = 0;
	return ptr;
}

#pragma endregion
