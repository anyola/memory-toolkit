#ifndef CONTROL_BLOCK_HPP
#define CONTROL_BLOCK_HPP

#include <atomic>
#include <cstddef>

namespace mtk {
    class ControlBlockBase {
    public:
        std::atomic<std::size_t> strong_count{1};
        std::atomic<std::size_t> weak_count{0};
        
        ControlBlockBase() = default;
        virtual void destroy() = 0;
        virtual ~ControlBlockBase() = default;
        ControlBlockBase(const ControlBlockBase&) = delete;
        ControlBlockBase& operator=(const ControlBlockBase&) = delete;
        ControlBlockBase(ControlBlockBase&&) = delete;
        ControlBlockBase& operator=(ControlBlockBase&&) = delete;
    };

    template<typename T>
    class ControlBlock : public ControlBlockBase {
    private:
        T* ptr;
    public:
        explicit ControlBlock(T* p) : ptr(p) {}

        void destroy() override {
            delete ptr;
            ptr = nullptr;
        }
    };
}

#endif