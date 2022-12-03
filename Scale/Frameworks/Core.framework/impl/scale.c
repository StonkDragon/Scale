#ifndef SCALE_H
#define SCALE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "scale.h"

extern scl_stack_t	 callstk;
extern scl_stack_t   stack;
extern size_t 		 sap_enabled[STACK_SIZE];
extern size_t 		 sap_count[STACK_SIZE];
extern size_t 		 sap_index;

#pragma region Natives

sclDefFunc(dumpstack, void) {
	callstk.data[callstk.ptr++].s = "dumpstack()";
	printf("Dump:\n");
	for (ssize_t i = stack.ptr - 1; i >= 0; i--) {
		long long v = (long long) stack.data[i].i;
		printf("   %zd: 0x%016llx, %lld, %s\n", i, v, v, (scl_str) v);
	}
	printf("\n");
	callstk.ptr--;
}

sclDefFunc(sleep, void) {
	callstk.data[callstk.ptr++].s = "sleep()";
	long long c = ctrl_pop_long();
	sleep(c);
	callstk.ptr--;
}

sclDefFunc(getenv, scl_str) {
	callstk.data[callstk.ptr++].s = "getenv()";
	scl_str c = ctrl_pop_string();
	scl_str prop = getenv(c);
	return prop;
	callstk.ptr--;
}

sclDefFunc(sizeof_stack, scl_int) {
	callstk.data[callstk.ptr++].s = "sizeof_stack()";
	callstk.ptr--;
	return stack.ptr;
}

sclDefFunc(concat, scl_str) {
	callstk.data[callstk.ptr++].s = "concat()";
	scl_str s2 = ctrl_pop_string();
	scl_str s1 = ctrl_pop_string();
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
	callstk.ptr--;
	return ((scl_int) rand() << 32) | (scl_int) rand();
}

sclDefFunc(crash, void) {
	callstk.data[callstk.ptr++].s = "crash()";
	scl_security_safe_exit(1);
	callstk.ptr--;
}

sclDefFunc(system, scl_int) {
	callstk.data[callstk.ptr++].s = "system()";
	scl_str cmd = ctrl_pop_string();
	int ret = system(cmd);
	callstk.ptr--;
	return ret;
}

sclDefFunc(strlen, scl_int) {
	callstk.data[callstk.ptr++].s = "strlen()";
	scl_str s = ctrl_pop_string();
	size_t len;
	for (len = 0; s[len] != '\0'; len++);
	callstk.ptr--;
	return len;
}

sclDefFunc(strcmp, scl_int) {
	callstk.data[callstk.ptr++].s = "strcmp()";
	scl_str s1 = ctrl_pop_string();
	scl_str s2 = ctrl_pop_string();
	callstk.ptr--;
	return strcmp(s1, s2) == 0;
}

sclDefFunc(strdup, scl_str) {
	callstk.data[callstk.ptr++].s = "strdup()";
	scl_str s = ctrl_pop_string();
	callstk.ptr--;
	return strdup(s);
}

sclDefFunc(strncmp, scl_int) {
	callstk.data[callstk.ptr++].s = "strncmp()";
	long long n = ctrl_pop_long();
	scl_str s1 = ctrl_pop_string();
	scl_str s2 = ctrl_pop_string();
	callstk.ptr--;
	return strncmp(s1, s2, n) == 0;
}

sclDefFunc(raise, void) {
	callstk.data[callstk.ptr++].s = "raise()";
	long long n = ctrl_pop_long();
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

sclDefFunc(strrev, scl_str) {
	callstk.data[callstk.ptr++].s = "strrev()";
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
	callstk.ptr--;
	return out;
}

struct Array {
	scl_int $__type__;
	scl_str $__type_name__;
	scl_value values;
	scl_value count;
	scl_value capacity;
};

void Method_Array_init(struct Array*);
void Method_Array_push(struct Array*);

sclDefFunc(strsplit, struct Array*) {
	callstk.data[callstk.ptr++].s = "strsplit()";
    scl_str sep = stack.data[--stack.ptr].s;
    scl_str string = strdup(stack.data[--stack.ptr].s);
	
    struct Array* arr = scl_alloc_struct(sizeof(struct Array), "Array");
    stack.data[stack.ptr++].i = 10;
    Method_Array_init(arr);

    scl_str line = strtok(string, sep);
    while (line != NULL) {
        stack.data[stack.ptr++].s = line;
        Method_Array_push(arr);
        line = strtok(NULL, sep);
    }
	callstk.ptr--;
    return arr;
}

sclDefFunc(malloc, scl_value) {
	callstk.data[callstk.ptr++].s = "malloc()";
	long long n = ctrl_pop_long();
	scl_value s = scl_alloc(n);
	if (!s) {
		scl_security_throw(EX_BAD_PTR, "malloc() failed!");
		return NULL;
	}
	callstk.ptr--;
	return s;
}

sclDefFunc(realloc, scl_value) {
	callstk.data[callstk.ptr++].s = "realloc()";
	long long n = ctrl_pop_long();
	scl_value s = ctrl_pop();
	if (!s) {
		scl_security_throw(EX_BAD_PTR, "realloc() failed!");
		return NULL;
	}
	callstk.ptr--;
	return scl_realloc(s, n);
}

sclDefFunc(free, void) {
	callstk.data[callstk.ptr++].s = "free()";
	scl_value s = ctrl_pop();
	scl_dealloc_struct(s);
	scl_free(s);
	callstk.ptr--;
}

sclDefFunc(breakpoint, void) {
	callstk.data[callstk.ptr++].s = "breakpoint()";
	printf("Hit breakpoint. Press enter to continue.\n");
	getchar();
	callstk.ptr--;
}

sclDefFunc(memset, void) {
	callstk.data[callstk.ptr++].s = "memset()";
	scl_value s = ctrl_pop();
	long long n = ctrl_pop_long();
	long long c = ctrl_pop_long();
	memset(s, c, n);
	callstk.ptr--;
}

sclDefFunc(memcpy, void) {
	callstk.data[callstk.ptr++].s = "memcpy()";
	scl_value s2 = ctrl_pop();
	scl_value s1 = ctrl_pop();
	long long n = ctrl_pop_long();
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

sclDefFunc(sqrt, scl_float) {
	callstk.data[callstk.ptr++].s = "sqrt()";
    double n = ctrl_pop_double();
	callstk.ptr--;
    return sqrt(n);
}

sclDefFunc(sin, scl_float) {
	callstk.data[callstk.ptr++].s = "sin()";
	double n = ctrl_pop_double();
	callstk.ptr--;
	return sin(n);
}

sclDefFunc(cos, scl_float) {
	callstk.data[callstk.ptr++].s = "cos()";
	double n = ctrl_pop_double();
	callstk.ptr--;
	return cos(n);
}

sclDefFunc(tan, scl_float) {
	callstk.data[callstk.ptr++].s = "tan()";
	double n = ctrl_pop_double();
	callstk.ptr--;
	return tan(n);
}

sclDefFunc(asin, scl_float) {
	callstk.data[callstk.ptr++].s = "asin()";
	double n = ctrl_pop_double();
	callstk.ptr--;
	return asin(n);
}

sclDefFunc(acos, scl_float) {
	callstk.data[callstk.ptr++].s = "acos()";
	double n = ctrl_pop_double();
	callstk.ptr--;
	return acos(n);
}

sclDefFunc(atan, scl_float) {
	callstk.data[callstk.ptr++].s = "atan()";
	double n = ctrl_pop_double();
	callstk.ptr--;
	return atan(n);
}

sclDefFunc(atan2, scl_float) {
	callstk.data[callstk.ptr++].s = "atan2()";
	double n2 = ctrl_pop_double();
	double n1 = ctrl_pop_double();
	callstk.ptr--;
	return atan2(n1, n2);
}

sclDefFunc(sinh, scl_float) {
	callstk.data[callstk.ptr++].s = "sinh()";
	double n = ctrl_pop_double();
	callstk.ptr--;
	return sinh(n);
}

sclDefFunc(cosh, scl_float) {
	callstk.data[callstk.ptr++].s = "cosh()";
	double n = ctrl_pop_double();
	callstk.ptr--;
	return cosh(n);
}

sclDefFunc(tanh, scl_float) {
	callstk.data[callstk.ptr++].s = "tanh()";
	double n = ctrl_pop_double();
	callstk.ptr--;
	return tanh(n);
}

sclDefFunc(asinh, scl_float) {
	callstk.data[callstk.ptr++].s = "asinh()";
	double n = ctrl_pop_double();
	callstk.ptr--;
	return asinh(n);
}

sclDefFunc(acosh, scl_float) {
	callstk.data[callstk.ptr++].s = "acosh()";
	double n = ctrl_pop_double();
	callstk.ptr--;
	return acosh(n);
}

sclDefFunc(atanh, scl_float) {
	callstk.data[callstk.ptr++].s = "atanh()";
	double n = ctrl_pop_double();
	callstk.ptr--;
	return atanh(n);
}

sclDefFunc(exp, scl_float) {
	callstk.data[callstk.ptr++].s = "exp()";
	double n = ctrl_pop_double();
	callstk.ptr--;
	return exp(n);
}

sclDefFunc(log, scl_float) {
	callstk.data[callstk.ptr++].s = "log()";
	double n = ctrl_pop_double();
	callstk.ptr--;
	return log(n);
}

sclDefFunc(log10, scl_float) {
	callstk.data[callstk.ptr++].s = "log10()";
	double n = ctrl_pop_double();
	callstk.ptr--;
	return log10(n);
}

sclDefFunc(longToString, scl_str) {
	callstk.data[callstk.ptr++].s = "longToString()";
	long long a = ctrl_pop_long();
	scl_str out = scl_alloc(25);
	sprintf(out, "%lld", a);
	callstk.ptr--;
	return out;
}

sclDefFunc(stringToLong, scl_int) {
	callstk.data[callstk.ptr++].s = "stringToLong()";
	scl_str s = ctrl_pop_string();
	long long a = atoll(s);
	callstk.ptr--;
	return a;
}

sclDefFunc(stringToDouble, scl_float) {
	callstk.data[callstk.ptr++].s = "stringToDouble()";
	scl_str s = ctrl_pop_string();
	double a = atof(s);
	callstk.ptr--;
	return a;
}

sclDefFunc(doubleToString, scl_str) {
	callstk.data[callstk.ptr++].s = "doubleToString()";
	double a = ctrl_pop_double();
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
