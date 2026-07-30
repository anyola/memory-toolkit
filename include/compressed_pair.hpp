#ifndef COMPRESSED_PAIR_HPP
#define COMPRESSED_PAIR_HPP

#include <type_traits>
#include <utility>

namespace mtk {
    template<typename T1, typename T2, bool = std::is_empty_v<T2> && !std::is_final_v<T2>>
    class CompressedPair;

    template<typename T1, typename T2>
    class CompressedPair<T1, T2, true> : private T2 {
    private:
        T1 first;
    public:
        CompressedPair() = default;
        CompressedPair(T1 f, T2 s) : T2(std::move(s)), first(std::move(f)) {}

        T1& get_first() noexcept {
            return first;
        }
        T2& get_second() noexcept {
            return *this;
        }
        const T1& get_first() const noexcept {
            return first;
        }
        const T2& get_second() const noexcept {
            return *this;
        }
    };

    template<typename T1, typename T2>
    class CompressedPair<T1, T2, false> {
    private:
        T1 first;
        T2 second;
    public:
        CompressedPair() = default;
        CompressedPair(T1 f, T2 s) : first(std::move(f)), second(std::move(s)) {}
        
        T1& get_first() noexcept {
            return first;
        }
        T2& get_second() noexcept {
            return second;
        }
        const T1& get_first() const noexcept {
            return first;
        }
        const T2& get_second() const noexcept {
            return second;
        }

    };

}

#endif