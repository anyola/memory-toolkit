#ifndef ENABLE_SHARED_FROM_THIS_HPP
#define ENABLE_SHARED_FROM_THIS_HPP

#include "weak_ptr.hpp"

namespace mtk {
    template<typename T>
    class enable_shared_from_this {
    private:
        mutable weak_ptr<T> weak_ptr_this;
        template<typename U> friend class shared_ptr;
    protected:
        enable_shared_from_this() noexcept = default;
        enable_shared_from_this(const enable_shared_from_this&) noexcept {}
        enable_shared_from_this& operator=(const enable_shared_from_this&) noexcept {
            return *this;
        }
        ~enable_shared_from_this() = default;
    public:
        shared_ptr<T> shared_from_this() {
            return weak_ptr_this.lock();
        }


    };
}