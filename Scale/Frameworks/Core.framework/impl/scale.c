#ifndef SCALE_H
#define SCALE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "scale.h"

extern scl_stack_t	 callstk;
extern scl_stack_t   stack;
extern char* 		 current_file;
extern size_t 		 current_line;
extern size_t 		 current_column;
extern size_t 		 sap_enabled[STACK_SIZE];
extern size_t 		 sap_count[STACK_SIZE];
extern size_t 		 sap_index;

#pragma region Natives

sclDeclFunc(dumpstack) {
	printf("Dump:\n");
	for (ssize_t i = stack.ptr - 1; i >= 0; i--) {
		long long v = (long long) stack.data[i].value.i;
		printf("   %zd: 0x%016llx, %lld\n", i, v, v);
	}
	printf("\n");
}

sclDeclFunc(exit) {
	long long n = ctrl_pop_long();
	scl_security_safe_exit(n);
}

sclDeclFunc(sleep) {
	long long c = ctrl_pop_long();
	sleep(c);
}

sclDeclFunc(getenv) {
	char *c = ctrl_pop_string();
	char *prop = getenv(c);
	ctrl_push_string(prop);
}

sclDeclFunc(less) {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a < b);
}

sclDeclFunc(more) {
	int64_t b = ctrl_pop_long();
	int64_t a = ctrl_pop_long();
	ctrl_push_long(a > b);
}

sclDeclFunc(equal) {
	int64_t a = ctrl_pop_long();
	int64_t b = ctrl_pop_long();
	ctrl_push_long(a == b);
}

sclDeclFunc(sizeof_stack) {
	ctrl_push_long(stack.ptr);
}

sclDeclFunc(concat) {
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

sclDeclFunc(random) {
	ctrl_push_long(rand());
}

sclDeclFunc(crash) {
	scl_security_safe_exit(1);
}

sclDeclFunc(and) {
	int a = ctrl_pop_long();
	int b = ctrl_pop_long();
	ctrl_push_long(a && b);
}

sclDeclFunc(system) {
	char *cmd = ctrl_pop_string();
	int ret = system(cmd);
	ctrl_push_long(ret);
}

sclDeclFunc(not) {
	ctrl_push_long(!ctrl_pop_long());
}

sclDeclFunc(or) {
	int a = ctrl_pop_long();
	int b = ctrl_pop_long();
	ctrl_push_long(a || b);
}

sclDeclFunc(strlen) {
	char *s = ctrl_pop_string();
	size_t len;
	for (len = 0; s[len] != '\0'; len++);
	ctrl_push_long(len);
}

sclDeclFunc(strcmp) {
	char *s1 = ctrl_pop_string();
	char *s2 = ctrl_pop_string();
	ctrl_push_long(strcmp(s1, s2) == 0);
}

sclDeclFunc(strncmp) {
	long long n = ctrl_pop_long();
	char *s1 = ctrl_pop_string();
	char *s2 = ctrl_pop_string();
	ctrl_push_long(strncmp(s1, s2, n) == 0);
}

sclDeclFunc(fopen) {
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

sclDeclFunc(fclose) {
	FILE *f = (FILE*) ctrl_pop();
	fclose(f);
}

sclDeclFunc(fseek) {
	long long offset = ctrl_pop_long();
	int whence = ctrl_pop_long();
	FILE *f = (FILE*) ctrl_pop();
	fseek(f, offset, whence);
}

sclDeclFunc(ftell) {
	FILE *f = (FILE*) ctrl_pop();
	ctrl_push((scl_value) f);
	ctrl_push_long(ftell(f));
}

sclDeclFunc(fileno) {
	FILE *f = (FILE*) ctrl_pop();
	ctrl_push_long(fileno(f));
}

sclDeclFunc(raise) {
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

sclDeclFunc(abort) {
	abort();
}

sclDeclFunc(write) {
	long long n = ctrl_pop_long();
	scl_value s = ctrl_pop();
	long long fd = ctrl_pop_long();
	ssize_t result = write(fd, s, n);
	if (result == -1) {
		scl_security_throw(EX_INVALID_ARGUMENT, "Write failed");
	}
}

sclDeclFunc(read) {
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

sclDeclFunc(strrev) {
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

sclDeclFunc(malloc) {
	long long n = ctrl_pop_long();
	scl_value s = malloc(n);
	ctrl_push(s);
}

sclDeclFunc(realloc) {
	long long n = ctrl_pop_long();
	scl_value s = ctrl_pop();
	ctrl_push(realloc(s, n));
}

sclDeclFunc(free) {
	scl_value s = ctrl_pop();
	scl_dealloc_complex(s);
	free(s);
}

sclDeclFunc(breakpoint) {
	printf("Hit breakpoint. Press enter to continue.\n");
	getchar();
}

sclDeclFunc(memset) {
	scl_value s = ctrl_pop();
	long long n = ctrl_pop_long();
	long long c = ctrl_pop_long();
	memset(s, c, n);
}

sclDeclFunc(memcpy) {
	scl_value s2 = ctrl_pop();
	scl_value s1 = ctrl_pop();
	long long n = ctrl_pop_long();
	memcpy(s2, s1, n);
}

sclDeclFunc(time) {
	ctrl_push_long(time(NULL));
}

sclDeclFunc(trace) {
	print_stacktrace();
}

sclDeclFunc(sqrt) {
    double n = ctrl_pop_double();
    ctrl_push_double(sqrt(n));
}

sclDeclFunc(sin) {
	double n = ctrl_pop_double();
	ctrl_push_double(sin(n));
}

sclDeclFunc(cos) {
	double n = ctrl_pop_double();
	ctrl_push_double(cos(n));
}

sclDeclFunc(tan) {
	double n = ctrl_pop_double();
	ctrl_push_double(tan(n));
}

sclDeclFunc(asin) {
	double n = ctrl_pop_double();
	ctrl_push_double(asin(n));
}

sclDeclFunc(acos) {
	double n = ctrl_pop_double();
	ctrl_push_double(acos(n));
}

sclDeclFunc(atan) {
	double n = ctrl_pop_double();
	ctrl_push_double(atan(n));
}

sclDeclFunc(atan2) {
	double n2 = ctrl_pop_double();
	double n1 = ctrl_pop_double();
	ctrl_push_double(atan2(n1, n2));
}

sclDeclFunc(sinh) {
	double n = ctrl_pop_double();
	ctrl_push_double(sinh(n));
}

sclDeclFunc(cosh) {
	double n = ctrl_pop_double();
	ctrl_push_double(cosh(n));
}

sclDeclFunc(tanh) {
	double n = ctrl_pop_double();
	ctrl_push_double(tanh(n));
}

sclDeclFunc(asinh) {
	double n = ctrl_pop_double();
	ctrl_push_double(asinh(n));
}

sclDeclFunc(acosh) {
	double n = ctrl_pop_double();
	ctrl_push_double(acosh(n));
}

sclDeclFunc(atanh) {
	double n = ctrl_pop_double();
	ctrl_push_double(atanh(n));
}

sclDeclFunc(exp) {
	double n = ctrl_pop_double();
	ctrl_push_double(exp(n));
}

sclDeclFunc(log) {
	double n = ctrl_pop_double();
	ctrl_push_double(log(n));
}

sclDeclFunc(log10) {
	double n = ctrl_pop_double();
	ctrl_push_double(log10(n));
}

sclDeclFunc(longToString) {
	long long a = ctrl_pop_long();
	char *out = (char*) malloc(25);
	sprintf(out, "%lld", a);
	ctrl_push_string(out);
}

sclDeclFunc(stringToLong) {
	char *s = ctrl_pop_string();
	long long a = atoll(s);
	ctrl_push_long(a);
}

sclDeclFunc(stringToDouble) {
	char *s = ctrl_pop_string();
	double a = atof(s);
	ctrl_push_double(a);
}

sclDeclFunc(doubleToString) {
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
