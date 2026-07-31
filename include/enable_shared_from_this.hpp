#ifndef ENABLE_SHARED_FROM_THIS_HPP
#define ENABLE_SHARED_FROM_THIS_HPP

#include <exception>
#include "weak_ptr.hpp"

namespace mtk {

    class bad_weak_ptr : public std::exception {
    public:
        const char* what() const noexcept override {
            return "mtk::bad_weak_ptr: object is not owned by a shared_ptr";
        }
    };

    template<typename T>
    class enable_shared_from_this {
    private:
        mutable weak_ptr<T> weak_ptr_this;
        template<typename U> friend class shared_ptr;
        template<typename U> friend void init_weak_this(U* p, ControlBlockBase* cb);
    protected:
        enable_shared_from_this() noexcept = default;
        enable_shared_from_this(const enable_shared_from_this&) noexcept {}
        enable_shared_from_this& operator=(const enable_shared_from_this&) noexcept {
            return *this;
        }
        ~enable_shared_from_this() = default;
    public:
        shared_ptr<T> shared_from_this() {
            shared_ptr<T> sp = weak_ptr_this.lock();
            if(sp != nullptr) {
                return sp;
            }
            else {
                throw bad_weak_ptr();
            }
        }
    };
}

#endif