#ifndef DEFAULT_DELETE_HPP
#define DEFAULT_DELETE_HPP

#include <cstddef>

namespace mtk {
    template<typename T>
    class default_delete{
    public:
    default_delete() noexcept = default;
    void operator()(T* ptr) const {
        static_assert(sizeof(T) > 0, "can't delete an incomplete type");
        delete ptr;
    }
    };

    template<typename T>
    class default_delete<T[]>{
        public:
        default_delete() noexcept = default;
        void operator()(T* ptr) const {
            static_assert(sizeof(T) > 0, "can't delete an incomplete type");
            delete[] ptr;
        }
    };

}

#endif