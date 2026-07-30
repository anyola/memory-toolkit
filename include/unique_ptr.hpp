#ifndef UNIQUE_PTR_HPP
#define UNIQUE_PTR_HPP

#include <cstddef>
#include <utility>
#include <type_traits>

#include "default_delete.hpp"
#include "compressed_pair.hpp"

namespace mtk {
    template<typename T, typename Deleter = default_delete<T>>
    class unique_ptr{
    private:
        CompressedPair<T*, Deleter> pair;
    public:
    unique_ptr() noexcept : pair(nullptr, Deleter()) {}
    unique_ptr(std::nullptr_t) : pair(nullptr, Deleter()) {}
    explicit unique_ptr(T* p) : pair(p, Deleter()) {}
    unique_ptr(T* p, const Deleter& d) : pair(p, d) {}
    unique_ptr(T* p, Deleter&& d) : pair(p, std::move(d)) {}
    unique_ptr(const unique_ptr&) = delete;
    unique_ptr& operator=(const unique_ptr&) = delete;
    unique_ptr(unique_ptr&& other) noexcept : pair(other.pair.get_first(), std::move(other.pair.get_second())) {
        other.pair.get_first() = nullptr;
    }
    unique_ptr& operator=(unique_ptr&& other) {
        if(this != &other){
            if(pair.get_first() != nullptr){
                pair.get_second()(pair.get_first());
            }
            pair.get_first() = other.pair.get_first();
            other.pair.get_first() = nullptr;
            pair.get_second() = std::move(other.pair.get_second());
        }
        return *this;
    }
    unique_ptr& operator=(std::nullptr_t) {
        reset();
        return *this;
    }
    ~unique_ptr() {
        if(pair.get_first() != nullptr) {
            pair.get_second()(pair.get_first());
        }
    }
    [[nodiscard]] T* release() noexcept {
        T* temp = pair.get_first();
        pair.get_first() = nullptr;
        return temp;
    }
    void reset(T* new_ptr = nullptr) noexcept {
        if(new_ptr != pair.get_first()){
            if(pair.get_first() != nullptr){
                pair.get_second()(pair.get_first());
            }
            pair.get_first() = new_ptr;
        }
    }
    [[nodiscard]] T* get() const noexcept {
        return pair.get_first();
    }
    T& operator*() const {
        return *pair.get_first();
    }
    T* operator->() const noexcept {
        return pair.get_first();
    }
    explicit operator bool() const noexcept {
        if(pair.get_first() == nullptr) {
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
        std::swap(lhs.pair.get_first(), rhs.pair.get_first());
        std::swap(lhs.pair.get_second(), rhs.pair.get_second());
    }
    };

    template<typename T, typename ... Args>
    typename std::enable_if<!std::is_array<T>::value, unique_ptr<T>>::type
    [[nodiscard]] unique_ptr<T> make_unique(Args&&... args){
        return unique_ptr<T>(new T(std::forward<Args>(args)...));
    }


    template<typename T, typename Deleter>
    class unique_ptr<T[], Deleter> {
    private:
        CompressedPair<T*, Deleter> pair;
    public:
        unique_ptr() noexcept : pair(nullptr, Deleter()) {}
        unique_ptr(std::nullptr_t) : pair(nullptr, Deleter()) {}
        explicit unique_ptr(T* p) : pair(p, Deleter()) {}
        unique_ptr(T* p, const Deleter& d) : pair(p, d) {}
        unique_ptr(T* p, Deleter&& d) : pair(p, std::move(d)) {}
        unique_ptr(const unique_ptr&) = delete;
        unique_ptr& operator=(const unique_ptr&) = delete;
        unique_ptr(unique_ptr&& other) noexcept : pair(other.pair.get_first(), std::move(other.pair.get_second())) {
        other.pair.get_first() = nullptr;
        }
        unique_ptr& operator=(unique_ptr&& other) {
            if(this != &other){
                if(pair.get_first() != nullptr){
                    pair.get_second()(pair.get_first());
                }
                pair.get_first() = other.pair.get_first();
                other.pair.get_first() = nullptr;
                pair.get_second() = std::move(other.pair.get_second());
            }
            return *this;
        }
        unique_ptr& operator=(std::nullptr_t) {
            reset();
            return *this;
        }
        ~unique_ptr() {
            if(pair.get_first() != nullptr) {
                pair.get_second()(pair.get_first());
            }
        }
        [[nodiscard]] T* release() noexcept {
            T* temp = pair.get_first();
            pair.get_first() = nullptr;
            return temp;
        }
        void reset(T* new_ptr = nullptr) noexcept {
            if(new_ptr != pair.get_first()){
                if(pair.get_first() != nullptr){
                    pair.get_second()(pair.get_first());
                }
                pair.get_first() = new_ptr;
            }
        }
        [[nodiscard]] T* get() const noexcept {
            return pair.get_first();
        }
        T& operator[](std::size_t i) const {
            return pair.get_first()[i];
        }
        explicit operator bool() const noexcept {
            if(pair.get_first() == nullptr) {
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
            std::swap(lhs.pair.get_first(), rhs.pair.get_first());
            std::swap(lhs.pair.get_second(), rhs.pair.get_second());
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