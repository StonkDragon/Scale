#ifndef SCALE
#define SCALE

#if __SIZEOF_POINTER__ < 8
#error "Scale is not supported on this platform"
#endif

#ifdef __cplusplus
extern "C" {
#endif

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

#include "../../Scale/comp/scale.h"

#define INITIAL_SIZE (1024)
#define MALLOC_LIMIT (1024)
#define MAX_STRING_SIZE (2048)
#define LONG_AS_STR_LEN (22)

// Define scale-specific signals
#define EX_BAD_PTR (128)
#define EX_STACK_OVERFLOW (129)
#define EX_STACK_UNDERFLOW (130)
#define EX_UNHANDLED_DATA (131)
#define EX_IO_ERROR (132)
#define EX_INVALID_ARGUMENT (133)
#define EX_CAST_ERROR (134)

/* FUNCTION PROTOTYPES */
void native_raise();
void native_strlen();
void native_free();
void fun_main();

/* STRUCTURES */
typedef struct {
	scl_word 	 ptr;
	unsigned int level;
	int 		 isFile;
} memory_t;
memory_t 		 memalloced[MALLOC_LIMIT];
int		 		 memalloced_ptr = 0;
struct {
	size_t 		 ptr;
	char*  		 name[INITIAL_SIZE];
} Callstack;
struct {
	size_t 	 	 ptr;
	size_t 	 	 depth;
	scl_word 	 data[INITIAL_SIZE][INITIAL_SIZE];
} stack = {0, -1, {0}};

void throw(int code, char* msg) {
	fprintf(stderr, "Exception: %s\n", msg);
	push_long(code);
	native_raise();
}

void require_elements(size_t n, char* func) {
	if (stack.ptr < n) {
		char* err = (char*) malloc(MAX_STRING_SIZE);
		sprintf(err, "Error: Function %s requires %zu arguments, but only %zu are provided.", func, n, stack.ptr);
		throw(EX_STACK_UNDERFLOW, err);
		free(err);
	}
}

void stacktrace_print() {
	size_t i;
	printf("Stacktrace:\n");
	for (i = (Callstack.ptr - 1); i >= 0; i--) {
		printf("  %s\n", Callstack.name[i]);
	}
}

void heap_collect() {
	int i;
	int count = 0;
	int collect = 1;
	for (i = 0; i < memalloced_ptr; i++) {
		if (memalloced[i].ptr) {
			if (memalloced[i].level > Callstack.ptr) {
				for (int j = (stack.ptr - 1); j >= 0; j--) {
					if (stack.data[stack.depth][j] == memalloced[i].ptr) {
						collect = 0;
						break;
					}
				}
				if (collect) {
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

void heap_collect_all() {
	int i;
	for (i = 0; i < memalloced_ptr; i++) {
		if (memalloced[i].ptr) {
			if (memalloced[i].isFile) {
				fclose(memalloced[i].ptr);
			} else {
				free(memalloced[i].ptr);
			}
			memalloced[i].ptr = NULL;
		}
	}
}

void heap_add(scl_word ptr, int isFile) {
	for (int i = 0; i < memalloced_ptr; i++) {
		if (memalloced[i].ptr == NULL) {
			memalloced[i].ptr = ptr;
			memalloced[i].isFile = isFile;
			memalloced[i].level = Callstack.ptr;
			return;
		}
	}
	memalloced[memalloced_ptr].ptr = ptr;
	memalloced[memalloced_ptr].isFile = isFile;
	memalloced[memalloced_ptr].level = Callstack.ptr;
	memalloced_ptr++;
}

int heap_is_alloced(scl_word ptr) {
	for (int i = 0; i < memalloced_ptr; i++) {
		if (memalloced[i].ptr == ptr) {
			return 1;
		}
	}
	return 0;
}

void heap_remove(scl_word ptr) {
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

void safe_exit(int code) {
	heap_collect_all();
	exit(code);
}

void function_start(char* name) {
	Callstack.name[Callstack.ptr++] = name;
	stack.depth++;
}

void function_end() {
	Callstack.ptr--;
	stack.depth--;
	heap_collect();
}

void function_native_start(char* name) {
	Callstack.name[Callstack.ptr++] = name;
}

void function_native_end() {
	Callstack.ptr--;
}

void function_nps_start(char* name) {
	Callstack.name[Callstack.ptr++] = name;
}

void function_nps_end() {
	Callstack.ptr--;
	heap_collect();
}

void print_stacktrace() {
	printf("Stacktrace:\n");
	for (int i = Callstack.ptr - 1; i >= 0; i--) {
		printf("  %s\n", Callstack.name[i]);
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
			signalString = (char*) malloc(strlen("Unhandled data on the stack. %zu element too much!") + LONG_AS_STR_LEN);
			sprintf(signalString, "Unhandled data on the stack. %zu element too much!", stack.ptr);
		} else {
			signalString = (char*) malloc(strlen("Unhandled data on the stack. %zu elements too much!") + LONG_AS_STR_LEN);
			sprintf(signalString, "Unhandled data on the stack. %zu elements too much!", stack.ptr);
		}
	}
	else if (sig_num == EX_CAST_ERROR) signalString = "Cast Error";
	else signalString = "Unknown signal";

	printf("\n%s\n\n", signalString);
	print_stacktrace();

	safe_exit(sig_num);
}

void push_string(const char* c) {
	if (stack.ptr + 1 >= INITIAL_SIZE) {
		throw(EX_STACK_OVERFLOW, "Stack overflow!");
	}
	stack.data[stack.depth][stack.ptr++] = (scl_word) c;
}

void push_long(long long n) {
	if (stack.ptr + 1 >= INITIAL_SIZE) {
		throw(EX_STACK_OVERFLOW, "Stack overflow!");
	}
	stack.data[stack.depth][stack.ptr] = (scl_word) n;
	stack.ptr++;
}

long long pop_long() {
	if (stack.ptr <= 0) {
		throw(EX_STACK_UNDERFLOW, "Stack underflow!");
	}
	return (long long) stack.data[stack.depth][--stack.ptr];
}

void push(scl_word n) {
	if (stack.ptr + 1 >= INITIAL_SIZE) {
		throw(EX_STACK_OVERFLOW, "Stack overflow!");
	}
	stack.data[stack.depth][stack.ptr++] = n;
}

char* pop_string() {
	if (stack.ptr <= 0) {
		throw(EX_STACK_UNDERFLOW, "Stack underflow!");
	}
	return (char*) stack.data[stack.depth][--stack.ptr];
}

scl_word pop() {
	if (stack.ptr <= 0) {
		throw(EX_STACK_UNDERFLOW, "Stack underflow!");
	}
	return stack.data[stack.depth][--stack.ptr];
}

scl_word pop_word() {
	if (stack.ptr <= 0) {
		throw(EX_STACK_UNDERFLOW, "Stack underflow!");
	}
	return stack.data[stack.depth][--stack.ptr];
}

void push_word(scl_word n) {
	if (stack.ptr + 1 >= INITIAL_SIZE) {
		throw(EX_STACK_OVERFLOW, "Stack overflow!");
	}
	stack.data[stack.depth][stack.ptr++] = n;
}

void op_add() {
	int64_t b = pop_long();
	int64_t a = pop_long();
	push_long(a + b);
}

void op_sub() {
	int64_t b = pop_long();
	int64_t a = pop_long();
	push_long(a - b);
}

void op_mul() {
	int64_t b = pop_long();
	int64_t a = pop_long();
	push_long(a * b);
}

void op_div() {
	int64_t b = pop_long();
	int64_t a = pop_long();
	if (b == 0) {
		throw(EX_INVALID_ARGUMENT, "Division by zero!");
	}
	push_long(a / b);
}

void op_mod() {
	int64_t b = pop_long();
	int64_t a = pop_long();
	if (b == 0) {
		throw(EX_INVALID_ARGUMENT, "Division by zero!");
	}
	push_long(a % b);
}

void op_land() {
	int64_t b = pop_long();
	int64_t a = pop_long();
	push_long(a & b);
}

void op_lor() {
	int64_t b = pop_long();
	int64_t a = pop_long();
	push_long(a | b);
}

void op_lxor() {
	int64_t b = pop_long();
	int64_t a = pop_long();
	push_long(a ^ b);
}

void op_lnot() {
	int64_t a = pop_long();
	push_long(~a);
}

void op_lsh() {
	int64_t b = pop_long();
	int64_t a = pop_long();
	push_long(a << b);
}

void op_rsh() {
	int64_t b = pop_long();
	int64_t a = pop_long();
	push_long(a >> b);
}

void op_pow() {
	long long exp = pop_long();
	if (exp < 0) {
		throw(EX_BAD_PTR, "Negative exponent!");
	}
	int64_t base = pop_long();
	long long intResult = (int64_t) base;
	long long i = 0;
	while (i < exp) {
		intResult *= (int64_t) base;
		i++;
	}
	push_long(intResult);
}

void native_dumpstack() {
	printf("Dump:\n");
	for (int i = stack.ptr - 1; i >= 0; i--) {
		long long v = (long long) stack.data[stack.depth][i];
		printf("    0x%016llx, %lld\n", v, v);
	}
	printf("\n");
}

void native_exit() {
	long long n = pop_long();
	safe_exit(n);
}

void native_sleep() {
	long long c = pop_long();
	sleep(c);
}

void native_getenv() {
	char *c = pop_string();
	char *prop = getenv(c);
	push_string(prop);
}

void native_less() {
	int64_t b = pop_long();
	int64_t a = pop_long();
	push_long(a < b);
}

void native_more() {
	int64_t b = pop_long();
	int64_t a = pop_long();
	push_long(a > b);
}

void native_equal() {
	int64_t a = pop_long();
	int64_t b = pop_long();
	push_long(a == b);
}

void native_dup() {
	scl_word c = pop();
	push(c);
	push(c);
}

void native_over() {
	void *a = pop();
	void *b = pop();
	void *c = pop();
	push(a);
	push(b);
	push(c);
}

void native_swap() {
	void *a = pop();
	void *b = pop();
	push(a);
	push(b);
}

void native_drop() {
	pop();
}

void native_sizeof_stack() {
	push_long(stack.ptr);
}

void native_concat() {
	char *s2 = pop_string();
	char *s1 = pop_string();
	push_string(s1);
	native_strlen();
	long long len = pop_long();
	push_string(s2);
	native_strlen();
	long long len2 = pop_long();
	char *out = (char*) malloc(len + len2 + 1);
	heap_add(out, 0);
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
	push_string(out);
}

void native_random() {
	push_long(rand());
}

void native_crash() {
	safe_exit(1);
}

void native_and() {
	int a = pop_long();
	int b = pop_long();
	push_long(a && b);
}

void native_system() {
	char *cmd = pop_string();
	int ret = system(cmd);
	push_long(ret);
}

void native_not() {
	push_long(!pop_long());
}

void native_or() {
	int a = pop_long();
	int b = pop_long();
	push_long(a || b);
}

void native_sprintf() {
	char *fmt = pop_string();
	scl_word s = pop();
	char *out = (char*) malloc(LONG_AS_STR_LEN + strlen(fmt) + 1);
	heap_add(out, 0);
	sprintf(out, fmt, s);
	push_string(out);
}

void native_strlen() {
	char *s = pop_string();
	size_t len = 0;
	while (s[len] != '\0') {
		len++;
	}
	push_long(len);
}

void native_strcmp() {
	char *s1 = pop_string();
	char *s2 = pop_string();
	push_long(strcmp(s1, s2) == 0);
}

void native_strncmp() {
	char *s1 = pop_string();
	char *s2 = pop_string();
	long long n = pop_long();
	push_long(strncmp(s1, s2, n) == 0);
}

void native_fopen() {
	char *mode = pop_string();
	char *name = pop_string();
	FILE *f = fopen(name, mode);
	if (f == NULL) {
		char* err = malloc(strlen("Unable to open file '%s'") + strlen(name) + 1);
		sprintf(err, "Unable to open file '%s'", name);
		throw(EX_IO_ERROR, err);
	}
	heap_add(f, 1);
	push((scl_word) f);
}

void native_fclose() {
	FILE *f = (FILE*) pop();
	heap_remove(f);
	fclose(f);
}

void native_fseek() {
	long long offset = pop_long();
	int whence = pop_long();
	FILE *f = (FILE*) pop();
	fseek(f, offset, whence);
}

void native_ftell() {
	FILE *f = (FILE*) pop();
	push((scl_word) f);
	push_long(ftell(f));
}

void native_fileno() {
	FILE *f = (FILE*) pop();
	push_long(fileno(f));
}

void native_raise() {
	long long n = pop_long();
	if (n != 2 && n != 4 && n != 6 && n != 8 && n != 11) {
		int raised = raise(n);
		if (raised != 0) {
			process_signal(n);
		}
	} else {
		process_signal(n);
	}
}

void native_abort() {
	abort();
}

void native_write() {
	void *s = pop();
	long long n = pop_long();
	long long fd = pop_long();
	write(fd, s, n);
	push(s);
	native_free();
}

void native_read() {
	long long n = pop_long();
	long long fd = pop_long();
	void *s = malloc(n);
	heap_add(s, 0);
	int ret = read(fd, s, n);
	if (ret == -1) {
		push_long(EX_IO_ERROR);
		native_raise();
	}
	push(s);
}

void native_strrev() {
	char* s = pop_string();
	int i = 0;
	push_string(s);
	native_strlen();
	long long len = pop_long();
	char* out = (char*) malloc(len + 1);
	heap_add(out, 0);
	for (i = len - 1; i >= 0; i--) {
		out[i] = s[i];
	}
	out[len] = '\0';
	push_string(out);
}

void native_malloc() {
	long long n = pop_long();
	scl_word s = malloc(n);
	heap_add(s, 0);
	push(s);
}

void native_free() {
	scl_word s = pop();
	int is_alloc = heap_is_alloced(s);
	heap_remove(s);
	if (is_alloc) {
		free(s);
	}
}

void native_breakpoint() {
	printf("Hit breakpoint. Press enter to continue.\n");
	getchar();
}

void native_memset() {
	scl_word s = pop();
	long long n = pop_long();
	long long c = pop_long();
	memset(s, c, n);
}

void native_memcpy() {
	scl_word s2 = pop();
	scl_word s1 = pop();
	long long n = pop_long();
	memcpy(s2, s1, n);
}

void native_time() {
	struct timespec t;
	clock_gettime(CLOCK_REALTIME, &t);
	long long millis = t.tv_sec * 1000 + t.tv_nsec / 1000000;
	push_long(millis);
}

void native_heap_collect() {
	heap_collect();
}

void native_trace() {
	stacktrace_print();
}

int main(int argc, char const *argv[])
{
	signal(SIGINT, process_signal);
	signal(SIGILL, process_signal);
	signal(SIGABRT, process_signal);
	signal(SIGFPE, process_signal);
	signal(SIGSEGV, process_signal);

	for (int i = argc - 1; i > 0; i--) {
		push_string(argv[i]);
	}

	fun_main();
	heap_collect_all();
	return 0;
}

#endif

#ifdef __cplusplus
}
#endif
