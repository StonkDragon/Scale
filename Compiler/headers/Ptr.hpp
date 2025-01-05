#pragma once

template<typename T>
using Ptr = T*;

// template<typename T>
// class Ptr {
//     T* ptr;
//     long long* refCount;

// public:
//     Ptr(T* ptr) : ptr(ptr), refCount(new long long(1)) {}
//     ~Ptr() {
//         if (--*refCount == 0) {
//             delete ptr;
//             delete refCount;
//         }
//     }

//     Ptr(const Ptr<T>& other) : ptr(other.ptr), refCount(other.refCount) {
//         (*refCount)++;
//     }

//     Ptr<T>& operator=(const Ptr<T>& other) {
//         if (this == &other) return *this;
//         if (--*refCount == 0) {
//             delete ptr;
//             delete refCount;
//         }
//         ptr = other.ptr;
//         refCount = other.refCount;
//         (*refCount)++;
//         return *this;
//     }

//     Ptr<T>& operator=(T* other) {
//         if (--*refCount == 0) {
//             delete ptr;
//             delete refCount;
//         }
//         ptr = other;
//         refCount = new long long(1);
//         return *this;
//     }

//     template<typename U>
//     Ptr(const Ptr<U>& other) : ptr(other.get()), refCount(other.getRefCount()) {
//         (*refCount)++;
//     }

//     template<typename U>
//     Ptr<T>& operator=(const Ptr<U>& other) {
//         if (this == &other) return *this;
//         if (--*refCount == 0) {
//             delete ptr;
//             delete refCount;
//         }
//         ptr = other.get();
//         refCount = other.getRefCount();
//         (*refCount)++;
//         return *this;
//     }

//     template<typename U>
//     Ptr<T>& operator=(U* other) {
//         if (--*refCount == 0) {
//             delete ptr;
//             delete refCount;
//         }
//         ptr = other;
//         refCount = new long long(1);
//         return *this;
//     }

//     T* operator->() {
//         return ptr;
//     }

//     T& operator*() {
//         return *ptr;
//     }

//     T* get() {
//         return ptr;
//     }

//     long long getRefCount() {
//         return *refCount;
//     }

//     bool operator==(const Ptr<T>& other) {
//         return ptr == other.ptr;
//     }

//     bool operator!=(const Ptr<T>& other) {
//         return ptr != other.ptr;
//     }

//     bool operator==(T* other) {
//         return ptr == other;
//     }

//     bool operator!=(T* other) {
//         return ptr != other;
//     }

//     operator bool() {
//         return ptr != nullptr;
//     }
// };
