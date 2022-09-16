#ifndef SCALE_H
#define SCALE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "scale.h"

extern size_t		 memalloced_ptr[STACK_SIZE];
extern size_t 		 stack_depth;
extern scl_memory_t  memalloced[STACK_SIZE][MALLOC_LIMIT];
extern scl_stack_t	 callstk;
extern scl_stack_t   stack;
extern char* 		 current_file;
extern size_t 		 current_line;
extern size_t 		 current_column;
extern size_t 		 sap_enabled[STACK_SIZE];
extern size_t 		 sap_count[STACK_SIZE];
extern size_t 		 sap_index;

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
	print_stacktrace();
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
