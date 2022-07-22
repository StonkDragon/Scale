#ifndef _SCALE_VERSION
#define _SCALE_VERSION "2.0"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#ifndef SCALE
#define SCALE

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#define STACK_SIZE 65536
#define MAX_STRING_SIZE STACK_SIZE

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
	for (int i = trace.ptr - 2; i >= 0; i--) {
		char* frame = (char*) trace.data[i];
		printf("    %s\n", frame);
	}
	printf("\n");
}

int scale_extern_trace() {
	stacktrace_print();
	return 0;
}

void scale_push_string(char* c) {
	stack.data[stack.ptr++] = c;
}

#if __SIZEOF_POINTER__ >= 4
void scale_push_int(int n) {
	stack.data[stack.ptr++] = (void*) ((long long) n);
}
#else
#error "Pointer size is not supported"
#endif

#if __SIZEOF_POINTER__ >= 8
void scale_push_long(long long n) {
	stack.data[stack.ptr++] = (void*) n;
}
#else
void scale_push_long(int n) {
	scale_push_int(n);
}
#endif

void scale_push(void* n) {
	stack.data[stack.ptr++] = n;
}

char* scale_pop_string() {
	if (stack.ptr <= 0) {
		return "(null)";
	}
	char *c = (char*) stack.data[--stack.ptr];
	return c;
}

#if __SIZEOF_POINTER__ >= 8
long long scale_pop_long() {
	if (stack.ptr <= 0) {
		return 0;
	}
	void* n = stack.data[--stack.ptr];
	return (long long) n;
}
#else
#error "Pointer size is not supported"
#endif

#if __SIZEOF_POINTER__ >= 4
long long scale_pop_int() {
	return (int) (scale_pop_long() & 0xFFFFFFFF);
}
#else
#error "Pointer size is not supported"
#endif

void* scale_pop() {
	void* v = stack.data[--stack.ptr];
	return v;
}

int scale_extern_printf() {
	char *c = scale_pop_string();
	printf("%s", c);
	scale_push_string(c);
	return 0;
}

int scale_extern_read() {
	char *c = (char*) malloc(MAX_STRING_SIZE);
	fread(c, 1, MAX_STRING_SIZE, stdin);
	scale_push_string(c);
	free(c);
	return 0;
}

int scale_extern_tochars() {
	char *c = scale_pop_string();
	for (int i = strlen(c); i >= 0; i--) {
		scale_push_int(c[i]);
	}
	return 0;
}

int scale_extern_exit() {
	int n = scale_pop_int();
	exit(n);
	return 0;
}

int scale_extern_sleep() {
	int c = scale_pop_int();
	sleep(c);
	return 0;
}

int scale_extern_getproperty() {
	char *c = scale_pop_string();
	char *prop = getenv(c);
	scale_push_string(prop);
	return 0;
}

int scale_extern_less() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_int(a < b);
	return 0;
}

int scale_extern_more() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_int(a > b);
	return 0;
}

int scale_extern_equal() {
	long long a = scale_pop_long();
	long long b = scale_pop_long();
	scale_push_int(a == b);
	return 0;
}

int scale_extern_dup() {
	void* c = scale_pop();
	scale_push(c);
	scale_push(c);
	return 0;
}

int scale_extern_over() {
	void *a = scale_pop();
	void *b = scale_pop();
	void *c = scale_pop();
	scale_push(a);
	scale_push(b);
	scale_push(c);
	return 0;
}

int scale_extern_swap() {
	void *a = scale_pop();
	void *b = scale_pop();
	scale_push(a);
	scale_push(b);
	return 0;
}

int scale_extern_drop() {
	scale_pop();
	return 0;
}

int scale_extern_sizeof_stack() {
	scale_push_long(stack.ptr);
	return 0;
}
// TODO: Args

int scale_extern_concat() {
	char *s2 = scale_pop_string();
	char *s1 = scale_pop_string();
	char *out = (char*) malloc(strlen(s1) + strlen(s2) + 1);
	strcpy(out, s1);
	strcat(out, s2);
	scale_push_string(out);
	free(out);
	return 0;
}

int scale_extern_strstarts() {
	char *s1 = scale_pop_string();
	char *s2 = scale_pop_string();
	scale_push_int(strncmp(s1, s2, strlen(s2)) == 0);
	return 0;
}

int scale_extern_add() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_long(a + b);
	return 0;
}

int scale_extern_sub() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_long(a - b);
	return 0;
}

int scale_extern_mul() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_long(a * b);
	return 0;
}

int scale_extern_div() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_long(a / b);
	return 0;
}

int scale_extern_mod() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_long(a % b);
	return 0;
}

int scale_extern_lshift() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_long(a << b);
	return 0;
}

int scale_extern_rshift() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_long(a >> b);
	return 0;
}

int scale_extern_random() {
	scale_push_long(rand());
	return 0;
}

int scale_extern_crash() {
	exit(1);
	return 0;
}

int scale_extern_and() {
	int a = scale_pop_int();
	int b = scale_pop_int();
	scale_push_int(a && b);
	return 0;
}

int scale_extern_eval() {
	char *cmd = scale_pop_string();
	int ret = system(cmd);
	scale_push_int(ret);
	return 0;
}

int scale_extern_getstack() {
	int i = scale_pop_int();
	scale_push(stack.data[i]);
	return 0;
}

int scale_extern_not() {
	scale_push_int(!scale_pop_int());
	return 0;
}

int scale_extern_or() {
	int a = scale_pop_int();
	int b = scale_pop_int();
	scale_push_int(a || b);
	return 0;
}

#endif
#endif