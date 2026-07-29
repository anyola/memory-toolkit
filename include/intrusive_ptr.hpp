#ifndef INTRUSIVE_PTR_HPP
#define INTRUSIVE_PTR_HPP

#include <atomic>
#include <utility>  

namespace mtk {
    class RefCounter {
    private:
        mutable std::atomic<std::size_t> ref_count {0};

    protected:
        RefCounter() noexcept = default;
        RefCounter(const RefCounter&) noexcept {}
        RefCounter& operator=(const RefCounter&) noexcept {
            return *this;
        }
        RefCounter(RefCounter&& other) noexcept {}
         RefCounter& operator=(RefCounter&& other) noexcept {
            return *this;
        }
        virtual ~RefCounter() = default;
    public:
        template<typename T> friend void intrusive_ptr_add_ref(T*);
        template<typename T>friend void intrusive_ptr_release(T*);
    };
    
    template<typename T>
    void intrusive_ptr_add_ref(T* p) {
        p->ref_count.fetch_add(1);
    }
    template<typename T>
    void intrusive_ptr_release(T* p) {
        if(p->ref_count.fetch_sub(1) == 1) {
            delete p;
        }
    }

    template<typename T>
    class intrusive_ptr{
    private:
        T* ptr;
    public:
        intrusive_ptr() noexcept : ptr(nullptr) {}
        intrusive_ptr(std::nullptr_t) noexcept : ptr(nullptr) {}
        explicit intrusive_ptr(T* p) {
            ptr = p;
            if(p != nullptr) {
                intrusive_ptr_add_ref(p);
            }
        }
        intrusive_ptr(const intrusive_ptr& other) {
            ptr = other.ptr;
            if(other.ptr != nullptr) {
                intrusive_ptr_add_ref(other.ptr);
            }
        }
        intrusive_ptr(intrusive_ptr&& other) noexcept {
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        intrusive_ptr& operator=(intrusive_ptr other) {
            swap(*this, other);
            return *this; 
        }
        ~intrusive_ptr() {
            if(ptr != nullptr){
                intrusive_ptr_release(ptr);
            }
        }
        
        [[nodiscard]] T* get() const noexcept {
            return ptr;
        }
        explicit operator bool() const noexcept {
            return ptr != nullptr;
        }
        T& operator*() const noexcept {
            return *ptr;
        }
        T* operator->() const noexcept {
            return ptr;
        }

        friend bool operator==(const intrusive_ptr& lhs, const intrusive_ptr& rhs) {
            if(lhs.get() == rhs.get()){
                return true;
            }
            return false;
        }
        friend bool operator!=(const intrusive_ptr& lhs, const intrusive_ptr& rhs) {
            return !(lhs.get() == rhs.get());
        }
        friend bool operator==(const intrusive_ptr& p, std::nullptr_t) {
            return p.get() == nullptr;
        }
        friend bool operator==(std::nullptr_t, const intrusive_ptr& p) {
            return p.get() == nullptr;
        }
        friend bool operator!=(const intrusive_ptr& p, std::nullptr_t) {
            return !(p.get() == nullptr);
        }
        friend bool operator!=(std::nullptr_t, const intrusive_ptr& p) {
            return !(p.get() == nullptr);
        }
        friend void swap(intrusive_ptr& lhs, intrusive_ptr& rhs) noexcept {
            std::swap(lhs.ptr, rhs.ptr);
        }

    };
}

#endif