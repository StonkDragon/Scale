#ifndef SCALE_H
#define SCALE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#define sleep(s) Sleep(s)
#define read(fd, buf, n) _read(fd, buf, n)
#define write(fd, buf, n) _write(fd, buf, n)
#ifndef WINDOWS
#define WINDOWS
#endif
#else
#include <unistd.h>
#define sleep(s) do { struct timespec __ts = {((s) / 1000), ((s) % 1000) * 1000000}; nanosleep(&__ts, NULL); } while (0)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "scale.h"

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
	fprintf(stderr, "Exception: %s\n", msg);
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

void ctrl_trace() {
	size_t i;
	printf("Stacktrace:\n");
	for (i = (callstk.ptr - 1); i >= 0; i--) {
		printf("  %s\n", (char*) callstk.data[i]);
	}
}

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

#pragma region Natives

scl_native void native_dumpstack() {
	printf("Dump:\n");
	ssize_t stack_offset = stack.offset[stack_depth];
	for (ssize_t i = stack.ptr - 1; i >= stack_offset; i--) {
		long long v = (long long) stack.data[i];
		printf("   %zd: 0x%016llx, %lld\n", i, v, v);
	}
	printf("\n");
}

scl_native void native_exit() {
	long long n = ctrl_pop_long();
	safe_exit(n);
}

scl_native void native_sleep() {
	long long c = ctrl_pop_long();
	sleep(c);
}

scl_native void native_getenv() {
	char *c = ctrl_pop_string();
	char *prop = getenv(c);
	ctrl_push_string(prop);
}

scl_native void native_less() {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a < b);
}

scl_native void native_more() {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a > b);
}

scl_native void native_equal() {
	int64_t a = ctrl_pop_long();
	int64_t b = ctrl_pop_long();
	ctrl_push_long(a == b);
}

scl_native void native_dup() {
	scl_word c = ctrl_pop();
	ctrl_push(c);
	ctrl_push(c);
}

scl_native void native_over() {
	void *a = ctrl_pop();
	void *b = ctrl_pop();
	void *c = ctrl_pop();
	ctrl_push(a);
	ctrl_push(b);
	ctrl_push(c);
}

scl_native void native_swap() {
	void *a = ctrl_pop();
	void *b = ctrl_pop();
	ctrl_push(a);
	ctrl_push(b);
}

scl_native void native_drop() {
	ctrl_pop();
}

scl_native void native_sizeof_stack() {
	ctrl_push_long(stack.ptr - stack.offset[stack_depth]);
}

scl_native void native_concat() {
	char *s2 = ctrl_pop_string();
	char *s1 = ctrl_pop_string();
	ctrl_push_string(s1);
	native_strlen();
	long long len = ctrl_pop_long();
	ctrl_push_string(s2);
	native_strlen();
	long long len2 = ctrl_pop_long();
	char *out = (char*) malloc(len + len2 + 1);
	heap_add(out, 0);
	size_t i = 0;
	while (s1[i] != '\0') {
		out[i] = s1[i];
		i++;
	}
	int j = 0;
	while (s2[j] != '\0') {
		out[i + j] = s2[j];
		j++;
	}
	ctrl_push_string(out);
}

scl_native void native_random() {
	ctrl_push_long(rand());
}

scl_native void native_crash() {
	safe_exit(1);
}

scl_native void native_and() {
	int a = ctrl_pop_long();
	int b = ctrl_pop_long();
	ctrl_push_long(a && b);
}

scl_native void native_system() {
	char *cmd = ctrl_pop_string();
	int ret = system(cmd);
	ctrl_push_long(ret);
}

scl_native void native_not() {
	ctrl_push_long(!ctrl_pop_long());
}

scl_native void native_or() {
	int a = ctrl_pop_long();
	int b = ctrl_pop_long();
	ctrl_push_long(a || b);
}

scl_native void native_sprintf() {
	char *fmt = ctrl_pop_string();
	scl_word s = ctrl_pop();
	char *out = (char*) malloc(LONG_AS_STR_LEN + strlen(fmt) + 1);
	heap_add(out, 0);
	sprintf(out, fmt, s);
	ctrl_push_string(out);
}

scl_native void native_strlen() {
	char *s = ctrl_pop_string();
	size_t len = 0;
	while (s[len] != '\0') {
		len++;
	}
	ctrl_push_long(len);
}

scl_native void native_strcmp() {
	char *s1 = ctrl_pop_string();
	char *s2 = ctrl_pop_string();
	ctrl_push_long(strcmp(s1, s2) == 0);
}

scl_native void native_strncmp() {
	char *s1 = ctrl_pop_string();
	char *s2 = ctrl_pop_string();
	long long n = ctrl_pop_long();
	ctrl_push_long(strncmp(s1, s2, n) == 0);
}

scl_native void native_fopen() {
	char *mode = ctrl_pop_string();
	char *name = ctrl_pop_string();
	FILE *f = fopen(name, mode);
	if (f == NULL) {
		char* err = malloc(strlen("Unable to open file '%s'") + strlen(name) + 1);
		sprintf(err, "Unable to open file '%s'", name);
		throw(EX_IO_ERROR, err);
	}
	heap_add(f, 1);
	ctrl_push((scl_word) f);
}

scl_native void native_fclose() {
	FILE *f = (FILE*) ctrl_pop();
	heap_remove(f);
	fclose(f);
}

scl_native void native_fseek() {
	long long offset = ctrl_pop_long();
	int whence = ctrl_pop_long();
	FILE *f = (FILE*) ctrl_pop();
	fseek(f, offset, whence);
}

scl_native void native_ftell() {
	FILE *f = (FILE*) ctrl_pop();
	ctrl_push((scl_word) f);
	ctrl_push_long(ftell(f));
}

scl_native void native_fileno() {
	FILE *f = (FILE*) ctrl_pop();
	ctrl_push_long(fileno(f));
}

scl_native void native_raise() {
	long long n = ctrl_pop_long();
	if (n != 2 && n != 4 && n != 6 && n != 8 && n != 11) {
		int raised = raise(n);
		if (raised != 0) {
			process_signal(n);
		}
	} else {
		process_signal(n);
	}
}

scl_native void native_abort() {
	abort();
}

scl_native void native_write() {
	void *s = ctrl_pop();
	long long n = ctrl_pop_long();
	long long fd = ctrl_pop_long();
	write(fd, s, n);
	ctrl_push(s);
	native_free();
}

scl_native void native_read() {
	long long n = ctrl_pop_long();
	long long fd = ctrl_pop_long();
	void *s = malloc(n);
	heap_add(s, 0);
	int ret = read(fd, s, n);
	if (ret == -1) {
		ctrl_push_long(EX_IO_ERROR);
		native_raise();
	}
	ctrl_push(s);
}

scl_native void native_strrev() {
	char* s = ctrl_pop_string();
	size_t i = 0;
	ctrl_push_string(s);
	native_strlen();
	long long len = ctrl_pop_long();
	char* out = (char*) malloc(len + 1);
	heap_add(out, 0);
	for (i = len - 1; i >= 0; i--) {
		out[i] = s[i];
	}
	out[len] = '\0';
	ctrl_push_string(out);
}

scl_native void native_malloc() {
	long long n = ctrl_pop_long();
	scl_word s = malloc(n);
	heap_add(s, 0);
	ctrl_push(s);
}

scl_native void native_free() {
	scl_word s = ctrl_pop();
	int is_alloc = heap_is_alloced(s);
	heap_remove(s);
	if (is_alloc) {
		free(s);
	}
}

scl_native void native_breakpoint() {
	printf("Hit breakpoint. Press enter to continue.\n");
	getchar();
}

scl_native void native_memset() {
	scl_word s = ctrl_pop();
	long long n = ctrl_pop_long();
	long long c = ctrl_pop_long();
	memset(s, c, n);
}

scl_native void native_memcpy() {
	scl_word s2 = ctrl_pop();
	scl_word s1 = ctrl_pop();
	long long n = ctrl_pop_long();
	memcpy(s2, s1, n);
}

scl_native void native_time() {
	struct timespec t;
	clock_gettime(CLOCK_REALTIME, &t);
	long long millis = t.tv_sec * 1000 + t.tv_nsec / 1000000;
	ctrl_push_long(millis);
}

scl_native void native_heap_collect() {
	heap_collect();
}

scl_native void native_trace() {
	ctrl_trace();
}

scl_native void native_sqrt() {
    double n = ctrl_pop_double();
    ctrl_push_double(sqrt(n));
}

scl_native void native_sin() {
	double n = ctrl_pop_double();
	ctrl_push_double(sin(n));
}

scl_native void native_cos() {
	double n = ctrl_pop_double();
	ctrl_push_double(cos(n));
}

scl_native void native_tan() {
	double n = ctrl_pop_double();
	ctrl_push_double(tan(n));
}

scl_native void native_asin() {
	double n = ctrl_pop_double();
	ctrl_push_double(asin(n));
}

scl_native void native_acos() {
	double n = ctrl_pop_double();
	ctrl_push_double(acos(n));
}

scl_native void native_atan() {
	double n = ctrl_pop_double();
	ctrl_push_double(atan(n));
}

scl_native void native_atan2() {
	double n2 = ctrl_pop_double();
	double n1 = ctrl_pop_double();
	ctrl_push_double(atan2(n1, n2));
}

scl_native void native_sinh() {
	double n = ctrl_pop_double();
	ctrl_push_double(sinh(n));
}

scl_native void native_cosh() {
	double n = ctrl_pop_double();
	ctrl_push_double(cosh(n));
}

scl_native void native_tanh() {
	double n = ctrl_pop_double();
	ctrl_push_double(tanh(n));
}

scl_native void native_asinh() {
	double n = ctrl_pop_double();
	ctrl_push_double(asinh(n));
}

scl_native void native_acosh() {
	double n = ctrl_pop_double();
	ctrl_push_double(acosh(n));
}

scl_native void native_atanh() {
	double n = ctrl_pop_double();
	ctrl_push_double(atanh(n));
}

scl_native void native_exp() {
	double n = ctrl_pop_double();
	ctrl_push_double(exp(n));
}

scl_native void native_log() {
	double n = ctrl_pop_double();
	ctrl_push_double(log(n));
}

scl_native void native_log10() {
	double n = ctrl_pop_double();
	ctrl_push_double(log10(n));
}

scl_native void native_longToString() {
	long long a = ctrl_pop_long();
	char *out = (char*) malloc(LONG_AS_STR_LEN + 1);
	heap_add(out, 0);
	sprintf(out, "%lld", a);
	ctrl_push_string(out);
}

scl_native void native_stringToLong() {
	char *s = ctrl_pop_string();
	long long a = atoll(s);
	ctrl_push_long(a);
}

scl_native void native_stringToDouble() {
	char *s = ctrl_pop_string();
	double a = atof(s);
	ctrl_push_double(a);
}

scl_native void native_doubleToString() {
	double a = ctrl_pop_double();
	char *out = (char*) malloc(LONG_AS_STR_LEN + 1);
	heap_add(out, 0);
	sprintf(out, "%f", a);
	ctrl_push_string(out);
}

#pragma endregion

#ifdef __cplusplus
}
#endif

#endif // SCALE_H
