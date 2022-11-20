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

sclDefFunc(dumpstack, void) {
	printf("Dump:\n");
	for (ssize_t i = stack.ptr - 1; i >= 0; i--) {
		long long v = (long long) stack.data[i].i;
		printf("   %zd: 0x%016llx, %lld\n", i, v, v);
	}
	printf("\n");
}

sclDefFunc(sleep, void) {
	long long c = ctrl_pop_long();
	sleep(c);
}

sclDefFunc(getenv, scl_str) {
	scl_str c = ctrl_pop_string();
	scl_str prop = getenv(c);
	return prop;
}

sclDefFunc(sizeof_stack, scl_int) {
	return stack.ptr;
}

sclDefFunc(concat, scl_str) {
	scl_str s2 = ctrl_pop_string();
	scl_str s1 = ctrl_pop_string();
	ctrl_push_string(s1);
	Function_strlen();
	long long len = ctrl_pop_long();
	ctrl_push_string(s2);
	Function_strlen();
	long long len2 = ctrl_pop_long();
	scl_str out = scl_alloc(len + len2 + 1);
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
	return out;
}

int rand_was_seeded = 0;

sclDefFunc(random, scl_int) {
	if (!rand_was_seeded) {
		srand(time(NULL));
		rand_was_seeded = 1;
	}
	return ((scl_int) rand() << 32) | (scl_int) rand();
}

sclDefFunc(crash, void) {
	scl_security_safe_exit(1);
}

sclDefFunc(system, scl_int) {
	scl_str cmd = ctrl_pop_string();
	int ret = system(cmd);
	return ret;
}

sclDefFunc(strlen, scl_int) {
	scl_str s = ctrl_pop_string();
	size_t len;
	for (len = 0; s[len] != '\0'; len++);
	return len;
}

sclDefFunc(strcmp, scl_int) {
	scl_str s1 = ctrl_pop_string();
	scl_str s2 = ctrl_pop_string();
	return strcmp(s1, s2) == 0;
}

sclDefFunc(strncmp, scl_int) {
	long long n = ctrl_pop_long();
	scl_str s1 = ctrl_pop_string();
	scl_str s2 = ctrl_pop_string();
	return strncmp(s1, s2, n) == 0;
}

sclDefFunc(fopen, scl_value) {
	scl_str mode = ctrl_pop_string();
	scl_str name = ctrl_pop_string();
	FILE *f = fopen(name, mode);
	if (f == NULL) {
		char* err = malloc(strlen("Unable to open file '%s'") + strlen(name) + 1);
		sprintf(err, "Unable to open file '%s'", name);
		scl_security_throw(EX_IO_ERROR, err);
	}
	return (scl_value) f;
}

sclDefFunc(fclose, void) {
	FILE *f = (FILE*) ctrl_pop();
	fclose(f);
}

sclDefFunc(fseek, void) {
	long long offset = ctrl_pop_long();
	int whence = ctrl_pop_long();
	FILE *f = (FILE*) ctrl_pop();
	fseek(f, offset, whence);
}

sclDefFunc(ftell, scl_int) {
	FILE *f = (FILE*) ctrl_pop();
	ctrl_push((scl_value) f);
	return ftell(f);
}

sclDefFunc(fileno, scl_int) {
	FILE *f = (FILE*) ctrl_pop();
	return fileno(f);
}

sclDefFunc(raise, void) {
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

sclDefFunc(write, void) {
	long long n = ctrl_pop_long();
	scl_value s = ctrl_pop();
	long long fd = ctrl_pop_long();
	ssize_t result = write(fd, s, n);
	if (result == -1) {
		scl_security_throw(EX_INVALID_ARGUMENT, "Write failed");
	}
}

sclDefFunc(read, scl_value) {
	long long n = ctrl_pop_long();
	scl_value s = scl_alloc(n);
	long long fd = ctrl_pop_long();
	int ret = read(fd, s, n);
	if (ret == -1) {
		ctrl_push_long(EX_IO_ERROR);
		Function_raise();
	}
	return s;
}

sclDefFunc(strrev, scl_str) {
	char* s = ctrl_pop_string();
	size_t i = 0;
	ctrl_push_string(s);
	Function_strlen();
	long long len = ctrl_pop_long();
	char* out = scl_alloc(len + 1);
	for (i = len - 1; i >= 0; i--) {
		out[i] = s[i];
	}
	out[len] = '\0';
	return out;
}

sclDefFunc(malloc, scl_value) {
	long long n = ctrl_pop_long();
	scl_value s = scl_alloc(n);
	return s;
}

sclDefFunc(realloc, scl_value) {
	long long n = ctrl_pop_long();
	scl_value s = ctrl_pop();
	return scl_realloc(s, n);
}

sclDefFunc(free, void) {
	scl_value s = ctrl_pop();
	scl_dealloc_struct(s);
	scl_free(s);
}

sclDefFunc(breakpoint, void) {
	printf("Hit breakpoint. Press enter to continue.\n");
	getchar();
}

sclDefFunc(memset, void) {
	scl_value s = ctrl_pop();
	long long n = ctrl_pop_long();
	long long c = ctrl_pop_long();
	memset(s, c, n);
}

sclDefFunc(memcpy, void) {
	scl_value s2 = ctrl_pop();
	scl_value s1 = ctrl_pop();
	long long n = ctrl_pop_long();
	memcpy(s2, s1, n);
}

sclDefFunc(time, scl_float) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	double secs = (double)(tv.tv_usec) / 1000000 + (double)(tv.tv_sec);
	return secs;
}

sclDefFunc(trace, void) {
	print_stacktrace();
}

sclDefFunc(sqrt, scl_float) {
    double n = ctrl_pop_double();
    return sqrt(n);
}

sclDefFunc(sin, scl_float) {
	double n = ctrl_pop_double();
	return sin(n);
}

sclDefFunc(cos, scl_float) {
	double n = ctrl_pop_double();
	return cos(n);
}

sclDefFunc(tan, scl_float) {
	double n = ctrl_pop_double();
	return tan(n);
}

sclDefFunc(asin, scl_float) {
	double n = ctrl_pop_double();
	return asin(n);
}

sclDefFunc(acos, scl_float) {
	double n = ctrl_pop_double();
	return acos(n);
}

sclDefFunc(atan, scl_float) {
	double n = ctrl_pop_double();
	return atan(n);
}

sclDefFunc(atan2, scl_float) {
	double n2 = ctrl_pop_double();
	double n1 = ctrl_pop_double();
	return atan2(n1, n2);
}

sclDefFunc(sinh, scl_float) {
	double n = ctrl_pop_double();
	return sinh(n);
}

sclDefFunc(cosh, scl_float) {
	double n = ctrl_pop_double();
	return cosh(n);
}

sclDefFunc(tanh, scl_float) {
	double n = ctrl_pop_double();
	return tanh(n);
}

sclDefFunc(asinh, scl_float) {
	double n = ctrl_pop_double();
	return asinh(n);
}

sclDefFunc(acosh, scl_float) {
	double n = ctrl_pop_double();
	return acosh(n);
}

sclDefFunc(atanh, scl_float) {
	double n = ctrl_pop_double();
	return atanh(n);
}

sclDefFunc(exp, scl_float) {
	double n = ctrl_pop_double();
	return exp(n);
}

sclDefFunc(log, scl_float) {
	double n = ctrl_pop_double();
	return log(n);
}

sclDefFunc(log10, scl_float) {
	double n = ctrl_pop_double();
	return log10(n);
}

sclDefFunc(longToString, scl_str) {
	long long a = ctrl_pop_long();
	scl_str out = scl_alloc(25);
	sprintf(out, "%lld", a);
	return out;
}

sclDefFunc(stringToLong, scl_int) {
	scl_str s = ctrl_pop_string();
	long long a = atoll(s);
	return a;
}

sclDefFunc(stringToDouble, scl_float) {
	scl_str s = ctrl_pop_string();
	double a = atof(s);
	return a;
}

sclDefFunc(doubleToString, scl_str) {
	double a = ctrl_pop_double();
	scl_str out = scl_alloc(100);
	sprintf(out, "%f", a);
	return out;
}

#pragma endregion

#ifdef __cplusplus
}
#endif

#endif // SCALE_H
