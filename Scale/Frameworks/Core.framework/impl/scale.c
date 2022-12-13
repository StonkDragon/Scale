#ifndef SCALE_H
#define SCALE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "scale.h"

extern scl_stack_t	 callstk;
extern scl_stack_t   stack;

#pragma region Natives

sclDefFunc(dumpstack, void) {
	callstk.data[callstk.ptr++].s = "dumpstack()";
	printf("Dump:\n");
	for (ssize_t i = stack.ptr - 1; i >= 0; i--) {
		long long v = (long long) stack.data[i].i;
		printf("   %zd: 0x%016llx, %lld\n", i, v, v);
	}
	printf("\n");
	callstk.ptr--;
}

sclDefFunc(sleep, void, scl_int c) {
	callstk.data[callstk.ptr++].s = "sleep()";
	sleep(c);
	callstk.ptr--;
}

sclDefFunc(getenv, scl_str, scl_str c) {
	callstk.data[callstk.ptr++].s = "getenv()";
	scl_str prop = getenv(c);
	callstk.ptr--;
	return prop;
}

sclDefFunc(sizeof_stack, scl_int) {
	callstk.data[callstk.ptr++].s = "sizeof_stack()";
	callstk.ptr--;
	return stack.ptr;
}

sclDefFunc(concat, scl_str, scl_str s2, scl_str s1) {
	callstk.data[callstk.ptr++].s = "concat()";
	long long len = strlen(s1);
	long long len2 = strlen(s2);
	scl_str out = scl_alloc(len + len2 + 1);
	strcpy(out, s1);
	strcat(out, s2);
	callstk.ptr--;
	return out;
}

int rand_was_seeded = 0;

sclDefFunc(random, scl_int) {
	callstk.data[callstk.ptr++].s = "random()";
	if (!rand_was_seeded) {
		srand(time(NULL));
		rand_was_seeded = 1;
	}
	scl_int r = ((scl_int) rand() << 32) | (scl_int) rand();
	callstk.ptr--;
	return r;
}

sclDefFunc(crash, void) {
	callstk.data[callstk.ptr++].s = "crash()";
	scl_security_safe_exit(1);
	callstk.ptr--;
}

sclDefFunc(system, scl_int, scl_str cmd) {
	callstk.data[callstk.ptr++].s = "system()";
	int ret = system(cmd);
	callstk.ptr--;
	return ret;
}

sclDefFunc(strlen, scl_int, scl_str s) {
	callstk.data[callstk.ptr++].s = "strlen()";
	size_t len;
	for (len = 0; s[len] != '\0'; len++);
	callstk.ptr--;
	return len;
}

sclDefFunc(strcmp, scl_int, scl_str s2, scl_str s1) {
	callstk.data[callstk.ptr++].s = "strcmp()";
	scl_int t = strcmp(s1, s2) == 0;
	callstk.ptr--;
	return t;
}

sclDefFunc(strdup, scl_str, scl_str s) {
	callstk.data[callstk.ptr++].s = "strdup()";
	scl_str t = strdup(s);
	callstk.ptr--;
	return t;
}

sclDefFunc(strncmp, scl_int, scl_int n, scl_str s1, scl_str s2) {
	callstk.data[callstk.ptr++].s = "strncmp()";
	scl_int t = strncmp(s1, s2, n) == 0;
	callstk.ptr--;
	return t;
}

sclDefFunc(raise, void, scl_int n) {
	callstk.data[callstk.ptr++].s = "raise()";
	if (n != 2 && n != 4 && n != 6 && n != 8 && n != 11) {
		int raised = raise(n);
		if (raised != 0) {
			process_signal(n);
		}
	} else {
		process_signal(n);
	}
	callstk.ptr--;
}

sclDefFunc(strrev, scl_str, scl_str s) {
	callstk.data[callstk.ptr++].s = "strrev()";
	size_t i = 0;
	scl_int len = Function_strlen(s);
	char* out = scl_alloc(len + 1);
	for (i = len - 1; i >= 0; i--) {
		out[i] = s[i];
	}
	out[len] = '\0';
	callstk.ptr--;
	return out;
}

struct Array {
	scl_int $__type__;
	scl_str $__type_name__;
	scl_value $__lock__;
	scl_value values;
	scl_value count;
	scl_value capacity;
};

void Method_Array_init(struct Array* Var_self, scl_int Var_size) __asm("mthd_Array_fnct_init_sIargs_int_sItype_none");
void Method_Array_push(struct Array* Var_self, scl_value Var_value) __asm("mthd_Array_fnct_push_sIargs_any_sItype_none");

sclDefFunc(strsplit, struct Array*, scl_str sep, scl_str string_) {
	callstk.data[callstk.ptr++].s = "strsplit()";
	scl_str string = strdup(string_);
    struct Array* arr = scl_alloc_struct(sizeof(struct Array), "Array");
    Method_Array_init(arr, 10);

    scl_str line = strtok(string, sep);
    while (line != NULL) {
        Method_Array_push(arr, line);
        line = strtok(NULL, sep);
    }
	callstk.ptr--;
    return arr;
}

sclDefFunc(malloc, scl_value, scl_int n) {
	callstk.data[callstk.ptr++].s = "malloc()";
	scl_value s = scl_alloc(n);
	if (!s) {
		scl_security_throw(EX_BAD_PTR, "malloc() failed!");
		return NULL;
	}
	callstk.ptr--;
	return s;
}

sclDefFunc(realloc, scl_value, scl_value s, scl_int n) {
	callstk.data[callstk.ptr++].s = "realloc()";
	if (!s) {
		scl_security_throw(EX_BAD_PTR, "realloc() failed!");
		return NULL;
	}
	callstk.ptr--;
	return scl_realloc(s, n);
}

sclDefFunc(free, void, scl_value s) {
	callstk.data[callstk.ptr++].s = "free()";
	scl_free(s);
	callstk.ptr--;
}

sclDefFunc(breakpoint, void) {
	callstk.data[callstk.ptr++].s = "breakpoint()";
	printf("Hit breakpoint. Press enter to continue.\n");
	getchar();
	callstk.ptr--;
}

sclDefFunc(memset, void, scl_int c, scl_int n, scl_value s) {
	callstk.data[callstk.ptr++].s = "memset()";
	memset(s, c, n);
	callstk.ptr--;
}

sclDefFunc(memcpy, void, scl_int n, scl_value s1, scl_value s2) {
	callstk.data[callstk.ptr++].s = "memcpy()";
	memcpy(s2, s1, n);
	callstk.ptr--;
}

sclDefFunc(time, scl_float) {
	callstk.data[callstk.ptr++].s = "time()";
	struct timeval tv;
	gettimeofday(&tv, NULL);
	double secs = (double)(tv.tv_usec) / 1000000 + (double)(tv.tv_sec);
	callstk.ptr--;
	return secs;
}

sclDefFunc(trace, void) {
	callstk.data[callstk.ptr++].s = "trace()";
	print_stacktrace();
	callstk.ptr--;
}

sclDefFunc(sqrt, scl_float, scl_float n) {
	callstk.data[callstk.ptr++].s = "sqrt()";
	callstk.ptr--;
    return sqrt(n);
}

sclDefFunc(sin, scl_float, scl_float n) {
	callstk.data[callstk.ptr++].s = "sin()";
	callstk.ptr--;
	return sin(n);
}

sclDefFunc(cos, scl_float, scl_float n) {
	callstk.data[callstk.ptr++].s = "cos()";
	callstk.ptr--;
	return cos(n);
}

sclDefFunc(tan, scl_float, scl_float n) {
	callstk.data[callstk.ptr++].s = "tan()";
	callstk.ptr--;
	return tan(n);
}

sclDefFunc(asin, scl_float, scl_float n) {
	callstk.data[callstk.ptr++].s = "asin()";
	callstk.ptr--;
	return asin(n);
}

sclDefFunc(acos, scl_float, scl_float n) {
	callstk.data[callstk.ptr++].s = "acos()";
	callstk.ptr--;
	return acos(n);
}

sclDefFunc(atan, scl_float, scl_float n) {
	callstk.data[callstk.ptr++].s = "atan()";
	callstk.ptr--;
	return atan(n);
}

sclDefFunc(atan2, scl_float, scl_float n1, scl_float n2) {
	callstk.data[callstk.ptr++].s = "atan2()";
	callstk.ptr--;
	return atan2(n1, n2);
}

sclDefFunc(sinh, scl_float, scl_float n) {
	callstk.data[callstk.ptr++].s = "sinh()";
	callstk.ptr--;
	return sinh(n);
}

sclDefFunc(cosh, scl_float, scl_float n) {
	callstk.data[callstk.ptr++].s = "cosh()";
	callstk.ptr--;
	return cosh(n);
}

sclDefFunc(tanh, scl_float, scl_float n) {
	callstk.data[callstk.ptr++].s = "tanh()";
	callstk.ptr--;
	return tanh(n);
}

sclDefFunc(asinh, scl_float, scl_float n) {
	callstk.data[callstk.ptr++].s = "asinh()";
	callstk.ptr--;
	return asinh(n);
}

sclDefFunc(acosh, scl_float, scl_float n) {
	callstk.data[callstk.ptr++].s = "acosh()";
	callstk.ptr--;
	return acosh(n);
}

sclDefFunc(atanh, scl_float, scl_float n) {
	callstk.data[callstk.ptr++].s = "atanh()";
	callstk.ptr--;
	return atanh(n);
}

sclDefFunc(exp, scl_float, scl_float n) {
	callstk.data[callstk.ptr++].s = "exp()";
	callstk.ptr--;
	return exp(n);
}

sclDefFunc(log, scl_float, scl_float n) {
	callstk.data[callstk.ptr++].s = "log()";
	callstk.ptr--;
	return log(n);
}

sclDefFunc(log10, scl_float, scl_float n) {
	callstk.data[callstk.ptr++].s = "log10()";
	callstk.ptr--;
	return log10(n);
}

sclDefFunc(longToString, scl_str, scl_int a) {
	callstk.data[callstk.ptr++].s = "longToString()";
	scl_str out = scl_alloc(25);
	sprintf(out, "%lld", a);
	callstk.ptr--;
	return out;
}

sclDefFunc(stringToLong, scl_int, scl_str s) {
	callstk.data[callstk.ptr++].s = "stringToLong()";
	long long a = atoll(s);
	callstk.ptr--;
	return a;
}

sclDefFunc(stringToDouble, scl_float, scl_str s) {
	callstk.data[callstk.ptr++].s = "stringToDouble()";
	double a = atof(s);
	callstk.ptr--;
	return a;
}

sclDefFunc(doubleToString, scl_str, scl_float a) {
	callstk.data[callstk.ptr++].s = "doubleToString()";
	scl_str out = scl_alloc(100);
	sprintf(out, "%f", a);
	callstk.ptr--;
	return out;
}

#pragma endregion

#ifdef __cplusplus
}
#endif

#endif // SCALE_H
