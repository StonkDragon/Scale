#include "scale_internal.h"

#ifdef __cplusplus
#error C++ is not supported by Scale
#endif

/* Variables */
char* 		 current_file = "<init>";
size_t 		 current_line = 0;
size_t 		 current_column = 0;
size_t 		 stack_depth = 0;
scl_stack_t  stack = {0, {0}, {0}};
scl_stack_t	 callstk = {0, {0}, {0}};
size_t 		 sap_index = 0;
size_t 		 sap_enabled[STACK_SIZE] = {0};
size_t 		 sap_count[STACK_SIZE] = {0};
scl_value    alloced_complexes[STACK_SIZE] = {0};
size_t 		 alloced_complexes_count = 0;

#define UNIMPLEMENTED fprintf(stderr, "%s:%d: %s: Not Implemented\n", __FILE__, __LINE__, __FUNCTION__); exit(1)

#pragma region Security

void scl_security_throw(int code, char* msg) {
	printf("\n");
	printf("%s:%zu:%zu: %s\n", current_file, current_line, current_column, msg);
	if (errno) {
		printf("errno: %s\n", strerror(errno));
	}
	print_stacktrace();

	scl_security_safe_exit(code);
}

void scl_security_required_arg_count(ssize_t n, char* func) {
	if ((stack.ptr - stack.offset[stack_depth]) < n) {
		char* err = (char*) malloc(MAX_STRING_SIZE);
		sprintf(err, "Error: Function %s requires %zu arguments, but only %zu are provided.", func, n, stack.ptr);
		scl_security_throw(EX_STACK_UNDERFLOW, err);
	}
}

void scl_security_safe_exit(int code) {
	exit(code);
}

void scl_security_check_null(scl_value ptr) {
	if (ptr == NULL) {
		scl_security_throw(EX_BAD_PTR, "Null pointer");
	}
}

#pragma endregion

#pragma region Function Management

void ctrl_fn_start(char* name) {
	callstk.data[callstk.ptr++].ptr = name;
	stack_depth++;
	stack.offset[stack_depth] = stack.ptr;
}

void ctrl_fn_end() {
	callstk.ptr--;
	stack.offset[stack_depth] = 0;
	stack_depth--;
}

void sap_open(void) {
	sap_index++;
	if (sap_index >= STACK_SIZE) {
		scl_security_throw(EX_SAP_ERROR, "Exhaustive use of SAP");
	}
	sap_enabled[sap_index] = 1;
	sap_count[sap_index] = 0;
}

void sap_close(void) {
	if (sap_index == 0) {
		scl_security_throw(EX_SAP_ERROR, "No SAP open");
	}
	sap_enabled[sap_index] = 0;
	for (size_t i = 0; i < sap_count[sap_index]; i++) {
		ctrl_pop();
	}
	sap_index--;
}

void ctrl_fn_nps_start(char* name) {
	callstk.data[callstk.ptr++].ptr = name;
}

void ctrl_fn_nps_end() {
	callstk.ptr--;
}

#pragma endregion

#pragma region Exceptions

void ctrl_set_file(char* file) {
	current_file = file;
}

void ctrl_set_pos(size_t line, size_t col) {
	current_line = line;
	current_column = col;
}

void print_stacktrace() {
	printf("Stacktrace:\n");
	for (int i = callstk.ptr - 1; i >= 0; i--) {
		printf("  %s\n", (char*) callstk.data[i].ptr);
	}
	printf("\n");
}

void process_signal(int sig_num)
{
	char* signalString;
	// Signals
	if (sig_num == SIGABRT) signalString = "abort() called";
	else if (sig_num == SIGFPE) signalString = "Floating point exception";
	else if (sig_num == SIGILL) signalString = "Illegal instruction";
	else if (sig_num == SIGINT) signalString = "Software Interrupt (^C)";
	else if (sig_num == SIGSEGV) signalString = "Invalid/Illegal Memory Access";
	else if (sig_num == SIGBUS) signalString = "Unaccessible Memory Access";
	else if (sig_num == SIGTERM) signalString = "Software Termiation";
	else signalString = "Unknown signal";

	printf("\n");
	printf("%s:%zu:%zu: %s\n", current_file, current_line, current_column, signalString);
	if (errno) {
		printf("errno: %s\n", strerror(errno));
	}
	print_stacktrace();

	scl_security_safe_exit(sig_num);
}

#pragma endregion

#pragma region Stack Operations

void ctrl_push_string(const char* c) {
	if (sap_enabled[sap_index]) {
		sap_count[sap_index]++;
	}
	if (stack.ptr + 1 >= STACK_SIZE) {
		scl_security_throw(EX_STACK_OVERFLOW, "Stack overflow!");
	}
	stack.data[stack.ptr].ptr = (scl_value) c;
	stack.ptr++;
}

void ctrl_push_double(double d) {
	if (sap_enabled[sap_index]) {
		sap_count[sap_index]++;
	}
	if (stack.ptr + 1 >= STACK_SIZE) {
		scl_security_throw(EX_STACK_OVERFLOW, "Stack overflow!");
	}
	stack.data[stack.ptr].floating = d;
	stack.ptr++;
}

void ctrl_push_long(long long n) {
	if (sap_enabled[sap_index]) {
		sap_count[sap_index]++;
	}
	if (stack.ptr + 1 >= STACK_SIZE) {
		scl_security_throw(EX_STACK_OVERFLOW, "Stack overflow!");
	}
	stack.data[stack.ptr].ptr = (scl_value) n;
	stack.ptr++;
}

void ctrl_push(scl_value n) {
	if (sap_enabled[sap_index]) {
		sap_count[sap_index]++;
	}
	if (stack.ptr + 1 >= STACK_SIZE) {
		scl_security_throw(EX_STACK_OVERFLOW, "Stack overflow!");
	}
	stack.data[stack.ptr].ptr = n;
	stack.ptr++;
}

long long ctrl_pop_long() {
	if (sap_enabled[sap_index] && sap_count[sap_index] > 0) {
		sap_count[sap_index]--;
	}
	if (stack.ptr <= stack.offset[stack_depth]) {
		scl_security_throw(EX_STACK_UNDERFLOW, "Stack underflow!");
	}
	stack.ptr--;
	long long value = (long long) stack.data[stack.ptr].ptr;
	return value;
}

double ctrl_pop_double() {
	if (sap_enabled[sap_index] && sap_count[sap_index] > 0) {
		sap_count[sap_index]--;
	}
	if (stack.ptr <= stack.offset[stack_depth]) {
		scl_security_throw(EX_STACK_UNDERFLOW, "Stack underflow!");
	}
	stack.ptr--;
	double value = stack.data[stack.ptr].floating;
	return value;
}

char* ctrl_pop_string() {
	if (sap_enabled[sap_index] && sap_count[sap_index] > 0) {
		sap_count[sap_index]--;
	}
	if (stack.ptr <= stack.offset[stack_depth]) {
		scl_security_throw(EX_STACK_UNDERFLOW, "Stack underflow!");
	}
	stack.ptr--;
	char* value = (char*) stack.data[stack.ptr].ptr;
	return value;
}

scl_value ctrl_pop() {
	if (sap_enabled[sap_index] && sap_count[sap_index] > 0) {
		sap_count[sap_index]--;
	}
	if (stack.ptr <= stack.offset[stack_depth]) {
		scl_security_throw(EX_STACK_UNDERFLOW, "Stack underflow!");
	}
	stack.ptr--;
	return stack.data[stack.ptr].ptr;
}

ssize_t ctrl_stack_size(void) {
	return stack.ptr - stack.offset[stack_depth];
}

#pragma endregion

#pragma region Complex

int scl_is_complex(void* p) {
	for (size_t i = 0; i < alloced_complexes_count; i++) {
		if (p != 0 && alloced_complexes[i] == p) {
			return 1;
		}
	}
	return 0;
}

scl_value scl_alloc_complex(size_t size) {
	scl_value ptr = malloc(size);
	alloced_complexes[alloced_complexes_count++] = ptr;
	return ptr;
}

void scl_dealloc_complex(scl_value ptr) {
	for (size_t i = 0; i < alloced_complexes_count; i++) {
		if (alloced_complexes[i] == ptr) {
			alloced_complexes[i] = 0;
		}
	}
}

#pragma endregion

#pragma region Operators

operator(add) {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a + b);
}

operator(sub) {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a - b);
}

operator(mul) {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a * b);
}

operator(div) {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	if (b == 0) {
		scl_security_throw(EX_INVALID_ARGUMENT, "Division by zero!");
	}
	ctrl_push_long(a / b);
}

operator(mod) {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	if (b == 0) {
		scl_security_throw(EX_INVALID_ARGUMENT, "Division by zero!");
	}
	ctrl_push_long(a % b);
}

operator(land) {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a & b);
}

operator(lor) {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a | b);
}

operator(lxor) {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a ^ b);
}

operator(lnot) {
	int64_t a = ctrl_pop_long();
	ctrl_push_long(~a);
}

operator(lsh) {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a << b);
}

operator(rsh) {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a >> b);
}

operator(pow) {
	long long exp = ctrl_pop_long();
	if (exp < 0) {
		scl_security_throw(EX_BAD_PTR, "Negative exponent!");
	}
	int64_t base = ctrl_pop_long();
	long long intResult = (int64_t) base;
	long long i = 0;
	while (i < exp) {
		intResult *= (int64_t) base;
		i++;
	}
	ctrl_push_long(intResult);
}

operator(dadd) {
	double n2 = ctrl_pop_double();
	double n1 = ctrl_pop_double();
	ctrl_push_double(n1 + n2);
}

operator(dsub) {
	double n2 = ctrl_pop_double();
	double n1 = ctrl_pop_double();
	ctrl_push_double(n1 - n2);
}

operator(dmul) {
	double n2 = ctrl_pop_double();
	double n1 = ctrl_pop_double();
	ctrl_push_double(n1 * n2);
}

operator(ddiv) {
	double n2 = ctrl_pop_double();
	double n1 = ctrl_pop_double();
	ctrl_push_double(n1 / n2);
}

#pragma endregion
