#include <new>
#include <thread>
#include <functional>
#include <mutex>
#include <system_error>

extern "C" {
    #include "scale_runtime.h"
}

#define wrap extern "C"

wrap void cxx_std_thread_join(scl_any thread) {
    try {
        ((std::thread*) thread)->join();
    } catch(const std::system_error& e) {
        _scl_runtime_error(EX_THREAD_ERROR, "std::thread::join failed: %s\n", e.what());
    }
}
wrap void cxx_std_thread_delete(scl_any thread) {
    delete (std::thread*) thread;
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
wrap void cxx_std_recursive_mutex_delete(scl_any mutex) {
    delete (std::recursive_mutex*) mutex;
}
wrap void cxx_std_recursive_mutex_lock(scl_any mutex) {
    try {
        ((std::recursive_mutex*) mutex)->lock();
    } catch(const std::system_error& e) {
        _scl_runtime_error(EX_THREAD_ERROR, "std::recursive_mutex::lock failed: %s\n", e.what());
    }
}
wrap void cxx_std_recursive_mutex_unlock(scl_any mutex) {
    ((std::recursive_mutex*) mutex)->unlock();
}
