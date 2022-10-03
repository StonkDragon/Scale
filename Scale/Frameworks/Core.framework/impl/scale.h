#ifndef _SCALE_H_
#define _SCALE_H_

#include <scale_internal.h>

sclNativeImpl(dumpstack);
sclNativeImpl(exit);
sclNativeImpl(sleep);
sclNativeImpl(getenv);
sclNativeImpl(less);
sclNativeImpl(more);
sclNativeImpl(equal);
sclNativeImpl(dup);
sclNativeImpl(over);
sclNativeImpl(swap);
sclNativeImpl(drop);
sclNativeImpl(sizeof_stack);
sclNativeImpl(concat);
sclNativeImpl(random);
sclNativeImpl(crash);
sclNativeImpl(and);
sclNativeImpl(system);
sclNativeImpl(not);
sclNativeImpl(or);
sclNativeImpl(sprintf);
sclNativeImpl(strlen);
sclNativeImpl(strcmp);
sclNativeImpl(strncmp);
sclNativeImpl(fopen);
sclNativeImpl(fclose);
sclNativeImpl(fseek);
sclNativeImpl(ftell);
sclNativeImpl(fileno);
sclNativeImpl(raise);
sclNativeImpl(abort);
sclNativeImpl(write);
sclNativeImpl(read);
sclNativeImpl(strrev);
sclNativeImpl(malloc);
sclNativeImpl(realloc);
sclNativeImpl(free);
sclNativeImpl(breakpoint);
sclNativeImpl(memset);
sclNativeImpl(memcpy);
sclNativeImpl(time);
sclNativeImpl(trace);
sclNativeImpl(sqrt);
sclNativeImpl(sin);
sclNativeImpl(cos);
sclNativeImpl(tan);
sclNativeImpl(asin);
sclNativeImpl(acos);
sclNativeImpl(atan);
sclNativeImpl(atan2);
sclNativeImpl(sinh);
sclNativeImpl(cosh);
sclNativeImpl(tanh);
sclNativeImpl(asinh);
sclNativeImpl(acosh);
sclNativeImpl(atanh);
sclNativeImpl(exp);
sclNativeImpl(log);
sclNativeImpl(log10);
sclNativeImpl(longToString);
sclNativeImpl(stringToLong);
sclNativeImpl(stringToDouble);
sclNativeImpl(doubleToString);

#endif // _SCALE_H_
