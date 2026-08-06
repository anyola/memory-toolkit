# DESIGN.md - memory-toolkit

This document explains the architectural decisions behind the library: why each component is built the way it is, which trade-offs were made deliberately.

## 1. Architecture overview

The library consists of five main components:

| Component | File | Ownership model |
|---|---|---|
| `unique_ptr<T, Deleter>` | `unique_ptr.hpp` | exclusive ownership (move-only) |
| `shared_ptr<T>` / `weak_ptr<T>` | `shared_ptr.hpp`, `weak_ptr.hpp` | shared ownership, external control block |
| `intrusive_ptr<T>` | `intrusive_ptr.hpp` | shared ownership, counter embedded in the object |
| `enable_shared_from_this<T>` | `enable_shared_from_this.hpp` | obtaining a `shared_ptr` from inside the object |
| `CompressedPair<T1, T2>` | `compressed_pair.hpp` | utility primitive for EBO |

`shared_ptr` and `intrusive_ptr` implement the same idea - reference counting with automatic destruction - in two fundamentally different ways. Section 6 explains why a single library needs both.

## 2. Control block: one hierarchy instead of two unrelated implementations

`shared_ptr` does not store the managed object directly - it stores a pointer to a **control block**, a separate structure holding the reference counts. This is what lets several independent `shared_ptr` instances agree on when the object should be destroyed.

The control block was originally written for a single scenario: `shared_ptr(new T)`, where the object and the control block are two separate allocations. Once `make_shared` with a single-allocation optimization was needed (section 3), a **second** kind of control block became necessary - one where the object lives in the same memory block as the counters and is destroyed not via `delete`, but via an explicit destructor call.

For `shared_ptr`/`weak_ptr` to treat both kinds identically, without knowing which one they're actually holding, the control block is a small polymorphic hierarchy:

```
ControlBlockBase          - strong_count, weak_count, virtual destroy()
├── ControlBlock<T>        - stores T*, destroy() calls delete
└── InlineControlBlock<T>  - stores T inline (raw storage + placement new),
                              destroy() only calls the object's destructor
```

`shared_ptr`/`weak_ptr` store a `ControlBlockBase*` and call `destroy()` virtually - they don't need to know which concrete control block they're pointing at.

## 3. `make_shared`: why one allocation instead of two

`shared_ptr<T>(new T(args...))` requires two separate memory allocations: one for the object `T` itself (inside `new T(...)`), and one for the control block (inside the `shared_ptr` constructor). Two `malloc`/`free` calls are a measurable overhead when objects are created frequently, and the object and its control block also end up in potentially distant memory locations (worse cache locality).

`make_shared<T>(args...)` instead performs a **single** allocation - an `InlineControlBlock<T>`, which holds `alignas(T) unsigned char storage[sizeof(T)]` alongside the atomic counters. The object `T` is constructed in that storage via placement new:

```cpp
::new (static_cast<void*>(storage_)) T(std::forward<Args>(args)...);
```

This is measured, not just claimed: the `make_shared_allocation_check` test overrides the global `operator new` and counts the actual calls:

```
shared_ptr(new int(...)) allocations: 2
make_shared<int>(...) allocations:    1
```

The trade-off is that the `InlineControlBlock` stays alive as long as **at least one** weak reference exists, even after the object `T` itself has already been destroyed (see the `weak_count` discussion in section 5). This means the memory backing the object `T` isn't formally freed until the last `weak_ptr` is gone, not just the last `shared_ptr` - the same price `std::make_shared` pays.

## 4. Thread safety: what is protected by atomics, and what isn't

`strong_count` and `weak_count` in `ControlBlockBase` are `std::atomic<std::size_t>`. That is enough to make the **reference counts** thread-safe: multiple threads can concurrently copy and destroy `shared_ptr`/`intrusive_ptr` instances pointing to the same object without racing on the counter itself. This is verified with a stress test of 8 threads × 20,000 iterations under ThreadSanitizer, with zero warnings.

It's important to be precise about the boundary of this guarantee: **an atomic counter does not make the managed object itself thread-safe**. If two threads simultaneously write to `*shared_ptr<T>`, that's an ordinary data race, and `shared_ptr` does nothing to prevent it - exactly like `std::shared_ptr`. The library only guarantees correct reference counting and single destruction of the object, not thread-safe access to its data.

### `weak_ptr::lock()` - why it needs a CAS loop instead of a plain `fetch_add`

Copying a `shared_ptr` increments `strong_count` unconditionally - if a copy exists, the object is already guaranteed to be alive. `weak_ptr::lock()` is different: at the moment of the call the object **might already be dead**, and `strong_count` may only be incremented if it isn't zero yet - otherwise a destroyed object could be "resurrected". Between reading the current counter value and attempting to increment it, another thread could destroy the last `shared_ptr`. That's why `lock()` uses a compare-and-swap loop:

```cpp
std::size_t current = control_block->strong_count.load();
while (current != 0) {
    if (control_block->strong_count.compare_exchange_weak(current, current + 1)) {
        return shared_ptr<T>(ptr, control_block);
    }
    // on failure, compare_exchange_weak refreshes current with the actual value
}
return shared_ptr<T>(); // current reached 0 - the object is already gone
```

This is the only place in the library where a plain atomic increment isn't enough - the decision of whether the counter may be incremented depends on its current value at the moment of the attempt.

## 5. Why `weak_count` starts at 1, not 0

**Symptom.** A test for `enable_shared_from_this` was failing under AddressSanitizer with a heap-use-after-free, even though the counter logic looked correct at first glance and all other tests (`shared_ptr` and `weak_ptr` on their own) were passing cleanly.

**Root cause.** `enable_shared_from_this<T>` stores a `weak_ptr<T> weak_ptr_this` inside the object itself - the object holds a weak reference **to itself** for as long as it's alive. With the original implementation (`weak_count` starting at 0), the destruction sequence looked like this:

1. The last `shared_ptr<T>` dies → `strong_count` reaches 0 → `control_block->destroy()` is called
2. `destroy()` calls `delete ptr`, which invokes `~T()`
3. `~T()` destroys the `enable_shared_from_this<T>` base class, and therefore its `weak_ptr_this` member
4. `weak_ptr_this`'s destructor decrements `weak_count`. Since `strong_count` is already 0, and `weak_ptr_this` was the only weak reference, `weak_count` reaches 0 right here - and the `weak_ptr` destructor deletes the control block (`delete control_block`)
5. But we're still **inside** `control_block->destroy()` - a method of the very object that was just deleted. Returning from `delete ptr` then tries to write `ptr_ = nullptr` into memory that has already been freed.

The control block was being deleted from the middle of its own method call.

**Fix.** `weak_count` is initialized to **1**, not 0. That extra unit represents a "virtual" weak reference implicitly held by the entire group of `shared_ptr` instances, for as long as at least one of them is alive. Real `weak_ptr` instances (including `weak_ptr_this`) still honestly increment and decrement the counter on top of that baseline - their code doesn't change at all.

The key change is **when** that virtual reference gets released. Not inside `destroy()`, but as a separate step, only after `destroy()` has fully returned control to the `shared_ptr` destructor:

```cpp
std::size_t prev = control_block->strong_count.fetch_sub(1);
if (prev == 1) {
    control_block->destroy();                           // may safely decrement weak_count
                                                        // from within (e.g. via weak_ptr_this)
                                                        // without letting it hit zero early
    if (control_block->weak_count.fetch_sub(1) == 1) {  // release the virtual reference
        delete control_block;                           // now safe - outside destroy()
    }
}
```

This is the same technique used internally by `libstdc++`/`libc++` shared_ptr implementations: initializing a counter with an "extra" unit is a common way to decouple the order of operations in cases where the final decrement would otherwise happen at an unsafe point in the call stack.

## 6. `shared_ptr` vs `intrusive_ptr`: which one to use

| | `shared_ptr` | `intrusive_ptr` |
|---|---|---|
| Where the counter lives | separately, in a control block | inside the object itself |
| Allocations on regular creation | 2 (or 1 via `make_shared`) | always 1 (object is created once) |
| Can obtain a pointer from a raw `T*` alone | no | yes - the counter is already in the object |
| `weak_ptr` equivalent | yes | not in this implementation (would require a separate weak-reference counter inside the object) |
| Plugging in a counting strategy | fixed | via ADL (`intrusive_ptr_add_ref`/`intrusive_ptr_release`) - a custom one can be plugged in without inheritance |

`intrusive_ptr` is a typical choice in game engines and systems with a high frequency of small object creation/destruction (particles, entities), where a single allocation and a minimal pointer size matter. `shared_ptr` is the more general-purpose default, especially when a `weak_ptr` is needed or when the type can't be modified up front to carry an intrusive counter.

## 7. EBO via `CompressedPair`

`unique_ptr<T, Deleter>` stores both a pointer and a Deleter instance. If the Deleter is a stateless class (like `default_delete<T>`), naively storing it as `T* ptr; Deleter deleter;` still spends at least 1 byte on `deleter` (empty classes in C++ can't have zero size), plus possible padding.

`CompressedPair<T1, T2>` applies the Empty Base Optimization: if `T2` is empty and not `final`, `CompressedPair` **inherits** from `T2` instead of storing it as a field - empty base classes don't increase the size of the derived object. The choice between inheritance and a regular field is made at compile time via a partial specialization on a `bool` parameter, computed from `std::is_empty_v<T2> && !std::is_final_v<T2>`.

Verified with `static_assert` against the actual size:

```cpp
static_assert(sizeof(CompressedPair<int*, EmptyDeleter>) == sizeof(int*));       // EBO
static_assert(sizeof(CompressedPair<int*, FinalEmptyDeleter>) > sizeof(int*));   // final - EBO impossible
static_assert(sizeof(CompressedPair<int*, StatefulDeleter>) > sizeof(int*));     // regular field
```

In practice: `sizeof(unique_ptr<int, EmptyDeleter>) == 8` - exactly the size of a single pointer on a 64-bit platform, despite the presence of a Deleter.

## 8. Exception safety

The library follows RAII throughout its implementation, so most operations are naturally exception-safe without requiring explicit recovery logic.

Several guarantees come directly from the ownership model and the C++ object lifetime rules.

### Object construction

Both `make_unique()` and `make_shared()` rely on ordinary `new` and placement new.

If constructing `T` throws an exception, the language guarantees that the allocated memory is released automatically before the exception propagates.

As a result, partially constructed objects cannot leak memory.

### Ownership transfer

One constructor requires explicit exception handling:

```cpp
shared_ptr<T>(T* p)
```

Ownership of the raw pointer is transferred to the `shared_ptr` before the control block exists.

If allocating the control block throws `std::bad_alloc`, the managed object would otherwise leak because no control block has been created yet to destroy it.

The implementation therefore catches the allocation failure, deletes the managed object, and rethrows the original exception.

This preserves ownership semantics even when control block allocation fails.

### `shared_from_this()`

Calling `shared_from_this()` on an object that is not currently owned by any `shared_ptr` is considered a programming error.

Rather than silently returning an empty `shared_ptr`, the implementation throws `mtk::bad_weak_ptr`, mirroring the behavior of `std::bad_weak_ptr`.

Reporting the error immediately makes incorrect ownership usage easier to detect and prevents invalid ownership state from propagating through the program.

### Strong exception guarantee

Operations such as `reset()` are implemented using temporary objects.

If constructing the temporary object throws, the original smart pointer remains unchanged.

This naturally provides the strong exception guarantee without requiring explicit rollback logic.

## 9. Deliberate limitations

- **No allocator-aware variants.** `make_shared` calls the global `operator new` directly; an `allocate_shared` taking a custom allocator is not implemented.
- **`intrusive_ptr` has no weak references.** Implementing a weak-reference counter for the intrusive model would require either a second counter in `RefCounter` or a separate control block layered on top of the object - which would defeat the main advantage of the intrusive approach (a single allocation).
- **`shared_ptr`/`weak_ptr` don't support arrays** (`shared_ptr<T[]>`), unlike `unique_ptr`. This is a deliberate scope decision, not a technical limitation.
- **No `atomic_load`/`atomic_store` for `shared_ptr`** (the equivalent of the free functions from `<memory>` for thread-safe access to a `shared_ptr` object itself, as opposed to just the counter inside it) - currently, only copying/destroying independent `shared_ptr` instances is thread-safe, not concurrent writes to the same `shared_ptr` object from different threads.