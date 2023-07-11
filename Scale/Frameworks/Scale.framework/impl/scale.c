#include <scale_runtime.h>

extern const ID_t SclObjectHash; // SclObject

typedef struct Struct {
	StaticMembers*	statics;
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
}* _scl_Exception;

typedef struct Struct_NullPointerException {
	struct Struct_Exception self;
} scl_NullPointerException;

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
	struct Struct_Array self;
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
} scl_AssertError;

typedef struct Struct_UnreachableError {
	Struct rtFields;
	scl_str msg;
	struct Struct_Array* stackTrace;
	scl_str errno_str;
} scl_UnreachableError;

struct Struct_str {
	struct scale_string s;
};

typedef struct Struct_Thread {
	Struct rtFields;
	_scl_lambda function;
	pthread_t nativeThread;
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

scl_int8* argv0;

scl_int8* Library$progname(void) {
	return argv0;
}

struct Struct_Array* Process$stackTrace(void) {
	struct Struct_Array* arr = ALLOC(ReadOnlyArray);
	
	arr->capacity = _callstack.ptr;
	arr->initCapacity = _callstack.ptr;
	arr->count = 0;
	arr->values = _scl_new_array((_callstack.ptr) * sizeof(scl_str));
	
	for (scl_int i = 0; i < _callstack.ptr; i++) {
		((scl_str*) arr->values)[(scl_int) arr->count++] = _scl_create_string(_callstack.func[i]);
	}
	return arr;
}

scl_bool Process$gcEnabled(void) {
	return !_scl_gc_is_disabled();
}

#define TO(type, name) scl_ ## type int$to ## name (scl_int val) { return (scl_ ## type) (val & ((1ULL << (sizeof(scl_ ## type) * 8)) - 1)); }
#define VALID(type, name) scl_bool int$isValid ## name (scl_int val) { return val >= SCL_ ## type ## _MIN && val <= SCL_ ## type ## _MAX; }

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshift-count-overflow"
#endif

TO(int8, Int8)
TO(int16, Int16)
TO(int32, Int32)
TO(int64, Int64)
TO(int, Int)
TO(uint8, UInt8)
TO(uint16, UInt16)
TO(uint32, UInt32)
TO(uint64, UInt64)
TO(uint, UInt)

VALID(int8, Int8)
VALID(int16, Int16)
VALID(int32, Int32)
VALID(int64, Int64)
VALID(int, Int)
VALID(uint8, UInt8)
VALID(uint16, UInt16)
VALID(uint32, UInt32)
VALID(uint64, UInt64)
VALID(uint, UInt)

scl_str int$toString(scl_int val) {
	scl_int8* str = _scl_alloc(32);
	snprintf(str, 32, SCL_INT_FMT, val);
	scl_str s = str_of(str);
	return s;
}

scl_str int$toHexString(scl_int val) {
	scl_int8* str = _scl_alloc(19);
	snprintf(str, 19, SCL_INT_HEX_FMT, val);
	scl_str s = str_of(str);
	return s;
}

scl_str int8$toString(scl_int8 val) {
	scl_int8* str = _scl_alloc(5);
	snprintf(str, 5, "%c", val);
	scl_str s = str_of(str);
	return s;
}

void _scl_puts(scl_any val) {
	scl_str s;
	if (_scl_is_instance_of(val, SclObjectHash)) {
		s = virtual_call(val, "toString()s;");
	} else {
		s = int$toString((scl_int) val);
	}
	printf("%s\n", s->_data);
}

void _scl_puts_str(scl_str str) {
	printf("%s\n", str->_data);
}

void _scl_eputs(scl_any val) {
	scl_str s;
	if (_scl_is_instance_of(val, SclObjectHash)) {
		s = virtual_call(val, "toString()s;");
	} else {
		s = int$toString((scl_int) val);
	}
	fprintf(stderr, "%s\n", s->_data);
}

void _scl_write(scl_int fd, scl_str str) {
	write(fd, str->_data, str->_len);
}

scl_str _scl_read(scl_int fd, scl_int len) {
	scl_int8* buf = _scl_alloc(len + 1);
	read(fd, buf, len);
	scl_str str = str_of(buf);
	return str;
}

scl_int _scl_system(scl_str cmd) {
	return system(cmd->_data);
}

scl_str _scl_getenv(scl_str name) {
	scl_int8* val = getenv(name->_data);
	return val ? str_of(val) : nil;
}

scl_float _scl_time(void) {
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (scl_float) tv.tv_sec + (scl_float) tv.tv_usec / 1000000.0;
}

scl_str float$toString(scl_float val) {
	scl_int8* str = _scl_alloc(64);
	snprintf(str, 64, "%f", val);
	scl_str s = str_of(str);
	return s;
}

scl_str float$toPrecisionString(scl_float val) {
	scl_int8* str = _scl_alloc(256);
	snprintf(str, 256, "%.17f", val);
	scl_str s = str_of(str);
	return s;
}

scl_str float$toHexString(scl_float val) {
	return int$toHexString(REINTERPRET_CAST(scl_int, val));
}

scl_float float$fromBits(scl_any bits) {
	return REINTERPRET_CAST(scl_float, bits);
}

scl_bool float$isInfinite(scl_float val) {
	return isinf(val);
}

scl_bool float$isNaN(scl_float val) {
	return isnan(val);
}

void dumpStack(void) {
	printf("Dump:\n");
	for (ssize_t i = 0; i < _stack.ptr; i++) {
		scl_int v = _stack.data[i].i;
		printf("   %zd: 0x" SCL_INT_HEX_FMT ", " SCL_INT_FMT "\n", i, v, v);
	}
	printf("\n");
}

scl_any Library$self0(void) {
	scl_any lib = dlopen(nil, RTLD_NOW | RTLD_GLOBAL);
	if (!lib) {
		scl_NullPointerException* e = ALLOC(NullPointerException);
		virtual_call(e, "init()V;");
		e->self.msg = str_of("Failed to load library");
		_scl_throw(e);
	}
	return lib;
}

scl_any Library$getSymbol0(scl_any lib, scl_int8* name) {
	return dlsym(lib, name);
}

scl_any Library$open0(scl_int8* name) {
	scl_any lib = dlopen(name, RTLD_NOW | RTLD_GLOBAL);
	if (!lib) {
		scl_NullPointerException* e = ALLOC(NullPointerException);
		virtual_call(e, "init()V;");
		e->self.msg = str_of("Failed to load library");
		_scl_throw(e);
	}
	return lib;
}

void Library$close0(scl_any lib) {
	dlclose(lib);
}

scl_str Library$dlerror0(void) {
	return str_of(dlerror());
}

static struct Struct_Int _ints[256] = {};

scl_Int Int$valueOf(scl_int val) {
	if (val >= -128 && val <= 127) {
		return &_ints[val + 128];
	}
	scl_Int new = ALLOC(Int);
	new->value = val;
	return new;
}

scl_Any Any$valueOf(scl_any val) {
	scl_Any new = ALLOC(Any);
	new->value = val;
	return new;
}

scl_Float Float$valueOf(scl_float val) {
	scl_Float new = ALLOC(Float);
	new->value = val;
	return new;
}

void Thread$run(scl_Thread self) {
	_scl_stack_new();
	*(_scl_callstack_push()) = "<extern Thread:run(): none>";
	_currentThread = self;

	virtual_call(Var_Thread$threads, "push(a;)V;", self);
	
	self->function();

	virtual_call(Var_Thread$threads, "remove(a;)V;", self);
	
	_currentThread = nil;
	_callstack.ptr--;
	_scl_stack_free();
}

scl_int Thread$start0(scl_Thread self) {
	int ret = _scl_gc_pthread_create(&self->nativeThread, 0, (scl_any) Thread$run, self);
	return ret;
}

scl_int Thread$stop0(scl_Thread self) {
	int ret = _scl_gc_pthread_join(self->nativeThread, 0);
	return ret;
}

scl_Thread Thread$currentThread(void) {
	if (!_currentThread) {
		_currentThread = ALLOC(Thread);
		_currentThread->name = str_of("Main Thread");
		_currentThread->nativeThread = pthread_self();
		_currentThread->function = nil;
	}
	return _currentThread;
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

_scl_constructor
void _scale_framework_init(void) {
    _scl_setup();
	
    extern const StaticMembers _scl_statics_Int;

	ID_t intHash = id("Int");
	for (scl_int i = -128; i < 127; i++) {
		_ints[i + 128] = (struct Struct_Int) {
			.rtFields = {
				.statics = (StaticMembers*) &_scl_statics_Int,
				.mutex = _scl_mutex_new(),
			},
			.value = i
		};
		_scl_add_struct((Struct*) &_ints[i + 128]);
	}

	Var_Thread$mainThread = _currentThread = Thread$currentThread();
}