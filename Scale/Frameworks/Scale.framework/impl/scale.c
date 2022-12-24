#ifndef SCALE_H
#define SCALE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "scale.h"

extern scl_stack_t stack;
extern scl_stack_t  callstk;

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

sclDefFunc(crash, void) {
	callstk.data[callstk.ptr++].s = "crash()";
	scl_security_safe_exit(1);
	callstk.ptr--;
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

sclDefFunc(breakpoint, void) {
	callstk.data[callstk.ptr++].s = "breakpoint()";
	printf("Hit breakpoint. Press enter to continue.\n");
	getchar();
	callstk.ptr--;
}

sclDefFunc(memset, void, scl_int c, scl_int n, scl_any s) {
	callstk.data[callstk.ptr++].s = "memset()";
	memset(s, c, n);
	callstk.ptr--;
}

sclDefFunc(memcpy, void, scl_int n, scl_any s1, scl_any s2) {
	callstk.data[callstk.ptr++].s = "memcpy()";
	memcpy(s2, s1, n);
	callstk.ptr--;
}

sclDefFunc(trace, void) {
	callstk.data[callstk.ptr++].s = "trace()";
	print_stacktrace();
	callstk.ptr--;
}

#pragma endregion

#ifdef __cplusplus
}
#endif

#endif // SCALE_H
