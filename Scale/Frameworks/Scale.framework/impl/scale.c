#include <scale_runtime.h>

ID_t SclObjectHash; // SclObject

typedef struct Struct {
	_scl_lambda*	vtable;
	TypeInfo*		statics;
	scl_any			mutex;
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
	scl_any nativeThread;
	scl_str name;
}* scl_Thread;

typedef struct Struct_InvalidArgumentException {
	struct Struct_Exception self;
}* scl_InvalidArgumentException;

extern scl_Array		Var_Thread$threads;
extern scl_Thread		Var_Thread$mainThread;

tls scl_Thread			_currentThread = nil;

_scl_symbol_hidden
static scl_int count_trace_frames(scl_uint* stack_bottom, scl_uint* stack_top, scl_int iteration_direction) {
	scl_int frames = 0;
	while (stack_top != stack_bottom) {
		if (*stack_top == TRACE_MARKER) {
			frames++;
		}
		stack_top += iteration_direction;
	}
	return frames;
}

scl_Array Process$stackTrace(void) {
	scl_uint* stack_top = (scl_uint*) &stack_top;
	struct GC_stack_base sb;
	GC_get_my_stackbottom(&sb);
	scl_uint* stack_bottom = sb.mem_base;

	scl_int iteration_direction = 1;
	if (stack_top > stack_bottom) {
		iteration_direction = -1;
	}

	scl_ReadOnlyArray arr = ALLOC(ReadOnlyArray);
	arr->count = count_trace_frames(stack_bottom, stack_top, iteration_direction);

	arr->values = (scl_any*) _scl_new_array_by_size(arr->count, sizeof(scl_int8*));
	arr->initCapacity = arr->count;
	arr->capacity = arr->count;

	scl_int i = 0;
	while (stack_top != stack_bottom) {
		if (*stack_top == TRACE_MARKER) {
			if (i) {
				struct _scl_backtrace* bt = (struct _scl_backtrace*) stack_top;
				arr->values[i] = str_of_exact(bt->func_name);
			}
			i++;
		}

		stack_top += iteration_direction;
	}

	scl_int8* tmp;
	for (scl_int i = 0; i < arr->count / 2; i++) {
		tmp = arr->values[i];
		arr->values[i] = arr->values[arr->count - i - 1];
		arr->values[arr->count - i - 1] = tmp;
	}
	arr->count--;

	return (scl_Array) arr;
}

scl_bool Process$gcEnabled(void) {
	SCL_BACKTRACE("Process::gcEnabled(): bool");
	return !GC_is_disabled();
}

void Process$lock(scl_any obj) {
	if (_scl_expect(!obj, 0)) return;
	cxx_std_recursive_mutex_lock(((Struct*) obj)->mutex);
}

void Process$unlock(scl_any obj) {
	if (_scl_expect(!obj, 0)) return;
	cxx_std_recursive_mutex_unlock(((Struct*) obj)->mutex);
}

scl_str intToString(scl_int val) {
	scl_int8* str = (scl_int8*) _scl_alloc(32);
	snprintf(str, 32, SCL_INT_FMT, val);
	scl_str s = str_of_exact(str);
	return s;
}

scl_bool float$isInfinite(scl_float val) {
	return isinf(val);
}

scl_bool float$isNaN(scl_float val) {
	return isnan(val);
}

void Thread$run(scl_Thread self) {
	SCL_BACKTRACE("Thread:run(): none");
	_currentThread = self;

	Process$lock(Var_Thread$threads);
	virtual_call(Var_Thread$threads, "push(LThread;)V;", self);
	Process$unlock(Var_Thread$threads);
	
	TRY {
		self->function();
	} else {
		_scl_runtime_catch(_scl_exception_handler.exception);
	}

	Process$lock(Var_Thread$threads);
	virtual_call(Var_Thread$threads, "remove(LThread;)V;", self);
	Process$unlock(Var_Thread$threads);
	
	_currentThread = nil;
}

void Thread$start0(scl_Thread self) {
	SCL_BACKTRACE("Thread:start0(): none");
	self->nativeThread = cxx_std_thread_new_with_args(self);
}

void Thread$stop0(scl_Thread self) {
	SCL_BACKTRACE("Thread:stop0(): none");
	cxx_std_thread_join(self->nativeThread);
	cxx_std_thread_delete(self->nativeThread);
}

void Thread$detach0(scl_Thread self) {
	SCL_BACKTRACE("Thread:detach0(): none");
	cxx_std_thread_detach(self->nativeThread);
}

scl_Thread Thread$currentThread(void) {
	SCL_BACKTRACE("Thread::currentThread(): Thread");
	if (!_currentThread) {
		_currentThread = ALLOC(Thread);
		_currentThread->name = str_of_exact("Main Thread");
		_currentThread->nativeThread = cxx_std_thread_new();
		_currentThread->function = nil;
	}
	return _currentThread;
}

const scl_int8* GarbageCollector$getImplementation0(void) {
	return "Boehm GC";
}

scl_bool GarbageCollector$isPaused0(void) {
	return GC_is_disabled();
}

void GarbageCollector$setPaused0(scl_bool paused) {
	if (paused && !GC_is_disabled()) {
		GC_disable();
	} else if (GC_is_disabled()) {
		GC_enable();
	}
}

void GarbageCollector$run0(void) {
	GC_gcollect();
}

scl_int GarbageCollector$heapSize(void) {
	scl_uint heapSize;
	GC_get_heap_usage_safe(
		&heapSize,
		nil,
		nil,
		nil,
		nil
	);
	return (scl_int) heapSize;
}

scl_int GarbageCollector$freeBytesEstimate(void) {
	scl_uint freeBytes;
	GC_get_heap_usage_safe(
		nil,
		&freeBytes,
		nil,
		nil,
		nil
	);
	return (scl_int) freeBytes;
}

scl_int GarbageCollector$bytesSinceLastCollect(void) {
	scl_uint bytesSinceLastCollect;
	GC_get_heap_usage_safe(
		nil,
		nil,
		&bytesSinceLastCollect,
		nil,
		nil
	);
	return (scl_int) bytesSinceLastCollect;
}

scl_int GarbageCollector$totalMemory(void) {
	scl_uint totalMemory;
	GC_get_heap_usage_safe(
		nil,
		nil,
		nil,
		&totalMemory,
		nil
	);
	return (scl_int) totalMemory;
}

scl_int8* s_strndup(const scl_int8* str, scl_int len) {
	return _scl_migrate_foreign_array(str, len, sizeof(scl_int8));
}

scl_int8* s_strdup(const scl_int8* str) {
	return s_strndup(str, strlen(str));
}

scl_int get_errno() {
	return errno;
}

scl_str get_errno_as_str() {
	return str_of_exact(s_strdup(strerror(errno)));
}

scl_any s_memset(scl_any ptr, scl_int32 val, scl_int len) {
	return memset(ptr, val, len);
}

scl_any s_memcpy(scl_any dest, scl_any src, scl_int n) {
	return memcpy(dest, src, n);
}

scl_int8* s_strcpy(scl_int8* dest, scl_int8* src) {
	return s_memcpy(dest, src, strlen(src) + 1);
}

scl_str builtinToString(scl_any obj) {
	if (_scl_is_instance(obj)) {
		return (scl_str) virtual_call(obj, "toString()s;");
	}
	if (_scl_is_array((scl_any*) obj)) {
		return _scl_array_to_string((scl_any*) obj);
	}
	scl_int8* data = (scl_int8*) _scl_alloc(32);
	snprintf(data, 31, SCL_INT_FMT, (scl_int) obj);
	return str_of_exact(data);
}

scl_str _scl_array_to_string(scl_any* arr) {
	if (_scl_expect(arr == nil, 0)) {
		_scl_runtime_error(EX_BAD_PTR, "nil pointer detected");
	}
	if (_scl_expect(!_scl_is_array(arr), 0)) {
		_scl_runtime_error(EX_INVALID_ARGUMENT, "Array must be initialized with 'new[]')");
	}
	scl_int size = _scl_array_size(arr);
	scl_int element_size = _scl_array_elem_size(arr);
	scl_str s = str_of_exact("[");
	for (scl_int i = 0; i < size; i++) {
		if (i) {
			s = (scl_str) virtual_call(s, "append(s;)s;", str_of_exact(", "));
		}
		scl_str tmp = nil;
		scl_int value;
		switch (element_size) {
			case 1:
				value = ((scl_int8*) arr)[i];
				break;
			case 2:
				value = ((scl_int16*) arr)[i];
				break;
			case 4:
				value = ((scl_int32*) arr)[i];
				break;
			case 8: {
				if (_scl_is_instance(arr[i])) {
					tmp = (scl_str) virtual_call(arr[i], "toString()s;");
				} else {
					value = ((scl_int*) arr)[i];
				}
				break;
			}
			default:
				_scl_runtime_error(EX_INVALID_ARGUMENT, "Array element size must be 1, 2, 4 or 8");
		}
		if (tmp == nil) {
			scl_int8* str = (scl_int8*) _scl_alloc(32);
			snprintf(str, 31, SCL_INT_FMT, value);
			tmp = str_of_exact(str);
		}
		s = (scl_str) virtual_call(s, "append(s;)s;", tmp);
	}
	return (scl_str) virtual_call(s, "append(s;)s;", str_of_exact("]"));
}

_scl_constructor
void _scale_framework_init(void) {
    _scl_setup();

	Var_Thread$mainThread = _currentThread = Thread$currentThread();
}
