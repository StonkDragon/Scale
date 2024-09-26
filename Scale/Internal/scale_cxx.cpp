#include <new>
#include <thread>
#include <functional>
#include <mutex>
#include <system_error>

extern "C" {
    #include "scale_runtime.h"
}

#define wrap extern "C"

wrap scale_any cxx_std_recursive_mutex_new(void) {
    try {
        return (scale_any) new std::recursive_mutex();
    } catch(const std::system_error& e) {
        scale_runtime_error(EX_THREAD_ERROR, "std::recursive_mutex::new failed: %s\n", e.what());
    } catch(const std::bad_alloc& e) {
        scale_runtime_error(EX_BAD_PTR, "std::recursive_mutex::new failed: %s\n", e.what());
    }
}
wrap void cxx_std_recursive_mutex_delete(scale_any* mutex) {
    std::recursive_mutex** x = (std::recursive_mutex**) mutex;
    if (*x) {
        delete *x;
        *x = nullptr;
    }
}
wrap void cxx_std_recursive_mutex_lock(scale_any* mutex) {
    try {
        std::recursive_mutex** x = (std::recursive_mutex**) mutex;
        if (*x == nullptr) {
            *x = (std::recursive_mutex*) cxx_std_recursive_mutex_new();
        }
        (*x)->lock();
    } catch(const std::system_error& e) {
        scale_runtime_error(EX_THREAD_ERROR, "std::recursive_mutex::lock failed: %s\n", e.what());
    }
}
wrap void cxx_std_recursive_mutex_unlock(scale_any* mutex) {
    std::recursive_mutex** x = (std::recursive_mutex**) mutex;
    if (*x == nullptr) {
        *x = (std::recursive_mutex*) cxx_std_recursive_mutex_new();
    }
    (*x)->unlock();
}
