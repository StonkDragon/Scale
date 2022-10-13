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

sclDefFunc(dumpstack) {
	printf("Dump:\n");
	ssize_t stack_offset = stack.offset[stack_depth];
	for (ssize_t i = stack.ptr - 1; i >= stack_offset; i--) {
		long long v = (long long) stack.data[i].ptr;
		printf("   %zd: 0x%016llx, %lld\n", i, v, v);
	}
	printf("\n");
}

sclDefFunc(exit) {
	long long n = ctrl_pop_long();
	scl_security_safe_exit(n);
}

sclDefFunc(sleep) {
	long long c = ctrl_pop_long();
	sleep(c);
}

sclDefFunc(getenv) {
	char *c = ctrl_pop_string();
	char *prop = getenv(c);
	ctrl_push_string(prop);
}

sclDefFunc(less) {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a < b);
}

sclDefFunc(more) {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a > b);
}

sclDefFunc(equal) {
	int64_t a = ctrl_pop_long();
	int64_t b = ctrl_pop_long();
	ctrl_push_long(a == b);
}

sclDefFunc(sizeof_stack) {
	ctrl_push_long(stack.ptr - stack.offset[stack_depth]);
}

sclDefFunc(concat) {
	char *s2 = ctrl_pop_string();
	char *s1 = ctrl_pop_string();
	ctrl_push_string(s1);
	fn_strlen();
	long long len = ctrl_pop_long();
	ctrl_push_string(s2);
	fn_strlen();
	long long len2 = ctrl_pop_long();
	char *out = (char*) malloc(len + len2 + 1);
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

sclDefFunc(random) {
	ctrl_push_long(rand());
}

sclDefFunc(crash) {
	scl_security_safe_exit(1);
}

sclDefFunc(and) {
	int a = ctrl_pop_long();
	int b = ctrl_pop_long();
	ctrl_push_long(a && b);
}

sclDefFunc(system) {
	char *cmd = ctrl_pop_string();
	int ret = system(cmd);
	ctrl_push_long(ret);
}

sclDefFunc(not) {
	ctrl_push_long(!ctrl_pop_long());
}

sclDefFunc(or) {
	int a = ctrl_pop_long();
	int b = ctrl_pop_long();
	ctrl_push_long(a || b);
}

sclDefFunc(sprintf) {
	scl_value s = ctrl_pop();
	char *fmt = ctrl_pop_string();
	char *out = (char*) malloc(LONG_AS_STR_LEN + strlen(fmt) + 1);
	sprintf(out, fmt, s);
	ctrl_push_string(out);
}

sclDefFunc(strlen) {
	char *s = ctrl_pop_string();
	size_t len = 0;
	while (s[len] != '\0') {
		len++;
	}
	ctrl_push_long(len);
}

sclDefFunc(strcmp) {
	char *s1 = ctrl_pop_string();
	char *s2 = ctrl_pop_string();
	ctrl_push_long(strcmp(s1, s2) == 0);
}

sclDefFunc(strncmp) {
	long long n = ctrl_pop_long();
	char *s1 = ctrl_pop_string();
	char *s2 = ctrl_pop_string();
	ctrl_push_long(strncmp(s1, s2, n) == 0);
}

sclDefFunc(fopen) {
	char *mode = ctrl_pop_string();
	char *name = ctrl_pop_string();
	FILE *f = fopen(name, mode);
	if (f == NULL) {
		char* err = malloc(strlen("Unable to open file '%s'") + strlen(name) + 1);
		sprintf(err, "Unable to open file '%s'", name);
		scl_security_throw(EX_IO_ERROR, err);
	}
	ctrl_push((scl_value) f);
}

sclDefFunc(fclose) {
	FILE *f = (FILE*) ctrl_pop();
	fclose(f);
}

sclDefFunc(fseek) {
	long long offset = ctrl_pop_long();
	int whence = ctrl_pop_long();
	FILE *f = (FILE*) ctrl_pop();
	fseek(f, offset, whence);
}

sclDefFunc(ftell) {
	FILE *f = (FILE*) ctrl_pop();
	ctrl_push((scl_value) f);
	ctrl_push_long(ftell(f));
}

sclDefFunc(fileno) {
	FILE *f = (FILE*) ctrl_pop();
	ctrl_push_long(fileno(f));
}

sclDefFunc(raise) {
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

sclDefFunc(abort) {
	abort();
}

sclDefFunc(write) {
	long long n = ctrl_pop_long();
	scl_value s = ctrl_pop();
	long long fd = ctrl_pop_long();
	ssize_t result = write(fd, s, n);
	if (result == -1) {
		scl_security_throw(EX_INVALID_ARGUMENT, "Write failed");
	}
}

sclDefFunc(read) {
	long long n = ctrl_pop_long();
	scl_value s = malloc(n);
	long long fd = ctrl_pop_long();
	int ret = read(fd, s, n);
	if (ret == -1) {
		ctrl_push_long(EX_IO_ERROR);
		fn_raise();
	}
	ctrl_push(s);
}

sclDefFunc(strrev) {
	char* s = ctrl_pop_string();
	size_t i = 0;
	ctrl_push_string(s);
	fn_strlen();
	long long len = ctrl_pop_long();
	char* out = (char*) malloc(len + 1);
	for (i = len - 1; i >= 0; i--) {
		out[i] = s[i];
	}
	out[len] = '\0';
	ctrl_push_string(out);
}

sclDefFunc(malloc) {
	long long n = ctrl_pop_long();
	scl_value s = malloc(n);
	ctrl_push(s);
}

sclDefFunc(realloc) {
	long long n = ctrl_pop_long();
	scl_value s = ctrl_pop();
	ctrl_push(realloc(s, n));
}

sclDefFunc(free) {
	scl_value s = ctrl_pop();
	free(s);
}

sclDefFunc(breakpoint) {
	printf("Hit breakpoint. Press enter to continue.\n");
	getchar();
}

sclDefFunc(memset) {
	scl_value s = ctrl_pop();
	long long n = ctrl_pop_long();
	long long c = ctrl_pop_long();
	memset(s, c, n);
}

sclDefFunc(memcpy) {
	scl_value s2 = ctrl_pop();
	scl_value s1 = ctrl_pop();
	long long n = ctrl_pop_long();
	memcpy(s2, s1, n);
}

sclDefFunc(time) {
	struct timespec t;
	clock_gettime(CLOCK_REALTIME, &t);
	long long millis = t.tv_sec * 1000 + t.tv_nsec / 1000000;
	ctrl_push_long(millis);
}

sclDefFunc(trace) {
	print_stacktrace();
}

sclDefFunc(sqrt) {
    double n = ctrl_pop_double();
    ctrl_push_double(sqrt(n));
}

sclDefFunc(sin) {
	double n = ctrl_pop_double();
	ctrl_push_double(sin(n));
}

sclDefFunc(cos) {
	double n = ctrl_pop_double();
	ctrl_push_double(cos(n));
}

sclDefFunc(tan) {
	double n = ctrl_pop_double();
	ctrl_push_double(tan(n));
}

sclDefFunc(asin) {
	double n = ctrl_pop_double();
	ctrl_push_double(asin(n));
}

sclDefFunc(acos) {
	double n = ctrl_pop_double();
	ctrl_push_double(acos(n));
}

sclDefFunc(atan) {
	double n = ctrl_pop_double();
	ctrl_push_double(atan(n));
}

sclDefFunc(atan2) {
	double n2 = ctrl_pop_double();
	double n1 = ctrl_pop_double();
	ctrl_push_double(atan2(n1, n2));
}

sclDefFunc(sinh) {
	double n = ctrl_pop_double();
	ctrl_push_double(sinh(n));
}

sclDefFunc(cosh) {
	double n = ctrl_pop_double();
	ctrl_push_double(cosh(n));
}

sclDefFunc(tanh) {
	double n = ctrl_pop_double();
	ctrl_push_double(tanh(n));
}

sclDefFunc(asinh) {
	double n = ctrl_pop_double();
	ctrl_push_double(asinh(n));
}

sclDefFunc(acosh) {
	double n = ctrl_pop_double();
	ctrl_push_double(acosh(n));
}

sclDefFunc(atanh) {
	double n = ctrl_pop_double();
	ctrl_push_double(atanh(n));
}

sclDefFunc(exp) {
	double n = ctrl_pop_double();
	ctrl_push_double(exp(n));
}

sclDefFunc(log) {
	double n = ctrl_pop_double();
	ctrl_push_double(log(n));
}

sclDefFunc(log10) {
	double n = ctrl_pop_double();
	ctrl_push_double(log10(n));
}

sclDefFunc(longToString) {
	long long a = ctrl_pop_long();
	char *out = (char*) malloc(25);
	sprintf(out, "%lld", a);
	ctrl_push_string(out);
}

sclDefFunc(stringToLong) {
	char *s = ctrl_pop_string();
	long long a = atoll(s);
	ctrl_push_long(a);
}

sclDefFunc(stringToDouble) {
	char *s = ctrl_pop_string();
	double a = atof(s);
	ctrl_push_double(a);
}

sclDefFunc(doubleToString) {
	double a = ctrl_pop_double();
	char *out = (char*) malloc(100);
	sprintf(out, "%f", a);
	ctrl_push_string(out);
}

#pragma endregion

#ifdef __cplusplus
}
#endif

#endif // SCALE_H
