#pragma once
#ifndef CUSTOM_OPENCL_HPP
#define CUSTOM_OPENCL_HPP

// #include <CL_2_0/CL/cl.hpp>
#include <CL/cl.hpp>

struct My_Functor_Prototype {
    virtual void operator(const float *A, const float *B, float *C) = 0;
    virtual ~My_Functor_Prototype() = default;
};

extern std::unique_ptr<My_Functor_Prototype> functor_factory(const std::string &type);

// // Terribly bad designed, do not use outside the scope of this little project
// template <class T>
// class auto_ptr {
// private:
//     T *value;

// public:
//     // Construct in place (does not work with initialization lists)
//     template <class Args...>
//     static make(Args... args) {
//         return auto_ptr(new T(std::forward<Args>(args)...));
//     }

//     // Construct from pointer (must have a default deleter)
//     auto_ptr(T *value) : value(value) {}

//     // No copies!
//     auto_ptr(const auto_ptr &) = delete;
//     auto_ptr &operator=(const auto_ptr &) = delete;

//     // Yes moves!
//     auto_ptr(auto_ptr &&rhs) : value(rhs.value) {
//         rhs.value = nullptr;
//     }
//     auto_ptr &operator=(auto_ptr &&rhs) {
//         this->value = rhs.value;
//         rhs.value = nullptr;
//         return *this;
//     }

//     // Auto delete pointed value
//     ~auto_ptr() {
//         if (value != nullptr) {
//             delete value;
//         }
//         value = nullptr;
//     }

//     T &operator*() {
//         if (!value) {
//             throw std::runtime_error("Dereferencing nullptr!");
//         }

//         return *value;
//     }

//     T *operator->() {
//         if (!value) {
//             throw std::runtime_error("Dereferencing nullptr!");
//         }

//         return value;
//     }
// };

#endif // CUSTOM_OPENCL_HPP
