#ifndef CONTROL_BLOCK_HPP
#define CONTROL_BLOCK_HPP

#include <atomic>
#include <cstddef>

namespace mtk {
    template<typename T>
    class ControlBlock {
    private:
        T* ptr;
    public:
        explicit ControlBlock(T* p) : ptr(p) {}

        void destroy() {
            delete ptr;
            ptr = nullptr;
        }
        ControlBlock(const ControlBlock&) = delete;
        ControlBlock& operator=(const ControlBlock&) = delete;
        ControlBlock(ControlBlock&&) = delete;
        ControlBlock& operator=(ControlBlock&&) = delete;

        std::atomic<std::size_t> strong_count = 1;
        std::atomic<std::size_t> weak_count = 0;
    };
}

#endif