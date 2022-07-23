#ifndef _SCALE_VERSION
#define _SCALE_VERSION "2.0"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#ifndef SCALE
#define SCALE

#ifdef _WIN32
#include <Windows.h>
#define sleep(s) Sleep(s)
#else
#include <unistd.h>
#define sleep(s) do { struct timespec __ts = {((s) / 1000), ((s) % 1000) * 1000000}; nanosleep(&__ts, NULL); } while (0)
#endif

#include "scale.h"

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
	for (int i = trace.ptr - 1; i >= 0; i--) {
		char* frame = (char*) trace.data[i];
		printf("    %s\n", frame);
	}
	printf("\n");
}

void process_signal(int sig_num)
{
	char* signalString;
	if (sig_num == SIGHUP) signalString = "Hangup";
	else if (sig_num == SIGINT) signalString = "Interrupt";
	else if (sig_num == SIGQUIT) signalString = "Quit";
	else if (sig_num == SIGILL) signalString = "Illegal instruction";
	else if (sig_num == SIGTRAP) signalString = "Trace trap";
	else if (sig_num == SIGABRT) signalString = "abort()";
	#if (defined(_POSIX_C_SOURCE) && !defined(_DARWIN_C_SOURCE))
	else if (sig_num == SIGPOLL) signalString = "Pollable event ([XSR] generated, not supported)";
	#else
	else if (sig_num == SIGIOT) signalString = "Compatibility";
	else if (sig_num == SIGEMT) signalString = "EMT instruction";
	#endif
	else if (sig_num == SIGFPE) signalString = "Floating point exception";
	else if (sig_num == SIGKILL) signalString = "Kill";
	else if (sig_num == SIGBUS) signalString = "Bus error";
	else if (sig_num == SIGSEGV) signalString = "Segmentation violation";
	else if (sig_num == SIGSYS) signalString = "Bad argument to system call";
	else if (sig_num == SIGPIPE) signalString = "Write on unused pipe";
	else if (sig_num == SIGALRM) signalString = "Alarm clock";
	else if (sig_num == SIGTERM) signalString = "Software termination";
	else if (sig_num == SIGURG) signalString = "Urgent condition on IO channel";
	else if (sig_num == SIGSTOP) signalString = "Stop signal";
	else if (sig_num == SIGTSTP) signalString = "Stop signal from tty";
	else if (sig_num == SIGCONT) signalString = "Continue";
	else if (sig_num == SIGCHLD) signalString = "Child stop or exit";
	else if (sig_num == SIGTTIN) signalString = "Background tty read";
	else if (sig_num == SIGTTOU) signalString = "Background tty write";
	#if (!defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE))
	else if (sig_num == SIGIO) signalString = "Input/output signal";
	#endif
	else if (sig_num == SIGXCPU) signalString = "Exceeded CPU time limit";
	else if (sig_num == SIGXFSZ) signalString = "Exceeded file size limit";
	else if (sig_num == SIGVTALRM) signalString = "Virtual time alarm";
	else if (sig_num == SIGPROF) signalString = "Profiling time alarm";
	#if  (!defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE))
	else if (sig_num == SIGWINCH) signalString = "Window size changes";
	else if (sig_num == SIGINFO) signalString = "Information request";
	#endif
	else if (sig_num == SIGUSR1) signalString = "User defined signal 1";
	else if (sig_num == SIGUSR2) signalString = "User defined signal 2";
	else signalString = "Unknown signal";

	printf("Received signal '%s' (%d)\n", signalString, sig_num);
	stacktrace_print();
	exit(sig_num);
}

void scale_extern_trace() {
	stacktrace_print();
}

void scale_push_string(const char* c) {
	stack.data[stack.ptr++] = (char*) c;
}

#if __SIZEOF_POINTER__ >= 8
void scale_push_long(long long n) {
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
		return 0;
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

void* scale_pop() {
	void* v = stack.data[--stack.ptr];
	return v;
}

void scale_extern_dumpstack() {
	printf("Dump:\n");
	for (int i = stack.ptr - 1; i >= 0; i--) {
		void* v = stack.data[i];
		printf("    %p\n", v);
	}
	printf("\n");
}

void scale_extern_printf() {
	char *c = scale_pop_string();
	printf("%s", c);
	scale_push_string(c);
}

void scale_extern_read() {
	char* c = (char*) malloc(MAX_STRING_SIZE);
	fgets(c, MAX_STRING_SIZE, stdin);
	int len = strlen(c);
	if (c[len - 1] == '\n') {
		c[len - 1] = '\0';
	}
	scale_push_string(c);
}

void scale_extern_tochars() {
	char *c = scale_pop_string();
	for (int i = strlen(c); i >= 0; i--) {
		scale_push_long(c[i]);
	}
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
// TODO: Args

void scale_extern_concat() {
	char *s2 = scale_pop_string();
	char *s1 = scale_pop_string();
	char *out = (char*) malloc(strlen(s1) + strlen(s2) + 1);
	strcpy(out, s1);
	strcat(out, s2);
	scale_push_string(out);
	free(out);
}

void scale_extern_strstarts() {
	char *s1 = scale_pop_string();
	char *s2 = scale_pop_string();
	scale_push_long(strncmp(s1, s2, strlen(s2)) == 0);
}

void scale_extern_add() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_long(a + b);
}

void scale_extern_sub() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_long(a - b);
}

void scale_extern_mul() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_long(a * b);
}

void scale_extern_div() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_long(a / b);
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

void scale_extern_mod() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_long(a % b);
}

void scale_extern_lshift() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_long(a << b);
}

void scale_extern_rshift() {
	long long b = scale_pop_long();
	long long a = scale_pop_long();
	scale_push_long(a >> b);
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

void scale_extern_sprintf() {
	char *fmt = scale_pop_string();
	void* s = scale_pop();
	char *out = (char*) malloc(20 + strlen(fmt) + 1);
	sprintf(out, fmt, s);
	scale_push_string(out);
	free(out);
}

void scale_extern_strlen() {
	char *s = scale_pop_string();
	scale_push_long(strlen(s));
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

void scale_extern_fprintf() {
	FILE *f = (FILE*) scale_pop();
	char *s = scale_pop_string();
	fprintf(f, "%s", s);
	scale_push((void*) f);
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

void scale_extern_fread() {
	long long size = scale_pop_long();
	FILE *f = (FILE*) scale_pop();

	char *s = (char*) malloc(size + 1);
	fread(s, size, 1, f);
	s[size] = '\0';
	
	scale_push((void*) f);
	scale_push_string(s);
}

void scale_extern_fseekstart() {
	long long pos = scale_pop_long();
	FILE *f = (FILE*) scale_pop();
	fseek(f, pos, SEEK_SET);
	scale_push((void*) f);
}

void scale_extern_fseekend() {
	long long pos = scale_pop_long();
	FILE *f = (FILE*) scale_pop();
	fseek(f, pos, SEEK_END);
	scale_push((void*) f);
}

void scale_extern_fseekcur() {
	long long pos = scale_pop_long();
	FILE *f = (FILE*) scale_pop();
	fseek(f, pos, SEEK_CUR);
	scale_push((void*) f);
}

void scale_extern_ftell() {
	FILE *f = (FILE*) scale_pop();
	scale_push((void*) f);
	scale_push_long(ftell(f));
}

void scale_extern_fwrite() {
	char *s = scale_pop_string();
	long long size = scale_pop_long();
	FILE *f = (FILE*) scale_pop();
	fwrite(s, size, 1, f);
	scale_push((void*) f);
}

void scale_extern_sizeof() {
	char* type = scale_pop_string();
	if (strcmp(type, "int") == 0) {
		scale_push_long(sizeof(int));
	} else if (strcmp(type, "long") == 0) {
		scale_push_long(sizeof(long));
	} else if (strcmp(type, "long long") == 0) {
		scale_push_long(sizeof(long long));
	} else if (strcmp(type, "float") == 0) {
		scale_push_long(sizeof(float));
	} else if (strcmp(type, "double") == 0) {
		scale_push_long(sizeof(double));
	} else if (strcmp(type, "char") == 0) {
		scale_push_long(sizeof(char));
	} else if (type[strlen(type) - 1] == '*') {
		scale_push_long(sizeof(void*));
	} else if (strcmp(type, "size_t") == 0) {
		scale_push_long(sizeof(size_t));
	} else if (strcmp(type, "ssize_t") == 0) {
		scale_push_long(sizeof(ssize_t));
	}
}

void scale_extern_raise() {
	long long n = scale_pop_long();
	int raised = raise(n);
	if (raised != 0) {
		process_signal(n);
	}
}

void scale_extern_abort() {
	abort();
}

void scale_func_main();

int main(int argc, char const *argv[])
{
	for (int i = 1; i < argc; i++) {
		scale_push_string(argv[i]);
	}
	signal(SIGHUP, process_signal);       	/* hangup */
	signal(SIGINT, process_signal);       	/* interrupt */
	signal(SIGQUIT, process_signal);       	/* quit */
	signal(SIGILL, process_signal);       	/* illegal instruction (not reset when caught) */
	signal(SIGTRAP, process_signal);       	/* trace trap (not reset when caught) */
	signal(SIGABRT, process_signal);      	/* abort() */
	#if  (defined(_POSIX_C_SOURCE) && !defined(_DARWIN_C_SOURCE))
	signal(SIGPOLL, process_signal);       	/* pollable event ([XSR] generated, not supported) */
	#else   /* (!_POSIX_C_SOURCE || _DARWIN_C_SOURCE) */
	signal(SIGIOT, process_signal); 		/* compatibility */
	signal(SIGEMT, process_signal);       	/* EMT instruction */
	#endif  /* (!_POSIX_C_SOURCE || _DARWIN_C_SOURCE) */
	signal(SIGFPE, process_signal);       	/* floating point exception */
	signal(SIGKILL, process_signal);       	/* kill (cannot be caught or ignored) */
	signal(SIGBUS, process_signal);      	/* bus error */
	signal(SIGSEGV, process_signal);      	/* segmentation violation */
	signal(SIGSYS, process_signal);      	/* bad argument to system call */
	signal(SIGPIPE, process_signal);      	/* write on a pipe with no one to read it */
	signal(SIGALRM, process_signal);      	/* alarm clock */
	signal(SIGTERM, process_signal);      	/* software termination signal from kill */
	signal(SIGURG, process_signal);      	/* urgent condition on IO channel */
	signal(SIGSTOP, process_signal);      	/* sendable stop signal not from tty */
	signal(SIGTSTP, process_signal);      	/* stop signal from tty */
	signal(SIGCONT, process_signal);      	/* continue a stopped process */
	signal(SIGCHLD, process_signal);      	/* to parent on child stop or exit */
	signal(SIGTTIN, process_signal);      	/* to readers pgrp upon background tty read */
	signal(SIGTTOU, process_signal);      	/* like TTIN for output if (tp->t_local&LTOSTOP) */
	#if  (!defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE))
	signal(SIGIO, process_signal);      	/* input/output possible signal */
	#endif
	signal(SIGXCPU, process_signal);      	/* exceeded CPU time limit */
	signal(SIGXFSZ, process_signal);      	/* exceeded file size limit */
	signal(SIGVTALRM, process_signal);    	/* virtual time alarm */
	signal(SIGPROF, process_signal);      	/* profiling time alarm */
#if  (!defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE))
	signal(SIGWINCH, process_signal);     	/* window size changes */
	signal(SIGINFO, process_signal);      	/* information request */
#endif
	signal(SIGUSR1, process_signal);      /* user defined signal 1 */
	signal(SIGUSR2, process_signal);      /* user defined signal 2 */

	scale_func_main();
	return scale_pop_long();
}


#endif
#endif
