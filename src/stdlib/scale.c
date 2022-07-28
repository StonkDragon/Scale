#ifndef SCALE
#define SCALE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
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
#else
#include <unistd.h>
#define sleep(s) do { struct timespec __ts = {((s) / 1000), ((s) % 1000) * 1000000}; nanosleep(&__ts, NULL); } while (0)
#endif

#include "../../Scale/comp/scale.h"

#define STACK_SIZE 65536
#define MAX_STRING_SIZE STACK_SIZE
#define LONG_AS_STR_LEN 22

// define scale-specific signals
#define EX_BAD_PTR 128
#define EX_STACK_OVERFLOW 129
#define EX_STACK_UNDERFLOW 130
#define EX_UNHANDLED_DATA 131
#define EX_IO_ERROR 132
#define EX_INVALID_ARGUMENT 133

typedef struct {
	unsigned int ptr;
	void* data[STACK_SIZE];
} scale_stack;

scale_stack stack = {0, {}};
scale_stack trace = {0, {}};

int __SCALE_ARG_SIZE;

void stacktrace_push(char* ptr) {
	trace.data[trace.ptr++] = ptr;
}

void stacktrace_pop() {
	trace.ptr--;
}

void stacktrace_print() {
	printf("Stacktrace:\n");
	for (int i = trace.ptr - 1; i >= 0; i--) {
		char* frame = (char*) trace.data[i];
		printf("    %s\n", frame);
	}
	printf("\n");
}

void process_signal(int sig_num)
{
	char* signalString;
	if (sig_num == SIGABRT) signalString = "abort() called";
	else if (sig_num == SIGFPE) signalString = "Floating point exception";
	else if (sig_num == SIGILL) signalString = "Illegal instruction";
	else if (sig_num == SIGINT) signalString = "Software Interrupt (^C)";
	else if (sig_num == SIGSEGV) signalString = "Invalid/Illegal Memory Access";

	else if (sig_num == EX_BAD_PTR) signalString = "Bad pointer";
	else if (sig_num == EX_STACK_OVERFLOW) signalString = "Stack overflow";
	else if (sig_num == EX_STACK_UNDERFLOW) signalString = "Stack underflow";
	else if (sig_num == EX_IO_ERROR) signalString = "IO error";
	else if (sig_num == EX_INVALID_ARGUMENT) signalString = "Invalid argument";
	else if (sig_num == EX_UNHANDLED_DATA) {
		if (stack.ptr == 1) {
			signalString = (char*) malloc(strlen("Unhandled data on the stack. %d element too much!") + LONG_AS_STR_LEN);
			sprintf(signalString, "Unhandled data on the stack. %d element too much!", stack.ptr);
		} else {
			signalString = (char*) malloc(strlen("Unhandled data on the stack. %d elements too much!") + LONG_AS_STR_LEN);
			sprintf(signalString, "Unhandled data on the stack. %d elements too much!", stack.ptr);
		}
	}
	else signalString = "Unknown signal";

	printf("\n%s\n\n", signalString);

	if (trace.ptr < 1) {
		printf("The crash happened in native code, and a stack trace is not available!\n\n");
		exit(sig_num);
	}
	stacktrace_print();
	exit(sig_num);
}

void scale_extern_trace() {
	stacktrace_print();
}

void scale_push_string(const char* c) {
	if (stack.ptr + 1 > STACK_SIZE) {
		scale_push_long(EX_STACK_OVERFLOW);
		scale_extern_raise();
	}
	stack.data[stack.ptr++] = (char*) c;
}

#if __SIZEOF_POINTER__ >= 8
void scale_push_long(long long n) {
	if (stack.ptr + 1 > STACK_SIZE) {
		scale_push_long(EX_STACK_OVERFLOW);
		scale_extern_raise();
	}
	stack.data[stack.ptr++] = (void*) n;
}
#else
#error "Pointer size is not supported"
#endif

void scale_push_double(double n) {
	scale_push_long(*(long long*) &n);
}

#if __SIZEOF_POINTER__ >= 8
long long scale_pop_long() {
	if (stack.ptr <= 0) {
		scale_push_long(EX_STACK_UNDERFLOW);
		scale_extern_raise();
	}
	void* n = stack.data[--stack.ptr];
	return (long long) n;
}
#else
#error "Pointer size is not supported"
#endif

double scale_pop_double() {
	long long n = scale_pop_long();
	double d = *(double*) &n;
	return d;
}

#if __SIZEOF_POINTER__ >= 8
void scale_push(void* n) {
	if (stack.ptr + 1 > STACK_SIZE) {
		scale_push_long(EX_STACK_OVERFLOW);
		scale_extern_raise();
	}
	stack.data[stack.ptr++] = n;
}
#else
#error "Pointer size is not supported"
#endif

char* scale_pop_string() {
	if (stack.ptr <= 0) {
		scale_push_long(EX_STACK_UNDERFLOW);
		scale_extern_raise();
	}
	char *c = (char*) stack.data[--stack.ptr];
	return c;
}

void* scale_pop() {
	if (stack.ptr <= 0) {
		scale_push_long(EX_STACK_UNDERFLOW);
		scale_extern_raise();
	}
	void* v = stack.data[--stack.ptr];
	return v;
}

void scale_op_add() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_long(a + b);
}

void scale_op_sub() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_long(a - b);
}

void scale_op_mul() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_long(a * b);
}

void scale_op_div() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_long(a / b);
}

void scale_op_mod() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_long(a % b);
}

void scale_op_land() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_long(a & b);
}

void scale_op_lor() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_long(a | b);
}

void scale_op_lxor() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_long(a ^ b);
}

void scale_op_lnot() {
	long long a = scale_pop_long();
	scale_push_long(~a);
}

void scale_op_lsh() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_long(a << b);
}

void scale_op_rsh() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_long(a >> b);
}

void scale_op_pow() {
	long long exp = scale_pop_long();
	if (exp < 0) {
		scale_push_long(EX_INVALID_ARGUMENT);
		scale_extern_raise();
	}
	long long base = scale_pop_long();
	long long result = base;
	long long i = 0;
	while (i < exp) {
		result *= base;
		i++;
	}
	scale_push_long(result);
}

void scale_extern_dumpstack() {
	printf("Dump:\n");
	for (int i = stack.ptr - 1; i >= 0; i--) {
		void* v = stack.data[i];
		printf("    %p\n", v);
	}
	printf("\n");
}

void scale_extern_exit() {
	long long n = scale_pop_long();
	exit(n);
}

void scale_extern_sleep() {
	long long c = scale_pop_long();
	sleep(c);
}

void scale_extern_getproperty() {
	char *c = scale_pop_string();
	char *prop = getenv(c);
	scale_push_string(prop);
}

void scale_extern_less() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_long(a < b);
}

void scale_extern_more() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_long(a > b);
}

void scale_extern_equal() {
	long long a = scale_pop_long();
	long long b = scale_pop_long();
	scale_push_long(a == b);
}

void scale_extern_dup() {
	void* c = scale_pop();
	scale_push(c);
	scale_push(c);
}

void scale_extern_over() {
	void *a = scale_pop();
	void *b = scale_pop();
	void *c = scale_pop();
	scale_push(a);
	scale_push(b);
	scale_push(c);
}

void scale_extern_swap() {
	void *a = scale_pop();
	void *b = scale_pop();
	scale_push(a);
	scale_push(b);
}

void scale_extern_drop() {
	scale_pop();
}

void scale_extern_sizeof_stack() {
	scale_push_long(stack.ptr);
}

void scale_extern_concat() {
	char *s2 = scale_pop_string();
	char *s1 = scale_pop_string();
	scale_push_string(s1);
	scale_extern_strlen();
	long long len = scale_pop_long();
	scale_push_string(s2);
	scale_extern_strlen();
	long long len2 = scale_pop_long();
	char *out = (char*) malloc(len + len2 + 1);
	int i = 0;
	while (s1[i] != '\0') {
		out[i] = s1[i];
		i++;
	}
	int j = 0;
	while (s2[j] != '\0') {
		out[i + j] = s2[j];
		j++;
	}
	scale_push_string(out);
	free(out);
}

void scale_extern_dadd() {
	double b = scale_pop_double();
	double a = scale_pop_double();
	scale_push_double(a + b);
}

void scale_extern_dsub() {
	double b = scale_pop_double();
	double a = scale_pop_double();
	scale_push_double(a - b);
}

void scale_extern_dmul() {
	double b = scale_pop_double();
	double a = scale_pop_double();
	scale_push_double(a * b);
}

void scale_extern_ddiv() {
	double b = scale_pop_double();
	double a = scale_pop_double();
	scale_push_double(a / b);
}

void scale_extern_dtoi() {
	double d = scale_pop_double();
	scale_push_long((int) d);
}

void scale_extern_itod() {
	long long i = scale_pop_long();
	scale_push_double((double) i);
}

void scale_extern_random() {
	scale_push_long(rand());
}

void scale_extern_crash() {
	exit(1);
}

void scale_extern_and() {
	int a = scale_pop_long();
	int b = scale_pop_long();
	scale_push_long(a && b);
}

void scale_extern_eval() {
	char *cmd = scale_pop_string();
	int ret = system(cmd);
	scale_push_long(ret);
}

void scale_extern_getstack() {
	int i = scale_pop_long();
	scale_push(stack.data[i]);
}

void scale_extern_not() {
	scale_push_long(!scale_pop_long());
}

void scale_extern_or() {
	int a = scale_pop_long();
	int b = scale_pop_long();
	scale_push_long(a || b);
}

void scale_extern_lor() {
	int a = scale_pop_long();
	int b = scale_pop_long();
	scale_push_long(a | b);
}

void scale_extern_land() {
	int a = scale_pop_long();
	int b = scale_pop_long();
	scale_push_long(a & b);
}

void scale_extern_xor() {
	int a = scale_pop_long();
	int b = scale_pop_long();
	scale_push_long(a ^ b);
}

void scale_extern_lnot() {
	int a = scale_pop_long();
	scale_push_long(~a);
}

void scale_extern_sprintf() {
	char *fmt = scale_pop_string();
	void* s = scale_pop();
	char *out = (char*) malloc(LONG_AS_STR_LEN + strlen(fmt) + 1);
	sprintf(out, fmt, s);
	scale_push_string(out);
	free(out);
}

void scale_extern_strlen() {
	char *s = scale_pop_string();
	size_t len = 0;
	while (s[len] != '\0') {
		len++;
	}
	scale_push_long(len);
}

void scale_extern_strcmp() {
	char *s1 = scale_pop_string();
	char *s2 = scale_pop_string();
	int i = 0;
	while (s1[i] != '\0' && s2[i] != '\0') {
		if (s1[i] != s2[i]) {
			scale_push_long(0);
			return;
		}
		i++;
	}
	scale_push_long(1);
}

void scale_extern_strncmp() {
	char *s1 = scale_pop_string();
	char *s2 = scale_pop_string();
	long long n = scale_pop_long();
	int i = 0;
	while (s1[i] != '\0' && s2[i] != '\0' && i < n) {
		if (s1[i] != s2[i]) {
			scale_push_long(0);
			return;
		}
		i++;
	}
	scale_push_long(1);
}

void scale_extern_fopen() {
	char *mode = scale_pop_string();
	char *s = scale_pop_string();
	FILE *f = fopen(s, mode);
	if (f == NULL) {
		fprintf(stderr, "Error opening file '%s'\n", s);
		fprintf(stderr, "Error: %s\n", strerror(errno));
	}
	scale_push((void*) f);
}

void scale_extern_fclose() {
	FILE *f = (FILE*) scale_pop();
	fclose(f);
}

void scale_extern_fseek() {
	long long offset = scale_pop_long();
	int whence = scale_pop_long();
	FILE *f = (FILE*) scale_pop();
	fseek(f, offset, whence);
}

void scale_extern_ftell() {
	FILE *f = (FILE*) scale_pop();
	scale_push((void*) f);
	scale_push_long(ftell(f));
}

void scale_extern_fileno() {
	FILE *f = (FILE*) scale_pop();
	scale_push_long(fileno(f));
}

void scale_extern_raise() {
	long long n = scale_pop_long();
	if (n != 2 && n != 4 && n != 6 && n != 8 && n != 11) {
		int raised = raise(n);
		if (raised != 0) {
			process_signal(n);
		}
	} else {
		process_signal(n);
	}
}

void scale_extern_abort() {
	abort();
}

void scale_extern_write() {
	void *s = scale_pop();
	long long n = scale_pop_long();
	long long fd = scale_pop_long();
	write(fd, s, n);
}

void scale_extern_read() {
	long long n = scale_pop_long();
	long long fd = scale_pop_long();
	void *s = malloc(n);
	int ret = read(fd, s, n);
	if (ret == -1) {
		scale_push_long(EX_IO_ERROR);
		scale_extern_raise();
	}
	scale_push(s);
}

void scale_extern_strrev() {
	char* s = scale_pop_string();
	int i = 0;
	scale_push_string(s);
	scale_extern_strlen();
	long long len = scale_pop_long();
	char* out = (char*) malloc(len + 1);
	for (i = len - 1; i >= 0; i--) {
		out[i] = s[i];
	}
	out[len] = '\0';
	scale_push_string(out);
}

void scale_extern_malloc() {
	long long n = scale_pop_long();
	void* s = malloc(n);
	scale_push(s);
}

void scale_extern_free() {
	void* s = scale_pop();
	free(s);
}

void scale_func_main();

int main(int argc, char const *argv[])
{
	for (int i = argc - 1; i > 0; i--) {
		scale_push_string(argv[i]);
	}

	signal(SIGINT, process_signal);
	signal(SIGILL, process_signal);
	signal(SIGABRT, process_signal);
	signal(SIGFPE, process_signal);
	signal(SIGSEGV, process_signal);

	scale_func_main();

	if (stack.ptr > 0) {
		scale_push_long(EX_UNHANDLED_DATA);
		scale_extern_raise();
	}
	return 0;
}

#endif
