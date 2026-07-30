// Proves that make_shared<T> performs exactly one allocation,
// while shared_ptr<T>(new T(...)) performs two. Separated from
// the main test suite because overriding global operator new
// conflicts with GoogleTest's internals under AddressSanitizer.
#include <cstdlib>
#include <iostream>
#include "shared_ptr.hpp"
 
namespace {
int g_new_calls = 0;
}
 
void* operator new(std::size_t size) {
    ++g_new_calls;
    void* p = std::malloc(size);
    if (!p) throw std::bad_alloc();
    return p;
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
 
int main() {
    g_new_calls = 0;
    { mtk::shared_ptr<int> p(new int(1)); }
    int separate = g_new_calls;
 
    g_new_calls = 0;
    { auto p = mtk::make_shared<int>(1); }
    int combined = g_new_calls;
 
    std::cout << "shared_ptr(new int(...)) allocations: " << separate << "\n";
    std::cout << "make_shared<int>(...) allocations:    " << combined << "\n";
 
    if (separate != 2) {
        std::cerr << "FAIL: expected 2 allocations for shared_ptr(new T), got " << separate << "\n";
        return 1;
    }
    if (combined != 1) {
        std::cerr << "FAIL: expected 1 allocation for make_shared<T>, got " << combined << "\n";
        return 1;
    }
    std::cout << "PASS: make_shared performs a single allocation as expected\n";
    return 0;
}