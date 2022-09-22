#include "scale_internal.h"

/* Variables */
char* 		 current_file = "<init>";
size_t 		 current_line = 0;
size_t 		 current_column = 0;
size_t 		 stack_depth = 0;
scl_stack_t  stack = {0, {0}, {0}};
scl_stack_t	 callstk = {0, {0}, {0}};
size_t		 memalloced_ptr = 0;
scl_memory_t memalloced[MALLOC_LIMIT] = {0};
size_t 		 sap_index = 0;
size_t 		 sap_enabled[STACK_SIZE] = {0};
size_t 		 sap_count[STACK_SIZE] = {0};

#define UNIMPLEMENTED fprintf(stderr, "%s:%d: %s: Not Implemented\n", __FILE__, __LINE__, __FUNCTION__); exit(1)

#pragma region Security

void scl_security_throw(int code, char* msg) {
	if (msg) fprintf(stderr, "Exception: %s\n", msg);
	process_signal(code);
}

void scl_security_required_arg_count(ssize_t n, char* func) {
	if (stack.ptr < n) {
		char* err = (char*) malloc(MAX_STRING_SIZE);
		sprintf(err, "Error: Function %s requires %zu arguments, but only %zu are provided.", func, n, stack.ptr);
		scl_security_throw(EX_STACK_UNDERFLOW, err);
		free(err);
	}
}

void scl_security_safe_exit(int code) {
	exit(code);
}

void scl_security_check_null(scl_word ptr) {
	if (ptr == NULL) {
		scl_security_throw(EX_BAD_PTR, "Null pointer");
	}
}

#pragma endregion

#pragma region Memory Management/GC

void heap_collect() {
	for (size_t i = 0; i < memalloced_ptr; i++) {
		if (memalloced[i].ptr == 0)
			continue;
		
		if (memalloced[i].isFile) {
			fclose(memalloced[i].ptr);
		} else {
			free(memalloced[i].ptr);
		}
		memalloced[i].ptr = 0;
	}
}

void heap_add(scl_word ptr, int isFile) {
	for (size_t i = 0; i < memalloced_ptr; i++) {
		if (memalloced[i].ptr == NULL || memalloced[i].ptr == ptr) {
			memalloced[i].ptr = ptr;
			memalloced[i].isFile = isFile;
			memalloced[i].level = stack_depth;
			memalloced[i].returned = 0;
			return;
		}
	}
	memalloced[memalloced_ptr].ptr = ptr;
	memalloced[memalloced_ptr].isFile = isFile;
	memalloced[memalloced_ptr].level = stack_depth;
	memalloced[memalloced_ptr].returned = 0;
	memalloced_ptr++;
}

int heap_is_alloced(scl_word ptr) {
	for (size_t i = 0; i < memalloced_ptr; i++) {
		if (memalloced[i].ptr == ptr) {
			return 1;
		}
	}
	return 0;
}

void heap_remove(scl_word ptr) {
	for (size_t i = 0; i < memalloced_ptr; i++) {
		if (memalloced[i].ptr == ptr) {
			memalloced[i].ptr = 0;
			return;
		}
	}
}

#pragma endregion

#pragma region Function Management

void ctrl_fn_start(char* name) {
	callstk.data[callstk.ptr++] = name;
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

void ctrl_fn_native_start(char* name) {
	callstk.data[callstk.ptr++] = name;
}

void ctrl_fn_native_end() {
	callstk.ptr--;
}

void ctrl_fn_nps_start(char* name) {
	callstk.data[callstk.ptr++] = name;
}

void ctrl_fn_nps_end() {
	callstk.ptr--;
}

#pragma endregion

#pragma region Exceptions

void ctrl_where(char* file, size_t line, size_t col) {
	current_file = file;
	current_line = line;
	current_column = col;
}

void print_stacktrace() {
	printf("Stacktrace:\n");
	for (int i = callstk.ptr - 1; i >= 0; i--) {
		printf("  %s\n", (char*) callstk.data[i]);
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

	// Scale Exceptions
	else if (sig_num == EX_BAD_PTR) signalString = "Bad pointer";
	else if (sig_num == EX_STACK_OVERFLOW) signalString = "Stack overflow";
	else if (sig_num == EX_STACK_UNDERFLOW) signalString = "Stack underflow";
	else if (sig_num == EX_IO_ERROR) signalString = "IO error";
	else if (sig_num == EX_INVALID_ARGUMENT) signalString = "Invalid argument";
	else if (sig_num == EX_UNHANDLED_DATA) {
		if (stack.ptr == 1) {
			signalString = (char*) malloc(strlen("Unhandled data on the stack. %zu element too much!") + LONG_AS_STR_LEN);
			sprintf(signalString, "Unhandled data on the stack. %zu element too much!", stack.ptr);
		} else {
			signalString = (char*) malloc(strlen("Unhandled data on the stack. %zu elements too much!") + LONG_AS_STR_LEN);
			sprintf(signalString, "Unhandled data on the stack. %zu elements too much!", stack.ptr);
		}
	}
	else if (sig_num == EX_CAST_ERROR) signalString = "Cast Error";
	else if (sig_num == EX_SAP_ERROR) signalString = "SAP Error";
	else signalString = "Unknown signal";

	printf("\n");
	printf("%s:%zu:%zu: %s\n", current_file, current_line, current_column, signalString);
	if (errno) {
		printf("errno: %s\n", strerror(errno));
	}
	print_stacktrace();
	heap_collect();

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
	stack.data[stack.ptr++] = (scl_word) c;
}

void ctrl_push_double(double d) {
	if (sap_enabled[sap_index]) {
		sap_count[sap_index]++;
	}
	if (stack.ptr + 1 >= STACK_SIZE) {
		scl_security_throw(EX_STACK_OVERFLOW, "Stack overflow!");
	}
	stack.data[stack.ptr++] = *(scl_word*) &d;
}

void ctrl_push_long(long long n) {
	if (sap_enabled[sap_index]) {
		sap_count[sap_index]++;
	}
	if (stack.ptr + 1 >= STACK_SIZE) {
		scl_security_throw(EX_STACK_OVERFLOW, "Stack overflow!");
	}
	stack.data[stack.ptr] = (scl_word) n;
	stack.ptr++;
}

long long ctrl_pop_long() {
	if (sap_enabled[sap_index]) {
		sap_count[sap_index]--;
	}
	if (stack.ptr <= stack.offset[stack_depth]) {
		scl_security_throw(EX_STACK_UNDERFLOW, "Stack underflow!");
	}
	long long value = (long long) stack.data[--stack.ptr];
	stack.data[stack.ptr + 1] = NULL;
	return value;
}

double ctrl_pop_double() {
	if (sap_enabled[sap_index]) {
		sap_count[sap_index]--;
	}
	if (stack.ptr <= stack.offset[stack_depth]) {
		scl_security_throw(EX_STACK_UNDERFLOW, "Stack underflow!");
	}
	double value = *(double*) &stack.data[--stack.ptr];
	stack.data[stack.ptr + 1] = NULL;
	return value;
}

void ctrl_push(scl_word n) {
	if (sap_enabled[sap_index]) {
		sap_count[sap_index]++;
	}
	if (stack.ptr + 1 >= STACK_SIZE) {
		scl_security_throw(EX_STACK_OVERFLOW, "Stack overflow!");
	}
	stack.data[stack.ptr++] = n;
}

char* ctrl_pop_string() {
	if (sap_enabled[sap_index]) {
		sap_count[sap_index]--;
	}
	if (stack.ptr <= stack.offset[stack_depth]) {
		scl_security_throw(EX_STACK_UNDERFLOW, "Stack underflow!");
	}
	char* value = (char*) stack.data[--stack.ptr];
	stack.data[stack.ptr + 1] = NULL;
	return value;
}

scl_word ctrl_pop() {
	if (sap_enabled[sap_index]) {
		sap_count[sap_index]--;
	}
	if (stack.ptr <= stack.offset[stack_depth]) {
		scl_security_throw(EX_STACK_UNDERFLOW, "Stack underflow!");
	}
	void* value = stack.data[--stack.ptr];
	stack.data[stack.ptr + 1] = NULL;
	return value;
}

scl_word ctrl_pop_word() {
	if (sap_enabled[sap_index]) {
		sap_count[sap_index]--;
	}
	if (stack.ptr <= stack.offset[stack_depth]) {
		scl_security_throw(EX_STACK_UNDERFLOW, "Stack underflow!");
	}
	void* value = stack.data[--stack.ptr];
	stack.data[stack.ptr + 1] = NULL;
	return value;
}

void ctrl_push_word(scl_word n) {
	if (sap_enabled[sap_index]) {
		sap_count[sap_index]++;
	}
	if (stack.ptr + 1 >= STACK_SIZE) {
		scl_security_throw(EX_STACK_OVERFLOW, "Stack overflow!");
	}
	stack.data[stack.ptr++] = n;
}

ssize_t ctrl_stack_size(void) {
	return stack.ptr - stack.offset[stack_depth];
}

#pragma endregion

#pragma region Operators

void op_add() {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a + b);
}

void op_sub() {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a - b);
}

void op_mul() {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a * b);
}

void op_div() {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	if (b == 0) {
		scl_security_throw(EX_INVALID_ARGUMENT, "Division by zero!");
	}
	ctrl_push_long(a / b);
}

void op_mod() {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	if (b == 0) {
		scl_security_throw(EX_INVALID_ARGUMENT, "Division by zero!");
	}
	ctrl_push_long(a % b);
}

void op_land() {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a & b);
}

void op_lor() {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a | b);
}

void op_lxor() {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a ^ b);
}

void op_lnot() {
	int64_t a = ctrl_pop_long();
	ctrl_push_long(~a);
}

void op_lsh() {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a << b);
}

void op_rsh() {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a >> b);
}

void op_pow() {
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

void op_dadd() {
	double n2 = ctrl_pop_double();
	double n1 = ctrl_pop_double();
	ctrl_push_double(n1 + n2);
}

void op_dsub() {
	double n2 = ctrl_pop_double();
	double n1 = ctrl_pop_double();
	ctrl_push_double(n1 - n2);
}

void op_dmul() {
	double n2 = ctrl_pop_double();
	double n1 = ctrl_pop_double();
	ctrl_push_double(n1 * n2);
}

void op_ddiv() {
	double n2 = ctrl_pop_double();
	double n1 = ctrl_pop_double();
	ctrl_push_double(n1 / n2);
}

#pragma endregion
