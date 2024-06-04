#include <new>
#include <thread>
#include <functional>
#include <mutex>
#include <system_error>

extern "C" {
    #include "scale_runtime.h"
}

#define wrap extern "C"

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
