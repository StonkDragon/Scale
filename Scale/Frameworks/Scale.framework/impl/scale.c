#include <scale_runtime.h>

ID_t SclObjectHash; // SclObject

typedef struct Struct {
	_scl_lambda*	vtable;
	TypeInfo*		statics;
	mutex_t			mutex;
} Struct;

typedef struct Struct_SclObject {
	Struct rtFields;
}* scl_SclObject;

typedef struct Struct_Exception {
	Struct rtFields;
	scl_str msg;
	struct Struct_Array* stackTrace;
	scl_str errno_str;
}* scl_Exception;

typedef struct Struct_NullPointerException {
	struct Struct_Exception self;
}* scl_NullPointerException;

typedef struct Struct_IllegalStateException {
	struct Struct_Exception self;
}* scl_IllegalStateException;

typedef struct Struct_Array {
	Struct rtFields;
	scl_any* values;
	scl_int count;
	scl_int capacity;
	scl_int initCapacity;
}* scl_Array;

typedef struct Struct_ReadOnlyArray {
	Struct rtFields;
	scl_any* values;
	scl_int count;
	scl_int capacity;
	scl_int initCapacity;
}* scl_ReadOnlyArray;

typedef struct Struct_IndexOutOfBoundsException {
	struct Struct_Exception self;
}* scl_IndexOutOfBoundsException;

typedef struct Struct_Int {
	Struct rtFields;
	scl_int value;
}* scl_Int;

typedef struct Struct_Any {
	Struct rtFields;
	scl_any value;
}* scl_Any;

typedef struct Struct_Float {
	Struct rtFields;
	scl_float value;
}* scl_Float;

typedef struct Struct_AssertError {
	Struct rtFields;
	scl_str msg;
	struct Struct_Array* stackTrace;
	scl_str errno_str;
}* scl_AssertError;

typedef struct Struct_UnreachableError {
	Struct rtFields;
	scl_str msg;
	struct Struct_Array* stackTrace;
	scl_str errno_str;
}* scl_UnreachableError;

struct Struct_str {
	struct scale_string s;
};

typedef struct Struct_Thread {
	Struct rtFields;
	_scl_lambda function;
	_scl_thread_t nativeThread;
	scl_str name;
}* scl_Thread;

typedef struct Struct_InvalidArgumentException {
	struct Struct_Exception self;
}* scl_InvalidArgumentException;

extern scl_Array        Var_Thread$threads;
extern scl_Thread       Var_Thread$mainThread;
extern tls scl_Thread   _currentThread;

extern scl_any*         arrays;
extern scl_int          arrays_count;
extern scl_int          arrays_capacity;

extern _scl_stack_t**	stacks;
extern scl_int			stacks_count;
extern scl_int			stacks_cap;

extern scl_any*			allocated;
extern scl_int			allocated_count;
extern scl_int			allocated_cap;

extern scl_int*			memsizes;
extern scl_int			memsizes_count;
extern scl_int			memsizes_cap;

extern Struct**			instances;
extern scl_int			instances_count;
extern scl_int			instances_cap;

extern tls scl_any*		stackalloc_arrays;
extern tls scl_int*		stackalloc_array_sizes;
extern tls scl_int		stackalloc_arrays_count;
extern tls scl_int		stackalloc_arrays_cap;

scl_Array Process$stackTrace(void) {
	scl_ReadOnlyArray arr = ALLOC(ReadOnlyArray);
	
	const scl_int8** tmp;
	const scl_int8** _callstack = tmp = _stack.tbp;

	arr->count = (_stack.tp - _stack.tbp) - 1;

	arr->initCapacity = arr->count;
	arr->capacity = arr->count;
	arr->values = (scl_any*) _scl_new_array_by_size(arr->capacity, sizeof(scl_int8*));

	for (scl_int i = 0; i < arr->count; i++) {
		_scl_array_check_bounds_or_throw(arr->values, i);
		arr->values[i] = str_of_exact(_callstack[i]);
	}

	return (scl_Array) arr;
}

scl_bool Process$gcEnabled(void) {
	SCL_BACKTRACE("Process:gcEnabled(): bool");
	return !_scl_gc_is_disabled();
}

scl_any* Process$stackPointer() {
	SCL_BACKTRACE("Process:stackPointer(): [any]");
	return (scl_any*) _stack.sp;
}

scl_any* Process$basePointer() {
	SCL_BACKTRACE("Process:basePointer(): [any]");
	return (scl_any*) _stack.bp;
}

scl_str intToString(scl_int val) {
	scl_int8* str = (scl_int8*) _scl_alloc(32);
	snprintf(str, 32, SCL_INT_FMT, val);
	scl_str s = str_of(str);
	return s;
}

scl_bool float$isInfinite(scl_float val) {
	return isinf(val);
}

scl_bool float$isNaN(scl_float val) {
	return isnan(val);
}

void dumpStack(void) {
	printf("Dump:\n");
	_scl_frame_t* frame = _stack.bp;
	while (frame != _stack.sp) {
		printf("   %zd: 0x" SCL_INT_HEX_FMT ", " SCL_INT_FMT "\n", (frame - _stack.bp) / sizeof(scl_any), frame->i, frame->i);
	}
	printf("\n");
}

void Thread$run(scl_Thread self) {
	_scl_stack_new();
	SCL_BACKTRACE("Thread:run(): none");
	_currentThread = self;

	Process$lock(Var_Thread$threads);
	virtual_call(Var_Thread$threads, "push(LThread;)V;", self);
	Process$unlock(Var_Thread$threads);
	
	_scl_exception_push();
	TRY {
		self->function();
	} else {
		_scl_runtime_catch();
	}

	Process$lock(Var_Thread$threads);
	virtual_call(Var_Thread$threads, "remove(LThread;)V;", self);
	Process$unlock(Var_Thread$threads);
	
	_currentThread = nil;
	_scl_stack_free();
}

void Thread$start0(scl_Thread self) {
	SCL_BACKTRACE("Thread:start0(): none");
	_scl_thread_new(&self->nativeThread, (scl_any(*)(scl_any)) &Thread$run, self);
}

void Thread$stop0(scl_Thread self) {
	SCL_BACKTRACE("Thread:stop0(): none");
	_scl_thread_wait_for(self->nativeThread);
}

void Thread$detach0(scl_Thread self) {
	SCL_BACKTRACE("Thread:detach0(): none");
	_scl_thread_detach(self->nativeThread);
}

scl_Thread Thread$currentThread(void) {
	SCL_BACKTRACE("Thread:currentThread(): Thread");
	if (!_currentThread) {
		_currentThread = ALLOC(Thread);
		_currentThread->name = str_of_exact("Main Thread");
		_currentThread->nativeThread = _scl_thread_current();
		_currentThread->function = nil;
	}
	return _currentThread;
}

_scl_constructor
void _scale_framework_init(void) {
    _scl_setup();

	Var_Thread$mainThread = _currentThread = Thread$currentThread();
}
