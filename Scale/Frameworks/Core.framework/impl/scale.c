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

sclNativeImpl(dumpstack) {
	printf("Dump:\n");
	ssize_t stack_offset = stack.offset[stack_depth];
	for (ssize_t i = stack.ptr - 1; i >= stack_offset; i--) {
		long long v = (long long) stack.data[i];
		printf("   %zd: 0x%016llx, %lld\n", i, v, v);
	}
	printf("\n");
}

sclNativeImpl(exit) {
	long long n = ctrl_pop_long();
	safe_exit(n);
}

sclNativeImpl(sleep) {
	long long c = ctrl_pop_long();
	sleep(c);
}

sclNativeImpl(getenv) {
	char *c = ctrl_pop_string();
	char *prop = getenv(c);
	ctrl_push_string(prop);
}

sclNativeImpl(less) {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a < b);
}

sclNativeImpl(more) {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a > b);
}

sclNativeImpl(equal) {
	int64_t a = ctrl_pop_long();
	int64_t b = ctrl_pop_long();
	ctrl_push_long(a == b);
}

sclNativeImpl(dup) {
	scl_word c = ctrl_pop();
	ctrl_push(c);
	ctrl_push(c);
}

sclNativeImpl(over) {
	void *a = ctrl_pop();
	void *b = ctrl_pop();
	void *c = ctrl_pop();
	ctrl_push(a);
	ctrl_push(b);
	ctrl_push(c);
}

sclNativeImpl(swap) {
	void *a = ctrl_pop();
	void *b = ctrl_pop();
	ctrl_push(a);
	ctrl_push(b);
}

sclNativeImpl(drop) {
	ctrl_pop();
}

sclNativeImpl(sizeof_stack) {
	ctrl_push_long(stack.ptr - stack.offset[stack_depth]);
}

sclNativeImpl(concat) {
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

sclNativeImpl(random) {
	ctrl_push_long(rand());
}

sclNativeImpl(crash) {
	safe_exit(1);
}

sclNativeImpl(and) {
	int a = ctrl_pop_long();
	int b = ctrl_pop_long();
	ctrl_push_long(a && b);
}

sclNativeImpl(system) {
	char *cmd = ctrl_pop_string();
	int ret = system(cmd);
	ctrl_push_long(ret);
}

sclNativeImpl(not) {
	ctrl_push_long(!ctrl_pop_long());
}

sclNativeImpl(or) {
	int a = ctrl_pop_long();
	int b = ctrl_pop_long();
	ctrl_push_long(a || b);
}

sclNativeImpl(sprintf) {
	char *fmt = ctrl_pop_string();
	scl_word s = ctrl_pop();
	char *out = (char*) malloc(LONG_AS_STR_LEN + strlen(fmt) + 1);
	heap_add(out, 0);
	sprintf(out, fmt, s);
	ctrl_push_string(out);
}

sclNativeImpl(strlen) {
	char *s = ctrl_pop_string();
	size_t len = 0;
	while (s[len] != '\0') {
		len++;
	}
	ctrl_push_long(len);
}

sclNativeImpl(strcmp) {
	char *s1 = ctrl_pop_string();
	char *s2 = ctrl_pop_string();
	ctrl_push_long(strcmp(s1, s2) == 0);
}

sclNativeImpl(strncmp) {
	char *s1 = ctrl_pop_string();
	char *s2 = ctrl_pop_string();
	long long n = ctrl_pop_long();
	ctrl_push_long(strncmp(s1, s2, n) == 0);
}

sclNativeImpl(fopen) {
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

sclNativeImpl(fclose) {
	FILE *f = (FILE*) ctrl_pop();
	heap_remove(f);
	fclose(f);
}

sclNativeImpl(fseek) {
	long long offset = ctrl_pop_long();
	int whence = ctrl_pop_long();
	FILE *f = (FILE*) ctrl_pop();
	fseek(f, offset, whence);
}

sclNativeImpl(ftell) {
	FILE *f = (FILE*) ctrl_pop();
	ctrl_push((scl_word) f);
	ctrl_push_long(ftell(f));
}

sclNativeImpl(fileno) {
	FILE *f = (FILE*) ctrl_pop();
	ctrl_push_long(fileno(f));
}

sclNativeImpl(raise) {
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

sclNativeImpl(abort) {
	abort();
}

sclNativeImpl(write) {
	void *s = ctrl_pop();
	long long n = ctrl_pop_long();
	long long fd = ctrl_pop_long();
	write(fd, s, n);
	ctrl_push(s);
	native_free();
}

sclNativeImpl(read) {
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

sclNativeImpl(strrev) {
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

sclNativeImpl(malloc) {
	long long n = ctrl_pop_long();
	scl_word s = malloc(n);
	heap_add(s, 0);
	ctrl_push(s);
}

sclNativeImpl(free) {
	scl_word s = ctrl_pop();
	int is_alloc = heap_is_alloced(s);
	heap_remove(s);
	if (is_alloc) {
		free(s);
	}
}

sclNativeImpl(breakpoint) {
	printf("Hit breakpoint. Press enter to continue.\n");
	getchar();
}

sclNativeImpl(memset) {
	scl_word s = ctrl_pop();
	long long n = ctrl_pop_long();
	long long c = ctrl_pop_long();
	memset(s, c, n);
}

sclNativeImpl(memcpy) {
	scl_word s2 = ctrl_pop();
	scl_word s1 = ctrl_pop();
	long long n = ctrl_pop_long();
	memcpy(s2, s1, n);
}

sclNativeImpl(time) {
	struct timespec t;
	clock_gettime(CLOCK_REALTIME, &t);
	long long millis = t.tv_sec * 1000 + t.tv_nsec / 1000000;
	ctrl_push_long(millis);
}

sclNativeImpl(heap_collect) {
	heap_collect();
}

sclNativeImpl(trace) {
	print_stacktrace();
}

sclNativeImpl(sqrt) {
    double n = ctrl_pop_double();
    ctrl_push_double(sqrt(n));
}

sclNativeImpl(sin) {
	double n = ctrl_pop_double();
	ctrl_push_double(sin(n));
}

sclNativeImpl(cos) {
	double n = ctrl_pop_double();
	ctrl_push_double(cos(n));
}

sclNativeImpl(tan) {
	double n = ctrl_pop_double();
	ctrl_push_double(tan(n));
}

sclNativeImpl(asin) {
	double n = ctrl_pop_double();
	ctrl_push_double(asin(n));
}

sclNativeImpl(acos) {
	double n = ctrl_pop_double();
	ctrl_push_double(acos(n));
}

sclNativeImpl(atan) {
	double n = ctrl_pop_double();
	ctrl_push_double(atan(n));
}

sclNativeImpl(atan2) {
	double n2 = ctrl_pop_double();
	double n1 = ctrl_pop_double();
	ctrl_push_double(atan2(n1, n2));
}

sclNativeImpl(sinh) {
	double n = ctrl_pop_double();
	ctrl_push_double(sinh(n));
}

sclNativeImpl(cosh) {
	double n = ctrl_pop_double();
	ctrl_push_double(cosh(n));
}

sclNativeImpl(tanh) {
	double n = ctrl_pop_double();
	ctrl_push_double(tanh(n));
}

sclNativeImpl(asinh) {
	double n = ctrl_pop_double();
	ctrl_push_double(asinh(n));
}

sclNativeImpl(acosh) {
	double n = ctrl_pop_double();
	ctrl_push_double(acosh(n));
}

sclNativeImpl(atanh) {
	double n = ctrl_pop_double();
	ctrl_push_double(atanh(n));
}

sclNativeImpl(exp) {
	double n = ctrl_pop_double();
	ctrl_push_double(exp(n));
}

sclNativeImpl(log) {
	double n = ctrl_pop_double();
	ctrl_push_double(log(n));
}

sclNativeImpl(log10) {
	double n = ctrl_pop_double();
	ctrl_push_double(log10(n));
}

sclNativeImpl(longToString) {
	long long a = ctrl_pop_long();
	char *out = (char*) malloc(LONG_AS_STR_LEN + 1);
	heap_add(out, 0);
	sprintf(out, "%lld", a);
	ctrl_push_string(out);
}

sclNativeImpl(stringToLong) {
	char *s = ctrl_pop_string();
	long long a = atoll(s);
	ctrl_push_long(a);
}

sclNativeImpl(stringToDouble) {
	char *s = ctrl_pop_string();
	double a = atof(s);
	ctrl_push_double(a);
}

sclNativeImpl(doubleToString) {
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
