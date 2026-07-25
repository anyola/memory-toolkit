#ifndef UNIQUE_PTR_HPP
#define UNIQUE_PTR_HPP

#include <cstddef>
#include <utility>
#include <type_traits>

#include "default_delete.hpp"

namespace mtk {
    template<typename T, typename Deleter = default_delete<T>>
    class unique_ptr{
    private:
        T* ptr;
        Deleter deleter;
    public:
    unique_ptr() noexcept : ptr(nullptr) {}
    unique_ptr(std::nullptr_t) : ptr(nullptr) {}
    explicit unique_ptr(T* p) : ptr(p) {}
    unique_ptr(T* p, const Deleter& d) : ptr(p), deleter(d) {}
    unique_ptr(T* p, Deleter&& d) : ptr(p), deleter(std::move(d)) {}
    unique_ptr(const unique_ptr&) = delete;
    unique_ptr& operator=(const unique_ptr&) = delete;
    unique_ptr(unique_ptr&& other) noexcept : ptr(other.ptr), deleter(std::move(other.deleter)) {
        other.ptr = nullptr;
    }
    unique_ptr& operator=(unique_ptr&& other) {
        if(this != &other){
            if(ptr != nullptr){
                deleter(ptr);
            }
            ptr = other.ptr;
            other.ptr = nullptr;
            deleter = std::move(other.deleter);
        }
        return *this;
    }
    unique_ptr& operator=(std::nullptr_t) {
        reset();
        return *this;
    }
    ~unique_ptr() {
        if(ptr != nullptr) {
            deleter(ptr);
        }
    }
    [[nodiscard]] T* release() noexcept {
        T* temp = ptr;
        ptr = nullptr;
        return temp;
    }
    void reset(T* new_ptr = nullptr) noexcept {
        if(new_ptr != ptr){
            if(ptr != nullptr){
                deleter(ptr);
            }
            ptr = new_ptr;
        }
    }
    [[nodiscard]] T* get() const noexcept {
        return ptr;
    }
    T& operator*() const {
        return *ptr;
    }
    T* operator->() const noexcept {
        return ptr;
    }
    explicit operator bool() const noexcept {
        if(ptr == nullptr) {
            return false;
        }
        return true;
    }
    friend bool operator==(const unique_ptr& lhs, const unique_ptr& rhs) {
        if(lhs.get() == rhs.get()){
            return true;
        }
        return false;
    }
    friend bool operator!=(const unique_ptr& lhs, const unique_ptr& rhs) {
        return !(lhs.get() == rhs.get());
    }
    friend bool operator==(const unique_ptr& p, std::nullptr_t) {
        return p.get() == nullptr;
    }
    friend bool operator==(std::nullptr_t, const unique_ptr& p) {
        return p.get() == nullptr;
    }
    friend bool operator!=(const unique_ptr& p, std::nullptr_t) {
        return !(p.get() == nullptr);
    }
    friend bool operator!=(std::nullptr_t, const unique_ptr& p) {
        return !(p.get() == nullptr);
    }
    friend void swap(unique_ptr& lhs , unique_ptr& rhs) noexcept {
        std::swap(lhs.ptr, rhs.ptr);
        std::swap(lhs.deleter, rhs.deleter);
    }
    };

    template<typename T, typename ... Args>
    typename std::enable_if<!std::array<T>::value, unique_ptr<T>>::type
    [[nodiscard]] unique_ptr<T> make_unique(Args&&... args){
        return unique_ptr<T>(new T(std::forward<Args>(args)...));
    }


    template<typename T, typename Deleter = default_delete<T[]>>
    class unique_ptr<T[], Deleter> {
    private:
        T* ptr;
        Deleter deleter;

    public:
        unique_ptr() noexcept : ptr(nullptr) {}
        unique_ptr(std::nullptr_t) : ptr(nullptr) {}
        explicit unique_ptr(T* p) : ptr(p) {}
        unique_ptr(T* p, const Deleter& d) : ptr(p), deleter(d) {}
        unique_ptr(T* p, Deleter&& d) : ptr(p), deleter(std::move(d)) {}
        unique_ptr(const unique_ptr&) = delete;
        unique_ptr& operator=(const unique_ptr&) = delete;
        unique_ptr(unique_ptr&& other) noexcept : ptr(other.ptr), deleter(std::move(other.deleter)) {
            other.ptr = nullptr;
        }
        unique_ptr& operator=(unique_ptr&& other) {
            if(this != &other){
                if(ptr != nullptr){
                    deleter(ptr);
                }
                ptr = other.ptr;
                other.ptr = nullptr;
                deleter = std::move(other.deleter);
            }
            return *this;
        }
        unique_ptr& operator=(std::nullptr_t) {
            reset();
            return *this;
        }
        ~unique_ptr() {
            if(ptr != nullptr) {
                deleter(ptr);
            }
        }
        [[nodiscard]] T* release() noexcept {
            T* temp = ptr;
            ptr = nullptr;
            return temp;
        }
        void reset(T* new_ptr = nullptr) noexcept {
            if(new_ptr != ptr){
                if(ptr != nullptr){
                    deleter(ptr);
                }
                ptr = new_ptr;
            }
        }
        [[nodiscard]] T* get() const noexcept {
            return ptr;
        }
        T& operator[](std::size_t i) const {
            return ptr[i];
        }
        explicit operator bool() const noexcept {
            if(ptr == nullptr) {
                return false;
            }
            return true;
        }
        friend bool operator==(const unique_ptr& lhs, const unique_ptr& rhs) {
            if(lhs.get() == rhs.get()){
                return true;
            }
            return false;
        }
        friend bool operator!=(const unique_ptr& lhs, const unique_ptr& rhs) {
            return !(lhs.get() == rhs.get());
        }
        friend void swap(unique_ptr& lhs , unique_ptr& rhs) noexcept {
            std::swap(lhs.ptr, rhs.ptr);
            std::swap(lhs.deleter, rhs.deleter);
        }
        friend bool operator==(const unique_ptr& p, std::nullptr_t) {
            return p.get() == nullptr;
        }
        friend bool operator==(std::nullptr_t, const unique_ptr& p) {
            return p.get() == nullptr;
        }
        friend bool operator!=(const unique_ptr& p, std::nullptr_t) {
            return !(p.get() == nullptr);
        }
        friend bool operator!=(std::nullptr_t, const unique_ptr& p) {
            return !(p.get() == nullptr);
        }

    };
    template<typename T>
    [[nodiscard]] typename std::enable_if<std::is_array<T>::value, unique_ptr<T>>::type
    make_unique(std::size_t n) {
        using U = typename std::remove_extent<T>::type;
        return unique_ptr<T>(new U[n]());
    }
}


#endif