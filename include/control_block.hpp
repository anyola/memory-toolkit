#ifndef CONTROL_BLOCK_HPP
#define CONTROL_BLOCK_HPP

#include <atomic>
#include <cstddef>
#include <utility>

namespace mtk {
    class ControlBlockBase {
    public:
        std::atomic<std::size_t> strong_count{1};
        std::atomic<std::size_t> weak_count{1};

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

    template<typename T>
    class InlineControlBlock : public ControlBlockBase {
    private:
        alignas(T) unsigned char storage[sizeof(T)];
    public:
        template<typename... Args>
        explicit InlineControlBlock(Args&&... args) {
            ::new (static_cast<void*>(storage)) T(std::forward<Args>(args)...);
        }
        T* get() noexcept {
            return reinterpret_cast<T*>(storage);
        }
        void destroy() override {
            get()->~T();
        }
        
    };

}

#endif