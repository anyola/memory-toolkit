#ifndef SHARED_PTR_HPP
#define SHARED_PTR_HPP

#include <cstddef>
#include <utility>

#include "control_block.hpp"

namespace mtk {
    template<typename T> class weak_ptr;
    template<typename T>
    class shared_ptr {
    private:
        T* ptr;
        ControlBlockBase* control_block;
        shared_ptr(T* p, ControlBlockBase* cb) noexcept : ptr(p), control_block(cb) {}
        template<typename U> friend class weak_ptr;
    public:
        shared_ptr() noexcept : ptr(nullptr), control_block(nullptr) {}
        shared_ptr(std::nullptr_t) : ptr(nullptr), control_block(nullptr) {}
        explicit shared_ptr(T* p) {
            if(p != nullptr) {
                ptr = p;
                control_block = new ControlBlock<T>(p);
            }
            else {
                ptr = nullptr;
                control_block = nullptr;
            }
        }
        shared_ptr(const shared_ptr& other) {
            ptr = other.ptr;
            control_block = other.control_block;
            if(control_block != nullptr) {
                control_block->strong_count.fetch_add(1);
            }
        }
        shared_ptr(shared_ptr&& other) noexcept {
            ptr = other.ptr;
            control_block = other.control_block;
            other.ptr = nullptr;
            other.control_block = nullptr;
        }
        shared_ptr& operator=(shared_ptr other) {
            swap(*this, other);
            return *this;
        }
        ~shared_ptr() {
            if(control_block != nullptr) {
                std::size_t prev = control_block->strong_count.fetch_sub(1);
                if(prev == 1) {
                    control_block->destroy();
                    if(control_block->strong_count == 0 && control_block->weak_count == 0) {
                        delete control_block;
                    }
                }
            }
        }
        [[nodiscard]] std::size_t use_count() const noexcept {
            if(control_block != nullptr) {
                return control_block->strong_count;
            }
            return 0;
        }
        void reset() noexcept {
            *this = shared_ptr();
        }
        void reset(T* new_ptr) {
            *this = shared_ptr(new_ptr);
        }
        [[nodiscard]] T* get() const noexcept {
            return ptr;
        }
        T& operator*() const noexcept {
            return *ptr;
        }
        T* operator->() const noexcept {
            return ptr;
        }
        explicit operator bool() const noexcept {
            return ptr != nullptr;
        }
        friend bool operator==(const shared_ptr& lhs, const shared_ptr& rhs) {
            if(lhs.get() == rhs.get()){
                return true;
            }
            return false;
        }
        friend bool operator!=(const shared_ptr& lhs, const shared_ptr& rhs) {
            return !(lhs.get() == rhs.get());
        }
        friend bool operator==(const shared_ptr& p, std::nullptr_t) {
            return p.get() == nullptr;
        }
        friend bool operator==(std::nullptr_t, const shared_ptr& p) {
            return p.get() == nullptr;
        }
        friend bool operator!=(const shared_ptr& p, std::nullptr_t) {
            return !(p.get() == nullptr);
        }
        friend bool operator!=(std::nullptr_t, const shared_ptr& p) {
            return !(p.get() == nullptr);
        }
        friend void swap(shared_ptr& lhs, shared_ptr& rhs) noexcept {
            std::swap(lhs.ptr, rhs.ptr);
            std::swap(lhs.control_block, rhs.control_block);
        }
    };
}

#endif