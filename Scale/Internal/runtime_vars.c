#include <scale_runtime.h>

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

scl_any**       structs = nil;
scl_int         structs_count = 0;
scl_int         structs_cap = 64;

int             printingStacktrace = 0;
