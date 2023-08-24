#include <new>
#include <thread>
#include <functional>
#include <mutex>

extern "C" {
    #include "scale_runtime.h"
}

#define wrap extern "C"

typedef struct Struct_SclObject {
	_scl_lambda*	$fast;
	TypeInfo*		$statics;
	mutex_t			$mutex;
}* scl_SclObject;

typedef struct Struct_Exception {
	struct Struct_SclObject _super;
	scl_str msg;
	scl_any stackTrace;
	scl_str errno_str;
}* scl_Exception;

typedef struct Struct_ThreadException {
    struct Struct_SclObject _super;
	scl_str msg;
	scl_any stackTrace;
	scl_str errno_str;
}* scl_ThreadException;

typedef struct Struct_MutexError {
    struct Struct_SclObject _super;
    scl_str msg;
    scl_any stackTrace;
    scl_str errno_str;
}* scl_MutexError;

typedef struct Struct_OutOfMemoryError {
    struct Struct_SclObject _super;
    scl_str msg;
    scl_any stackTrace;
    scl_str errno_str;
}* scl_OutOfMemoryError;

// Constructs a new instance of the given Scale exception from the given C++ exception.
#define INTO(_scl_ex, _cxx_ex) ({ \
    scl_##_scl_ex _e  = ALLOC(_scl_ex); \
    virtual_call(_e, "init()V;"); \
    _e ->msg = str_of((_cxx_ex).what()); \
    _e ; \
})

wrap void cxx_std_thread_join(scl_any thread) {
    try {
        ((std::thread*) thread)->join();
    } catch(const std::system_error& e) {
        _scl_throw(INTO(ThreadException, e));
    }
}
wrap void cxx_std_thread_delete(scl_any thread) {
    delete (std::thread*) thread;
}
wrap void cxx_std_thread_detach(scl_any thread) {
    try {
        ((std::thread*) thread)->detach();
    } catch(const std::system_error& e) {
        _scl_throw(INTO(ThreadException, e));
    }
}
wrap scl_any cxx_std_thread_new() {
    try {
        return (scl_any) new std::thread();
    } catch(const std::bad_alloc& e) {
        _scl_throw(INTO(OutOfMemoryError, e));
    }
}
wrap scl_any cxx_std_thread_new_with_args(scl_any Thread$run, scl_any args) {
    try {
        return (scl_any) new std::thread(
            [Thread$run](scl_any self) {
                struct GC_stack_base stack_bottom;
                stack_bottom.mem_base = (void*) &stack_bottom;
                int ret = GC_register_my_thread(&stack_bottom);
                if (ret != GC_SUCCESS) {
                    fprintf(stderr, "GC_register_my_thread failed: %d\n", ret);
                    exit(1);
                }

                ((void(*)(scl_any)) Thread$run)(self);

                GC_unregister_my_thread();
            },
            args
        );
    } catch(const std::exception& e) {
        _scl_throw(INTO(ThreadException, e));
    }
}
wrap void cxx_std_this_thread_yield() {
    std::this_thread::yield();
}

wrap scl_any cxx_std_recursive_mutex_new() {
    try {
        return (scl_any) new std::recursive_mutex();
    } catch(const std::system_error& e) {
        _scl_throw(INTO(MutexError, e));
    } catch(const std::bad_alloc& e) {
        _scl_throw(INTO(OutOfMemoryError, e));
    }
}
wrap void cxx_std_recursive_mutex_delete(scl_any mutex) {
    delete (std::recursive_mutex*) mutex;
}
wrap void cxx_std_recursive_mutex_lock(scl_any mutex) {
    try {
        ((std::recursive_mutex*) mutex)->lock();
    } catch(const std::system_error& e) {
        _scl_throw(INTO(MutexError, e));
    }
}
wrap void cxx_std_recursive_mutex_unlock(scl_any mutex) {
    ((std::recursive_mutex*) mutex)->unlock();
}
