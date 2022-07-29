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

#define STACK_PTR 0
#define STACK_STR 1
#define STACK_LONG 2
#define STACK_DOUBLE 3

// define scale-specific signals
#define EX_BAD_PTR 128
#define EX_STACK_OVERFLOW 129
#define EX_STACK_UNDERFLOW 130
#define EX_UNHANDLED_DATA 131
#define EX_IO_ERROR 132
#define EX_INVALID_ARGUMENT 133
#define EX_CAST_ERROR 134

typedef struct {
	unsigned int ptr;
	void* data[STACK_SIZE];
} scale_stack;

scale_stack stack = {0, {}};
scale_stack trace = {0, {}};

int __SCALE_ARG_SIZE;

typedef struct {
	void* ptr;
	unsigned int level;
	int isFile;
} memory_t;

memory_t memalloced[STACK_SIZE];
int memalloced_ptr = 0;
int current_function_level = 0;

void scale_extern_heap_collect() {
	scale_heap_collect();
}

void scale_heap_collect() {
	int i;
	int count = 0;
	int collect = 1;
	for (i = 0; i < memalloced_ptr; i++) {
		if (memalloced[i].ptr) {
			if (memalloced[i].level > current_function_level) {
				for (int j = (stack.ptr - 1); j >= 0; j--) {
					if (stack.data[j] == memalloced[i].ptr) {
						collect = 0;
						break;
					}
				}
				if (collect) {
					fprintf(stderr, "Warning: Collecting dangling poiner: %p\n", memalloced[i].ptr);
					if (memalloced[i].isFile) {
						fclose(memalloced[i].ptr);
					} else {
						free(memalloced[i].ptr);
					}
					memalloced[i].ptr = NULL;
					count++;
				}
			}
		}
	}
}

void scale_heap_collect_all(int print) {
	int i;
	for (i = 0; i < memalloced_ptr; i++) {
		if (memalloced[i].ptr) {
			if (print) fprintf(stderr, "Warning: Collecting dangling poiner: %p\n", memalloced[i].ptr);
			if (memalloced[i].isFile) {
				fclose(memalloced[i].ptr);
			} else {
				free(memalloced[i].ptr);
			}
			memalloced[i].ptr = NULL;
		}
	}
}

void scale_heap_add(void* ptr, int isFile) {
	for (int i = 0; i < memalloced_ptr; i++) {
		if (memalloced[i].ptr == NULL) {
			memalloced[i].ptr = ptr;
			memalloced[i].isFile = isFile;
			memalloced[i].level = current_function_level;
			return;
		}
	}
	memalloced[memalloced_ptr].ptr = ptr;
	memalloced[memalloced_ptr].isFile = isFile;
	memalloced[memalloced_ptr].level = current_function_level;
	memalloced_ptr++;
}

int scale_heap_is_alloced(void* ptr) {
	for (int i = 0; i < memalloced_ptr; i++) {
		if (memalloced[i].ptr == ptr) {
			return 1;
		}
	}
	return 0;
}

void scale_heap_remove(void* ptr, int isFile) {
	int i;
	for (i = 0; i < memalloced_ptr; i++) {
		if (memalloced[i].ptr == ptr) {
			memalloced[i].ptr = NULL;
			break;
		}
	}
	for (int j = i; j < memalloced_ptr - 1; j++) {
		memalloced[j] = memalloced[j + 1];
	}
	memalloced_ptr--;
}

void scale_safe_exit(int code) {
	scale_heap_collect_all(1);
	exit(code);
}

void stacktrace_push(char* ptr, int inc) {
	trace.data[trace.ptr++] = ptr;
	current_function_level += inc;
}

void stacktrace_pop(int dec) {
	trace.ptr--;
	current_function_level -= dec;
	if (dec) scale_heap_collect();
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
	else if (sig_num == EX_CAST_ERROR) signalString = "Cast Error";
	else signalString = "Unknown signal";

	printf("\n%s\n\n", signalString);

	if (trace.ptr < 1) {
		printf("The crash happened in native code, and a stack trace is not available!\n\n");
		scale_safe_exit(sig_num);
	}
	stacktrace_print();
	scale_safe_exit(sig_num);
}

void scale_extern_trace() {
	stacktrace_print();
}

void scale_push_string(const char* c) {
	if (stack.ptr + 1 > STACK_SIZE) {
		scale_push_long(EX_STACK_OVERFLOW);
		scale_extern_raise();
	}
	stack.data[stack.ptr] = (char*) c;
	stack.ptr++;
}

#if __SIZEOF_POINTER__ >= 8
void scale_push_long(long long n) {
	if (stack.ptr + 1 > STACK_SIZE) {
		scale_push_long(EX_STACK_OVERFLOW);
		scale_extern_raise();
	}
	stack.data[stack.ptr] = (void*) n;
	stack.ptr++;
}
#else
#error "Pointer size is not supported"
#endif

void scale_push_double(double n) {
	if (stack.ptr + 1 > STACK_SIZE) {
		scale_push_long(EX_STACK_OVERFLOW);
		scale_extern_raise();
	}
	stack.data[stack.ptr] = (void*) (*(long long*) &n);
	stack.ptr++;
}

#if __SIZEOF_POINTER__ >= 8
long long scale_pop_long() {
	if (stack.ptr <= 0) {
		scale_push_long(EX_STACK_UNDERFLOW);
		scale_extern_raise();
	}
	return *(long long*) &stack.data[--stack.ptr];
}
#else
#error "Pointer size is not supported"
#endif

double scale_pop_double() {
	if (stack.ptr <= 0) {
		scale_push_long(EX_STACK_UNDERFLOW);
		scale_extern_raise();
	}
	return *(double*) &stack.data[--stack.ptr];
}

#if __SIZEOF_POINTER__ >= 8
void scale_push(void* n) {
	if (stack.ptr + 1 > STACK_SIZE) {
		scale_push_long(EX_STACK_OVERFLOW);
		scale_extern_raise();
	}
	stack.data[stack.ptr] = n;
	stack.ptr++;
}
#else
#error "Pointer size is not supported"
#endif

char* scale_pop_string() {
	if (stack.ptr <= 0) {
		scale_push_long(EX_STACK_UNDERFLOW);
		scale_extern_raise();
	}
	return (char*) stack.data[--stack.ptr];
}

void* scale_pop() {
	if (stack.ptr <= 0) {
		scale_push_long(EX_STACK_UNDERFLOW);
		scale_extern_raise();
	}
	return stack.data[--stack.ptr];
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
	scale_safe_exit(n);
}

void scale_extern_sleep() {
	long long c = scale_pop_long();
	sleep(c);
}

void scale_extern_getenv() {
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
	scale_heap_add(out, 0);
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
	scale_safe_exit(1);
}

void scale_extern_and() {
	int a = scale_pop_long();
	int b = scale_pop_long();
	scale_push_long(a && b);
}

void scale_extern_system() {
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

void scale_extern_sprintf() {
	char *fmt = scale_pop_string();
	void* s = scale_pop();
	char *out = (char*) malloc(LONG_AS_STR_LEN + strlen(fmt) + 1);
	scale_heap_add(out, 0);
	sprintf(out, fmt, s);
	scale_push_string(out);
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
	scale_push_long(strcmp(s1, s2) == 0);
}

void scale_extern_strncmp() {
	char *s1 = scale_pop_string();
	char *s2 = scale_pop_string();
	long long n = scale_pop_long();
	scale_push_long(strncmp(s1, s2, n) == 0);
}

void scale_extern_fopen() {
	char *mode = scale_pop_string();
	char *name = scale_pop_string();
	FILE *f = fopen(name, mode);
	if (f == NULL) {
		fprintf(stderr, "Error opening file '%s'\n", name);
		fprintf(stderr, "Error: %s\n", strerror(errno));
	}
	scale_heap_add(f, 1);
	scale_push((void*) f);
}

void scale_extern_fclose() {
	FILE *f = (FILE*) scale_pop();
	scale_heap_remove(f, 1);
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
	scale_push(s);
	scale_extern_free();
}

void scale_extern_read() {
	long long n = scale_pop_long();
	long long fd = scale_pop_long();
	void *s = malloc(n);
	scale_heap_add(s, 0);
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
	scale_heap_add(out, 0);
	for (i = len - 1; i >= 0; i--) {
		out[i] = s[i];
	}
	out[len] = '\0';
	scale_push_string(out);
}

void scale_extern_malloc() {
	long long n = scale_pop_long();
	void* s = malloc(n);
	scale_heap_add(s, 0);
	scale_push(s);
}

void scale_extern_free() {
	void* s = scale_pop();
	int is_alloc = scale_heap_is_alloced(s);
	scale_heap_remove(s, 0);
	if (is_alloc) {
		free(s);
	}
}

void scale_extern_breakpoint() {
	printf("Hit breakpoint. Press enter to continue.\n");
	getchar();
}

void scale_extern_memset() {
	void* s = scale_pop();
	long long n = scale_pop_long();
	long long c = scale_pop_long();
	memset(s, c, n);
}

void scale_extern_memcpy() {
	void* s2 = scale_pop();
	void* s1 = scale_pop();
	long long n = scale_pop_long();
	memcpy(s2, s1, n);
}

void scale_extern_cast() {
	char* s = scale_pop_string();
	void* n = scale_pop();
	if (strcmp(s, "float") == 0) {
		scale_push_double(*(double*) &n);
		return;
	} else if (strcmp(s, "int") == 0) {
		scale_push_long(*(long long*) &n);
		return;
	} else if (strcmp(s, "string") == 0) {
		scale_push_string(*(char**) &n);
		return;
	}
	scale_push_long(EX_CAST_ERROR);
	scale_extern_raise();
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
	scale_heap_collect_all(1);
	return 0;
}

#endif
