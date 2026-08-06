# memory-toolkit

Memory management library exploring ownership models,
reference counting strategies, and lifetime management techniques behind
modern C++ smart pointers.

The project implements several memory ownership abstractions inspired by
`std::memory`, focusing not only on reproducing the public API, but also on
understanding the engineering decisions behind it:

- exclusive ownership and move semantics;
- shared ownership and reference counting;
- weak observation without extending lifetime;
- intrusive reference counting;
- exception-safe resource management;
- memory optimization techniques.

The goal of this project is not to replace the standard library, but to
recreate a minimal memory toolkit and study the design trade-offs behind
modern C++ ownership models.

Detailed architectural decisions and implementation notes are available in
[DESIGN.md](DESIGN.md).

## Quick example

```cpp
#include "shared_ptr.hpp"
#include "unique_ptr.hpp"

auto owner = mtk::make_unique<Widget>(args...);

mtk::shared_ptr<Widget> a = mtk::make_shared<Widget>(args...);
mtk::shared_ptr<Widget> b = a;          // shared ownership, refcount == 2
mtk::weak_ptr<Widget> w = a;            // observes without extending lifetime

if (auto locked = w.lock()) {
    // safe access while the object is still alive
}
```

## What's implemented

| | Supported |
|---|---|
| `unique_ptr<T, Deleter>` | default/custom deleter, move-only, `unique_ptr<T[]>`, `make_unique` |
| `shared_ptr<T>` / `weak_ptr<T>` | copy/move, `use_count`, `reset`, comparisons, `swap` |
| `make_shared<T>` | single-allocation optimization (measured, not just claimed — see below) |
| Pointer casts | `static_pointer_cast`, `dynamic_pointer_cast`, `const_pointer_cast` |
| `enable_shared_from_this<T>` | safe `shared_from_this()` from inside an object |
| `intrusive_ptr<T>` + `RefCounter` | ADL-based reference counting, single allocation |
| `CompressedPair<T1, T2>` | Empty Base Optimization for stateless deleters |
| Thread safety | atomic reference counts, verified under ThreadSanitizer |

## Measured results

`make_shared` allocation count, verified by a standalone test that overrides the global `operator new`:

```text
shared_ptr(new int(...)) allocations: 2
make_shared<int>(...) allocations:    1
```

Empty Base Optimization, verified with `static_assert` against the actual object size:

```cpp
sizeof(unique_ptr<int, EmptyDeleter>) == sizeof(int*)   // == 8 on a 64-bit platform
```
## Safety guarantees

The implementation provides:

- exception-safe ownership transfer in `shared_ptr(T*)` (no leaks if control block allocation fails);
- correct `shared_from_this()` semantics via `bad_weak_ptr`;
- thread-safe reference counting using atomic counters;
- memory safety verified under AddressSanitizer and UndefinedBehaviorSanitizer.

## Build & test

```bash
mkdir build
cd build

cmake ..
cmake --build .
ctest --output-on-failure
```

With sanitizers:

```bash
cmake .. -DMEM_ENABLE_ASAN=ON   # AddressSanitizer + UndefinedBehaviorSanitizer
cmake .. -DMEM_ENABLE_TSAN=ON   # ThreadSanitizer
```

All unit tests (GoogleTest) plus a standalone allocation-count check pass cleanly under both sanitizer configurations.

## Project layout

```text
memory-toolkit/
├── CMakeLists.txt
├── include/
│   ├── unique_ptr.hpp
│   ├── shared_ptr.hpp
│   ├── weak_ptr.hpp
│   ├── intrusive_ptr.hpp
│   ├── enable_shared_from_this.hpp
│   ├── control_block.hpp
│   ├── compressed_pair.hpp
│   └── default_delete.hpp
├── test/
│   ├── all_tests.cpp
│   └── make_shared_allocation_check.cpp
└── DESIGN.md
```