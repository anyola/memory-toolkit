#ifndef SHARED_PTR_HPP
#define SHARED_PTR_HPP

#include <cstddef>
#include <utility>
#include <type_traits>

#include "control_block.hpp"

namespace mtk {
    template<typename T> class weak_ptr;
    template<typename T> class enable_shared_from_this;

    template<typename T>
    class shared_ptr {
    private:
        T* ptr;
        ControlBlockBase* control_block;
        shared_ptr(T* p, ControlBlockBase* cb) noexcept : ptr(p), control_block(cb) {}
        template<typename U> friend class weak_ptr;
        template<typename U, typename... UArgs>
        friend shared_ptr<U> make_shared(UArgs&&...);

        template<typename U, typename T2>
        friend shared_ptr<U> static_pointer_cast(const shared_ptr<T2>& s_ptr) noexcept;
        template<typename U, typename T2>
        friend shared_ptr<U> dynamic_pointer_cast(const shared_ptr<T2>& s_ptr) noexcept;
        template<typename U, typename T2>
        friend shared_ptr<U> const_pointer_cast(const shared_ptr<T2>& s_ptr) noexcept;
        
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

    template<typename T, typename... Args>
    [[nodiscard]] shared_ptr<T> make_shared(Args&&... args) {
        InlineControlBlock<T>* icb = new InlineControlBlock<T>(std::forward<Args>(args)...);
        return shared_ptr<T>(icb->get(), icb);
    }

    template<typename U, typename T>
    [[nodiscard]] shared_ptr<U> static_pointer_cast(const shared_ptr<T>& s_ptr) noexcept {
        T* raw_ptr = s_ptr.get();
        U* casted = static_cast<U*>(raw_ptr);
        s_ptr.control_block->strong_count.fetch_add(1);
        return shared_ptr<U>(casted, s_ptr.control_block);
    }

    template<typename U, typename T>
    [[nodiscard]] shared_ptr<U> dynamic_pointer_cast(const shared_ptr<T>& s_ptr) noexcept {
        T* raw_ptr = s_ptr.get();
        U* casted = dynamic_cast<U*>(raw_ptr);
        if(casted != nullptr) {
            s_ptr.control_block->strong_count.fetch_add(1);
            return shared_ptr<U>(casted, s_ptr.control_block);
        }
        else{
            return shared_ptr<U>(casted);
        }
    }

    template<typename U, typename T>
    [[nodiscard]] shared_ptr<U> const_pointer_cast(const shared_ptr<T>& s_ptr) noexcept {
        T* raw_ptr = s_ptr.get();
        U* casted = const_cast<U*>(raw_ptr);
        s_ptr.control_block->strong_count.fetch_add(1);
        return shared_ptr<U>(casted, s_ptr.control_block);
    }

    template<typename T>
    void init_weak_this(T* p, ControlBlockBase* cb) {
        if constexpr (std::is_base_of_v<enable_shared_from_this<T>, T>){
            p->weak_ptr_this = shared_ptr<T>(p, cb);
        }
    }
}

#endif 