#include <scale_runtime.h>

typedef struct Struct_SclObject {
	TypeInfo*		type;
} Struct;

typedef Struct* scale_SclObject;

typedef struct Struct_Lock {
	TypeInfo*		type;
	scale_any			mutex;
}* scale_Lock;

typedef struct Struct_Exception {
	Struct rtFields;
	scale_str msg;
	struct Struct_Array* stackTrace;
	scale_str errno_str;
}* scale_Exception;

typedef struct Struct_NullPointerException {
	struct Struct_Exception self;
}* scale_NullPointerException;

typedef struct Struct_IllegalStateException {
	struct Struct_Exception self;
}* scale_IllegalStateException;

typedef struct Struct_Array {
	Struct rtFields;
	scale_any* values;
	scale_int count;
	scale_int capacity;
}* scale_Array;

typedef struct Struct_ReadOnlyArray {
	Struct rtFields;
	scale_any* values;
	scale_int count;
	scale_int capacity;
}* scale_ReadOnlyArray;

typedef struct Struct_Int {
	Struct rtFields;
	scale_int value;
}* scale_Int;

typedef struct Struct_Any {
	Struct rtFields;
	scale_any value;
}* scale_Any;

typedef struct Struct_Float {
	Struct rtFields;
	scale_float value;
}* scale_Float;

typedef struct Struct_AssertError {
	Struct rtFields;
	scale_str msg;
	struct Struct_Array* stackTrace;
	scale_str errno_str;
}* scale_AssertError;

typedef struct Struct_UnreachableError {
	Struct rtFields;
	scale_str msg;
	struct Struct_Array* stackTrace;
	scale_str errno_str;
}* scale_UnreachableError;

typedef struct Struct_Thread {
	const TypeInfo* $type;
	scale_any mutex;
	scale_lambda function;
	scale_any nativeThread;
	scale_str name;
}* scale_Thread;

typedef struct Struct_InvalidArgumentException {
	struct Struct_Exception self;
}* scale_InvalidArgumentException;

typedef struct Struct_Range {
	Struct rtFields;
	scale_int begin;
	scale_int end;
}* scale_Range;

extern scale_Array		Thread$threads;
extern scale_Thread		Thread$mainThread;

tls scale_Thread			_currentThread = nil;

static scale_int count_trace_frames(scale_uint* stack_bottom, scale_uint* stack_top, scale_int iteration_direction) {
	scale_int frames = 0;
	while (stack_top != stack_bottom) {
		if (*stack_top == TRACE_MARKER) {
			frames++;
		}
		stack_top += iteration_direction;
	}
	return frames;
}

scale_str* Process$stackTrace(void) {
	scale_uint* stack_top = (scale_uint*) &stack_top;
	struct GC_stack_base sb;
	GC_get_my_stackbottom(&sb);
	scale_uint* stack_bottom = sb.mem_base;

	scale_int iteration_direction = 1;
	if (stack_top > stack_bottom) {
		iteration_direction = -1;
	}

	scale_int trace_frames = count_trace_frames(stack_bottom, stack_top, iteration_direction);

	scale_str* arr = (scale_str*) scale_new_array_by_size(trace_frames - 1, sizeof(scale_str));

	scale_int i = 0;
	while (stack_top != stack_bottom) {
		if (*stack_top == TRACE_MARKER) {
			if (i) {
				struct scale_backtrace* bt = (struct scale_backtrace*) stack_top;
				arr[i - 1] = str_of_exact(bt->func_name);
			}
			i++;
		}

		stack_top += iteration_direction;
	}
	return arr;
}

scale_bool Process$gcEnabled(void) {
	SCALE_BACKTRACE("Process::gcEnabled(): bool");
	return !GC_is_disabled();
}

void Process$lock(scale_any lock) {
	if (scale_expect(!lock, 0)) return;
	cxx_std_recursive_mutex_lock(&(((scale_Lock) lock)->mutex));
}

void Process$unlock(scale_any lock) {
	if (scale_expect(!lock, 0)) return;
	cxx_std_recursive_mutex_unlock(&(((scale_Lock) lock)->mutex));
}

scale_str intToString(scale_int val) {
	scale_int8* str = (scale_int8*) scale_alloc(32);
	snprintf(str, 32, SCALE_INT_FMT, val);
	scale_str s = str_of_exact(str);
	return s;
}

scale_bool float$isInfinite(scale_float val) {
	return isinf(val);
}

scale_bool float$isNaN(scale_float val) {
	return isnan(val);
}

scale_bool float32$isInfinite(scale_float32 val) {
	return isinf(val);
}

scale_bool float32$isNaN(scale_float32 val) {
	return isnan(val);
}

void Thread$run(scale_Thread self) {
	SCALE_BACKTRACE("Thread:run(): none");
	_currentThread = self;
	scale_set_thread_name(self->name->data);

	Process$lock(Thread$threads);
	virtual_call(Thread$threads, "push(LThread;)V;", void, self);
	Process$unlock(Thread$threads);
	
	TRY {
		(*self->function)(self->function);
	} else {
		scale_runtime_catch(scale_exception_handler.exception);
	}

	Process$lock(Thread$threads);
	virtual_call(Thread$threads, "remove(LThread;)V;", void, self);
	Process$unlock(Thread$threads);
	
	_currentThread = nil;
	scale_set_thread_name(nil);
}

void Thread$start0(scale_Thread self) {
	SCALE_BACKTRACE("Thread:start0(): none");
	if (self->function == nil) {
		scale_IllegalStateException e = ALLOC(IllegalStateException);
		virtual_call(e, "init(s;)V;", void, str_of_exact("Cannot call start on main thread!"));
		scale_throw(e);
	}
	self->nativeThread = scale_thread_start(&Thread$run, self);
}

void Thread$stop0(scale_Thread self) {
	SCALE_BACKTRACE("Thread:stop0(): none");
	if (self->function == nil) {
		scale_IllegalStateException e = ALLOC(IllegalStateException);
		virtual_call(e, "init(s;)V;", void, str_of_exact("Cannot call join on main thread!"));
		scale_throw(e);
	}
	scale_thread_finish(self->nativeThread);
}

void Thread$detach0(scale_Thread self) {
	SCALE_BACKTRACE("Thread:detach0(): none");
	if (self->function == nil) {
		scale_IllegalStateException e = ALLOC(IllegalStateException);
		virtual_call(e, "init(s;)V;", void, str_of_exact("Cannot detach main thread!"));
		scale_throw(e);
	}
	scale_thread_detach(self->nativeThread);
}

scale_Thread Thread$currentThread(void) {
	SCALE_BACKTRACE("Thread::currentThread(): Thread");
	if (!_currentThread) {
		_currentThread = Thread$mainThread;
	}
	return _currentThread;
}

const scale_int8* GarbageCollector$getImplementation0(void) {
	return "Boehm GC";
}

scale_bool GarbageCollector$isPaused0(void) {
	return GC_is_disabled();
}

void GarbageCollector$setPaused0(scale_bool paused) {
	if (paused && !GC_is_disabled()) {
		GC_disable();
	} else if (GC_is_disabled()) {
		GC_enable();
	}
}

void GarbageCollector$run0(void) {
	GC_gcollect();
}

scale_int GarbageCollector$heapSize(void) {
	scale_uint heapSize;
	GC_get_heap_usage_safe(
		&heapSize,
		nil,
		nil,
		nil,
		nil
	);
	return (scale_int) heapSize;
}

scale_int GarbageCollector$freeBytesEstimate(void) {
	scale_uint freeBytes;
	GC_get_heap_usage_safe(
		nil,
		&freeBytes,
		nil,
		nil,
		nil
	);
	return (scale_int) freeBytes;
}

scale_int GarbageCollector$bytesSinceLastCollect(void) {
	scale_uint bytesSinceLastCollect;
	GC_get_heap_usage_safe(
		nil,
		nil,
		&bytesSinceLastCollect,
		nil,
		nil
	);
	return (scale_int) bytesSinceLastCollect;
}

scale_int GarbageCollector$totalMemory(void) {
	scale_uint totalMemory;
	GC_get_heap_usage_safe(
		nil,
		nil,
		nil,
		&totalMemory,
		nil
	);
	return (scale_int) totalMemory;
}

scale_int8* s_strndup(const scale_int8* str, scale_int len) {
	return scale_migrate_foreign_array(str, len, sizeof(scale_int8));
}

scale_int8* s_strdup(const scale_int8* str) {
	return s_strndup(str, strlen(str));
}

scale_int get_errno() {
	return errno;
}

scale_str get_errno_as_str() {
	return str_of_exact(s_strdup(strerror(errno)));
}

scale_any s_memset(scale_any ptr, scale_int32 val, scale_int len) {
	return memset(ptr, val, len);
}

scale_any s_memcpy(scale_any dest, scale_any src, scale_int n) {
	return memcpy(dest, src, n);
}

scale_int8* s_strcpy(scale_int8* dest, scale_int8* src) {
	return s_memcpy(dest, src, strlen(src) + 1);
}

scale_str builtinToString(scale_any obj) {
	if (scale_is_instance(obj)) {
		return virtual_call(obj, "toString()s;", scale_str);
	}
	if (scale_is_array((scale_any*) obj)) {
		return scale_array_to_string((scale_any*) obj);
	}
	return str_of_exact(strformat(SCALE_INT_FMT, (scale_int) obj));
}

scale_str scale_array_to_string(scale_any* arr) {
	if (scale_expect(arr == nil, 0)) {
		scale_runtime_error(EX_BAD_PTR, "nil pointer detected");
	}
	if (scale_expect(!scale_is_array(arr), 0)) {
		scale_runtime_error(EX_INVALID_ARGUMENT, "Array must be initialized with 'new[]')");
	}
	scale_int size = scale_array_size(arr);
	scale_int element_size = scale_array_elem_size(arr);

	scale_str _F4int88toStringbE(scale_int8);
	scale_str _F5int168toStringsE(scale_int16);
	scale_str _F5int328toStringiE(scale_int32);

	scale_int len = 2;
	scale_str element_strs[size];
	for (scale_int i = 0; i < size; i++) {
		switch (element_size) {
			case 1: element_strs[i] = _F4int88toStringbE(((scale_int8*) arr)[i]); break;
			case 2: element_strs[i] = _F5int168toStringsE(((scale_int16*) arr)[i]); break;
			case 4: element_strs[i] = _F5int328toStringiE(((scale_int32*) arr)[i]); break;
			case 8: {
				if (scale_is_instance(arr[i])) {
					element_strs[i] = virtual_call(arr[i], "toString()s;", scale_str);
				} else {
					element_strs[i] = builtinToString(arr[i]);
				}
				break;
			}
			default:
				scale_runtime_error(EX_INVALID_ARGUMENT, "Array element size must be 1, 2, 4 or 8");
		}
		if (i) len += 2;
		len += element_strs[i]->length;
	}
	len++;

	scale_int8* data = scale_new_array_by_size(len, sizeof(scale_int8));
	strcat(data, "[");
	for (scale_int i = 0; i < size; i++) {
		if (i) {
			strcat(data, ", ");
		}
		strcat(data, element_strs[i]->data);
	}
	strcat(data, "]");

	scale_str _F3str2ofcE(scale_int8* data);
	return _F3str2ofcE(data);
}
