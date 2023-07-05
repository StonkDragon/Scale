#define _SCL_NO_STACK_OPS
#include <scale_runtime.h>
#undef _SCL_NO_STACK_OPS

_scl_stack_t**  stacks = nil;
scl_int         stacks_count = 0;
scl_int         stacks_cap = 64;

scl_any*        allocated = nil;
scl_int         allocated_count = 0;
scl_int         allocated_cap = 64;

scl_int*        memsizes = nil;
scl_int         memsizes_count = 0;
scl_int         memsizes_cap = 64;

scl_any**       instances = nil;
scl_int         instances_count = 0;
scl_int         instances_cap = 64;

tls scl_any*    stackalloc_arrays = nil;
tls scl_int*    stackalloc_array_sizes = nil;
tls scl_int     stackalloc_arrays_count = 0;
tls scl_int     stackalloc_arrays_cap = 64;
