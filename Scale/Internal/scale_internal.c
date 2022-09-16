#include "scale_internal.h"

/* Variables */
size_t		 memalloced_ptr[STACK_SIZE] = {0};
size_t 		 stack_depth = 0;
scl_memory_t memalloced[STACK_SIZE][MALLOC_LIMIT] = {{0}};
scl_stack_t	 callstk = {0, {0}, {0}};
scl_stack_t  stack = {0, {0}, {0}};
char* 		 current_file = "<init>";
size_t 		 current_line = 0;
size_t 		 current_column = 0;
size_t 		 sap_enabled[STACK_SIZE] = {0};
size_t 		 sap_count[STACK_SIZE] = {0};
size_t 		 sap_index = 0;

#pragma region Security

scl_force_inline void throw(int code, char* msg) {
	if (msg) fprintf(stderr, "Exception: %s\n", msg);
	process_signal(code);
}

scl_force_inline void ctrl_required(ssize_t n, char* func) {
	if (stack.ptr < n) {
		char* err = (char*) malloc(MAX_STRING_SIZE);
		sprintf(err, "Error: Function %s requires %zu arguments, but only %zu are provided.", func, n, stack.ptr);
		throw(EX_STACK_UNDERFLOW, err);
		free(err);
	}
}

void safe_exit(int code) {
	exit(code);
}

scl_force_inline void scl_security_check_null(scl_word ptr) {
	if (ptr == NULL) {
		throw(EX_BAD_PTR, "Null pointer");
	}
}

#pragma endregion

#pragma region Memory Management/GC

void heap_collect() {
	int count = 0;
	int collect = 1;
	for (size_t i = 0; i < memalloced_ptr[stack_depth + 1]; i++) {
		if (memalloced[stack_depth + 1][i].ptr) {
			if (memalloced[stack_depth + 1][i].level > callstk.ptr) {
				for (ssize_t j = (ssize_t) (stack.ptr - 1); j >= 0; j--) {
					if (stack.data[j] == memalloced[stack_depth + 1][i].ptr) {
						collect = 0;
						break;
					}
				}
				if (collect) {
					if (memalloced[stack_depth + 1][i].isFile) {
						fclose(memalloced[stack_depth + 1][i].ptr);
					} else {
						free(memalloced[stack_depth + 1][i].ptr);
					}
					memalloced[stack_depth + 1][i].ptr = NULL;
					count++;
				}
			}
		}
	}
}

void heap_add(scl_word ptr, int isFile) {
	for (size_t i = 0; i < memalloced_ptr[stack_depth]; i++) {
		if (memalloced[stack_depth][i].ptr == NULL) {
			memalloced[stack_depth][i].ptr = ptr;
			memalloced[stack_depth][i].isFile = isFile;
			memalloced[stack_depth][i].level = callstk.ptr;
			return;
		}
	}
	memalloced[stack_depth][memalloced_ptr[stack_depth]].ptr = ptr;
	memalloced[stack_depth][memalloced_ptr[stack_depth]].isFile = isFile;
	memalloced[stack_depth][memalloced_ptr[stack_depth]].level = callstk.ptr;
	memalloced_ptr[stack_depth]++;
}

int heap_is_alloced(scl_word ptr) {
	for (size_t i = 0; i < memalloced_ptr[stack_depth]; i++) {
		if (memalloced[stack_depth][i].ptr == ptr) {
			return 1;
		}
	}
	return 0;
}

void heap_remove(scl_word ptr) {
	size_t i;
	int found = 0;
	for (i = 0; i < memalloced_ptr[stack_depth]; i++) {
		if (memalloced[stack_depth][i].ptr == ptr) {
			memalloced[stack_depth][i].ptr = NULL;
			found = 1;
			break;
		}
	}
	
	if (!found) {
		return;
	}
	for (size_t j = i; j < memalloced_ptr[stack_depth] - 1; j++) {
		memalloced[stack_depth][j] = memalloced[stack_depth][j + 1];
	}
	memalloced_ptr[stack_depth]--;
}

#pragma endregion

#pragma region Function Management

scl_force_inline void ctrl_fn_start(char* name) {
	callstk.data[callstk.ptr++] = name;
	stack_depth++;
	stack.offset[stack_depth] = stack.ptr;
}

scl_force_inline void ctrl_fn_end() {
	callstk.ptr--;
	stack.offset[stack_depth] = 0;
	stack_depth--;
	heap_collect();
}

scl_force_inline void ctrl_fn_end_with_return() {
	scl_word ret = ctrl_pop();
	stack.offset[stack_depth] = 0;
	callstk.ptr--;
	stack_depth--;
	ctrl_push(ret);
	heap_collect();
}

scl_force_inline void sap_open(void) {
	sap_index++;
	if (sap_index >= STACK_SIZE) {
		throw(EX_SAP_ERROR, "Exhaustive use of SAP");
	}
	sap_enabled[sap_index] = 1;
	sap_count[sap_index] = 0;
}

scl_force_inline void sap_close(void) {
	if (sap_index == 0) {
		throw(EX_SAP_ERROR, "No SAP open");
	}
	sap_enabled[sap_index] = 0;
	for (size_t i = 0; i < sap_count[sap_index]; i++) {
		ctrl_pop();
	}
	sap_index--;
}

scl_force_inline void ctrl_fn_native_start(char* name) {
	callstk.data[callstk.ptr++] = name;
}

scl_force_inline void ctrl_fn_native_end() {
	callstk.ptr--;
}

scl_force_inline void ctrl_fn_nps_start(char* name) {
	callstk.data[callstk.ptr++] = name;
}

scl_force_inline void ctrl_fn_nps_end() {
	callstk.ptr--;
	heap_collect();
}

#pragma endregion

#pragma region Exceptions

scl_force_inline void ctrl_where(char* file, size_t line, size_t col) {
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

	safe_exit(sig_num);
}

#pragma endregion

#pragma region Stack Operations

scl_force_inline void ctrl_push_string(const char* c) {
	if (sap_enabled[sap_index]) {
		sap_count[sap_index]++;
	}
	if (stack.ptr + 1 >= STACK_SIZE) {
		throw(EX_STACK_OVERFLOW, "Stack overflow!");
	}
	stack.data[stack.ptr++] = (scl_word) c;
}

scl_force_inline void ctrl_push_double(double d) {
	if (sap_enabled[sap_index]) {
		sap_count[sap_index]++;
	}
	if (stack.ptr + 1 >= STACK_SIZE) {
		throw(EX_STACK_OVERFLOW, "Stack overflow!");
	}
	stack.data[stack.ptr++] = *(scl_word*) &d;
}

scl_force_inline void ctrl_push_long(long long n) {
	if (sap_enabled[sap_index]) {
		sap_count[sap_index]++;
	}
	if (stack.ptr + 1 >= STACK_SIZE) {
		throw(EX_STACK_OVERFLOW, "Stack overflow!");
	}
	stack.data[stack.ptr] = (scl_word) n;
	stack.ptr++;
}

scl_force_inline long long ctrl_pop_long() {
	if (sap_enabled[sap_index]) {
		sap_count[sap_index]--;
	}
	if (stack.ptr <= stack.offset[stack_depth]) {
		throw(EX_STACK_UNDERFLOW, "Stack underflow!");
	}
	long long value = (long long) stack.data[--stack.ptr];
	stack.data[stack.ptr + 1] = NULL;
	return value;
}

scl_force_inline double ctrl_pop_double() {
	if (sap_enabled[sap_index]) {
		sap_count[sap_index]--;
	}
	if (stack.ptr <= stack.offset[stack_depth]) {
		throw(EX_STACK_UNDERFLOW, "Stack underflow!");
	}
	double value = *(double*) &stack.data[--stack.ptr];
	stack.data[stack.ptr + 1] = NULL;
	return value;
}

scl_force_inline void ctrl_push(scl_word n) {
	if (sap_enabled[sap_index]) {
		sap_count[sap_index]++;
	}
	if (stack.ptr + 1 >= STACK_SIZE) {
		throw(EX_STACK_OVERFLOW, "Stack overflow!");
	}
	stack.data[stack.ptr++] = n;
}

scl_force_inline char* ctrl_pop_string() {
	if (sap_enabled[sap_index]) {
		sap_count[sap_index]--;
	}
	if (stack.ptr <= stack.offset[stack_depth]) {
		throw(EX_STACK_UNDERFLOW, "Stack underflow!");
	}
	char* value = (char*) stack.data[--stack.ptr];
	stack.data[stack.ptr + 1] = NULL;
	return value;
}

scl_force_inline scl_word ctrl_pop() {
	if (sap_enabled[sap_index]) {
		sap_count[sap_index]--;
	}
	if (stack.ptr <= stack.offset[stack_depth]) {
		throw(EX_STACK_UNDERFLOW, "Stack underflow!");
	}
	void* value = stack.data[--stack.ptr];
	stack.data[stack.ptr + 1] = NULL;
	return value;
}

scl_force_inline scl_word ctrl_pop_word() {
	if (sap_enabled[sap_index]) {
		sap_count[sap_index]--;
	}
	if (stack.ptr <= stack.offset[stack_depth]) {
		throw(EX_STACK_UNDERFLOW, "Stack underflow!");
	}
	void* value = stack.data[--stack.ptr];
	stack.data[stack.ptr + 1] = NULL;
	return value;
}

scl_force_inline void ctrl_push_word(scl_word n) {
	if (sap_enabled[sap_index]) {
		sap_count[sap_index]++;
	}
	if (stack.ptr + 1 >= STACK_SIZE) {
		throw(EX_STACK_OVERFLOW, "Stack overflow!");
	}
	stack.data[stack.ptr++] = n;
}

scl_force_inline ssize_t ctrl_stack_size(void) {
	return stack.ptr - stack.offset[stack_depth];
}

#pragma endregion

#pragma region Operators

scl_force_inline void op_add() {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a + b);
}

scl_force_inline void op_sub() {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a - b);
}

scl_force_inline void op_mul() {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a * b);
}

scl_force_inline void op_div() {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	if (b == 0) {
		throw(EX_INVALID_ARGUMENT, "Division by zero!");
	}
	ctrl_push_long(a / b);
}

scl_force_inline void op_mod() {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	if (b == 0) {
		throw(EX_INVALID_ARGUMENT, "Division by zero!");
	}
	ctrl_push_long(a % b);
}

scl_force_inline void op_land() {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a & b);
}

scl_force_inline void op_lor() {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a | b);
}

scl_force_inline void op_lxor() {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a ^ b);
}

scl_force_inline void op_lnot() {
	int64_t a = ctrl_pop_long();
	ctrl_push_long(~a);
}

scl_force_inline void op_lsh() {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a << b);
}

scl_force_inline void op_rsh() {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a >> b);
}

scl_force_inline void op_pow() {
	long long exp = ctrl_pop_long();
	if (exp < 0) {
		throw(EX_BAD_PTR, "Negative exponent!");
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

scl_force_inline void op_dadd() {
	double n2 = ctrl_pop_double();
	double n1 = ctrl_pop_double();
	ctrl_push_double(n1 + n2);
}

scl_force_inline void op_dsub() {
	double n2 = ctrl_pop_double();
	double n1 = ctrl_pop_double();
	ctrl_push_double(n1 - n2);
}

scl_force_inline void op_dmul() {
	double n2 = ctrl_pop_double();
	double n1 = ctrl_pop_double();
	ctrl_push_double(n1 * n2);
}

scl_force_inline void op_ddiv() {
	double n2 = ctrl_pop_double();
	double n1 = ctrl_pop_double();
	ctrl_push_double(n1 / n2);
}

#pragma endregion
