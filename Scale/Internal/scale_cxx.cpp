#include <new>
#include <thread>
#include <functional>
#include <mutex>
#include <system_error>

extern "C" {
    #include "scale_runtime.h"
}

#define wrap extern "C"

wrap void cxx_std_thread_join_and_delete(scl_any thread) {
    try {
        ((std::thread*) thread)->join();
        delete ((std::thread*) thread);
    } catch(const std::system_error& e) {
        _scl_runtime_error(EX_THREAD_ERROR, "std::thread::join failed: %s\n", e.what());
    }
}
wrap void cxx_std_thread_detach(scl_any thread) {
    try {
        ((std::thread*) thread)->detach();
    } catch(const std::system_error& e) {
        _scl_runtime_error(EX_THREAD_ERROR, "std::thread::detach failed: %s\n", e.what());
    }
}
wrap scl_any cxx_std_thread_new(void) {
    try {
        return (scl_any) new std::thread();
    } catch(const std::bad_alloc& e) {
        _scl_runtime_error(EX_BAD_PTR, "std::thread::new failed: %s\n", e.what());
    }
}
wrap scl_any cxx_std_thread_new_with_args(scl_any args) {
    try {
        return (scl_any) new std::thread(
            [](scl_any self) {
                struct GC_stack_base stack_bottom;
                stack_bottom.mem_base = (void*) &stack_bottom;
                int ret = GC_register_my_thread(&stack_bottom);
                if (ret != GC_SUCCESS) {
                    _scl_runtime_error(EX_THREAD_ERROR, "GC_register_my_thread failed: %d\n", ret);
                }

                virtual_call(self, "run()V;");

                GC_unregister_my_thread();
            },
            args
        );
    } catch(const std::system_error& e) {
        _scl_runtime_error(EX_THREAD_ERROR, "std::thread::new failed: %s\n", e.what());
    } catch(const std::bad_alloc& e) {
        _scl_runtime_error(EX_BAD_PTR, "std::thread::new failed: %s\n", e.what());
    }
}
struct async_func {
    std::thread thread;
    scl_any ret;
    scl_any args;
    scl_any(*func)(scl_any);
};
wrap scl_any cxx_async(scl_any func, scl_any args) {
    try {
        async_func* a = new async_func;
        a->args = args;
        a->func = (scl_any(*)(scl_any)) func;
        a->thread = std::move(std::thread(
            [](async_func* args) {
                struct GC_stack_base stack_bottom;
                stack_bottom.mem_base = (void*) &stack_bottom;
                int ret = GC_register_my_thread(&stack_bottom);
                if (ret != GC_SUCCESS) {
                    _scl_runtime_error(EX_THREAD_ERROR, "GC_register_my_thread failed: %d\n", ret);
                }

                TRY {
                    args->ret = args->func(args->args);
                    free(args->args);
                } else {
                    free(args->args);
                    _scl_throw(_scl_exception_handler.exception);
                }

                GC_unregister_my_thread();
            },
            a
        ));
        return (scl_any) a;
    } catch(const std::system_error& e) {
        _scl_runtime_error(EX_THREAD_ERROR, "std::thread::new failed: %s\n", e.what());
    } catch(const std::bad_alloc& e) {
        _scl_runtime_error(EX_BAD_PTR, "std::thread::new failed: %s\n", e.what());
    }
}
wrap scl_any cxx_await(scl_any t) {
    async_func* async = (async_func*) t;
    async->thread.join();
    scl_any ret = async->ret;
    delete async;
    return ret;
}
wrap void cxx_std_this_thread_yield(void) {
    std::this_thread::yield();
}

wrap scl_any cxx_std_recursive_mutex_new(void) {
    try {
        return (scl_any) new std::recursive_mutex();
    } catch(const std::system_error& e) {
        _scl_runtime_error(EX_THREAD_ERROR, "std::recursive_mutex::new failed: %s\n", e.what());
    } catch(const std::bad_alloc& e) {
        _scl_runtime_error(EX_BAD_PTR, "std::recursive_mutex::new failed: %s\n", e.what());
    }
}
wrap void cxx_std_recursive_mutex_delete(scl_any* mutex) {
    std::recursive_mutex** x = (std::recursive_mutex**) mutex;
    if (*x) {
        delete *x;
        *x = nullptr;
    }
}
wrap void cxx_std_recursive_mutex_lock(scl_any* mutex) {
    try {
        std::recursive_mutex** x = (std::recursive_mutex**) mutex;
        if (*x == nullptr) {
            *x = (std::recursive_mutex*) cxx_std_recursive_mutex_new();
        }
        (*x)->lock();
    } catch(const std::system_error& e) {
        _scl_runtime_error(EX_THREAD_ERROR, "std::recursive_mutex::lock failed: %s\n", e.what());
    }
}
wrap void cxx_std_recursive_mutex_unlock(scl_any* mutex) {
    std::recursive_mutex** x = (std::recursive_mutex**) mutex;
    if (*x == nullptr) {
        *x = (std::recursive_mutex*) cxx_std_recursive_mutex_new();
    }
    (*x)->unlock();
}
