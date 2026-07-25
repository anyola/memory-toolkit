#ifndef WEAK_PTR_HPP
#define WEAK_PTR_HPP

#include <cstddef>
#include <utility>

#include "control_block.hpp"
#include "shared_ptr.hpp"

namespace mtk {
    template<typename T>
    class weak_ptr {
    private:
        T* ptr;
        ControlBlock<T>* control_block;
    public:
        weak_ptr() noexcept : ptr(nullptr), control_block(nullptr) {}
        weak_ptr(const shared_ptr<T>& sp) {
            ptr = sp.ptr;
            control_block = sp.control_block;
            if(control_block != nullptr) {
                control_block->weak_count.fetch_add(1);
            }
        }
        weak_ptr(const weak_ptr& other) {
            ptr = other.ptr;
            control_block = other.control_block;
            if(control_block != nullptr) {
                control_block->weak_count.fetch_add(1);
            }
        }
        weak_ptr(weak_ptr&& other) noexcept {
            ptr = other.ptr;
            control_block = other.control_block;
            other.ptr = nullptr;
            other.control_block = nullptr;
        }
        ~weak_ptr() {
            if(control_block != nullptr) {
                std::size_t prev = control_block->weak_count.fetch_sub(1);
                if(prev == 1 && control_block->strong_count.load() == 0) {
                    delete control_block;
                }
            }
        }
        [[nodiscard]] bool expired() const noexcept {
            if(control_block != nullptr) {
                if(control_block->strong_count == 0) {
                    return true;
                }
                else {
                    return false;
                }
            }
            return true;
        }
        [[nodiscard]] std::size_t use_count() const noexcept {
            if(control_block != nullptr) {
                return control_block->strong_count.load();
            }
            return 0;
        }
        [[nodiscard]] shared_ptr<T> lock() const noexcept {
            if(control_block == nullptr) {
                return shared_ptr<T>();
            }
            std::size_t current = control_block->strong_count.load();
            while (current != 0) {
                if (control_block->strong_count.compare_exchange_weak(current, current + 1)) {
                    return shared_ptr<T>(ptr, control_block);
                }
            }
            return shared_ptr<T>();
        }
    };
}

#endif