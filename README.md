# memory-toolkit

[![CI](https://github.com/anyola/memory-toolkit/actions/workflows/ci.yml/badge.svg)](https://github.com/anyola/memory-toolkit/actions/workflows/ci.yml)

Библиотека управления памятью, исследующая модели владения, стратегии подсчёта ссылок и техники управления временем жизни объектов, лежащие в основе умных указателей C++.

- эксклюзивное владение и move-семантика;
- разделяемое владение и подсчёт ссылок;
- слабое наблюдение без продления времени жизни объекта;
- intrusive-подсчёт ссылок;
- exception-safe управление ресурсами;
- техники оптимизации памяти.

## Быстрый пример

```cpp
#include "shared_ptr.hpp"
#include "unique_ptr.hpp"

auto owner = mtk::make_unique<Widget>(args...);

mtk::shared_ptr<Widget> a = mtk::make_shared<Widget>(args...);
mtk::shared_ptr<Widget> b = a;          // разделяемое владение, refcount == 2
mtk::weak_ptr<Widget> w = a;            // наблюдение без продления времени жизни

if (auto locked = w.lock()) {
    // безопасный доступ, пока объект ещё жив
}
```

## Что реализовано

| | Поддерживается |
|---|---|
| `unique_ptr<T, Deleter>` | делитер по умолчанию/кастомный, move-only, `unique_ptr<T[]>`, `make_unique` |
| `shared_ptr<T>` / `weak_ptr<T>` | copy/move, `use_count`, `reset`, сравнения, `swap` |
| `make_shared<T>` | оптимизация в одну аллокацию (измерено, а не просто заявлено — см. ниже) |
| Приведения указателей | `static_pointer_cast`, `dynamic_pointer_cast`, `const_pointer_cast` |
| `enable_shared_from_this<T>` | безопасный `shared_from_this()` изнутри объекта |
| `intrusive_ptr<T>` + `RefCounter` | подсчёт ссылок, всегда одна аллокация |
| `CompressedPair<T1, T2>` | Empty Base Optimization для делитеров без состояния |
| Потокобезопасность | атомарные счётчики ссылок, проверено под ThreadSanitizer |

## Архитектурные решения

### Control block:

`shared_ptr` не хранит объект напрямую — он хранит указатель на **control block**, отдельную структуру со счётчиками ссылок. Это позволяет нескольким независимым `shared_ptr` согласованно определять момент удаления объекта.

Обычный `shared_ptr(new T)` требует двух аллокаций: под объект и под control block отдельно. Чтобы `make_shared` мог делать одну аллокацию, понадобился второй вид control block — где объект лежит в той же памяти, что и счётчики. Чтобы `shared_ptr`/`weak_ptr` работали с обоими видами одинаково, control block устроен как полиморфная иерархия:

```
ControlBlockBase          — strong_count, weak_count, virtual destroy()
├── ControlBlock<T>        — хранит T*, destroy() вызывает delete
└── InlineControlBlock<T>  — хранит T внутри себя (raw storage + placement new),
                              destroy() вызывает только деструктор объекта
```

### `make_shared`: одна аллокация вместо двух

`InlineControlBlock<T>` хранит `alignas(T) unsigned char storage[sizeof(T)]` рядом со счётчиками, а объект строится в этом хранилище через placement new — вместо раздельных `new T(...)` и `new ControlBlock<T>(...)`.

```text
shared_ptr(new int(...)) allocations: 2
make_shared<int>(...) allocations:    1
```

### Потокобезопасность:

`strong_count`/`weak_count` — атомарные, поэтому копирование и уничтожение `shared_ptr`/`intrusive_ptr` из разных потоков безопасно. Но атомарный счётчик **не** делает потокобезопасным сам управляемый объект — одновременная запись в `*shared_ptr<T>` из двух потоков остаётся обычной гонкой данных, как и в `std::shared_ptr`.

`weak_ptr::lock()`: объект может уже быть мёртв в момент вызова, и увеличивать `strong_count` можно только если он ещё не 0. Между чтением счётчика и попыткой его увеличить другой поток может успеть удалить последний `shared_ptr`, поэтому используется `compare_exchange_weak` в цикле.

### `weak_count` начинается с 1, а не с 0

Тест на `enable_shared_from_this` падал под AddressSanitizer с heap-use-after-free. Причина: `enable_shared_from_this<T>` хранит внутри объекта `weak_ptr<T> weak_ptr_this` — слабую ссылку на самого себя. При `weak_count`, стартующем с 0, последовательность разрушения была такой:

1. Последний `shared_ptr<T>` умирает → `strong_count` → 0 → вызывается `control_block->destroy()`
2. `destroy()` вызывает `delete ptr` → `~T()` → разрушается база `enable_shared_from_this<T>` → разрушается `weak_ptr_this`
3. Деструктор `weak_ptr_this` уменьшает `weak_count`. Поскольку `strong_count` уже 0, а `weak_ptr_this` была единственной weak-ссылкой, `weak_count` доходит до 0 прямо тут — и `weak_ptr` удаляет control block
4. Но мы всё ещё **внутри** `control_block->destroy()` — метода объекта, который только что удалили. Возврат из `delete ptr` пишет в уже освобождённую память.

**Решение**, то же, что используют `libstdc++`/`libc++`: `weak_count` инициализируется значением **1** — эта единица представляет "виртуальную" weak-ссылку, которую держит вся группа `shared_ptr`. Она освобождается не внутри `destroy()`, а отдельным шагом уже после его завершения:

```cpp
std::size_t prev = control_block->strong_count.fetch_sub(1);
if (prev == 1) {
    control_block->destroy();                          // может безопасно уменьшить weak_count
                                                        // изнутри, не давая ему дойти до нуля раньше
    if (control_block->weak_count.fetch_sub(1) == 1) {  // освобождаем виртуальную ссылку
        delete control_block;                           // теперь безопасно — снаружи destroy()
    }
}
```

### `shared_ptr` vs `intrusive_ptr`: когда что выбирать

| | `shared_ptr` | `intrusive_ptr` |
|---|---|---|
| Cчётчик | отдельно, в control block | внутри самого объекта |
| Дополнительная аллокация control block | есть | отсутствует |
| Указатель из `T*` | нет | да — счётчик уже в объекте |
| Аналог `weak_ptr` | есть | нет (потребовал бы второго счётчика в объекте) |
| Стратегия подсчёта | фиксированная | через ADL, можно подключить свою без наследования |


### EBO через `CompressedPair`

Если Deleter — класс без состояния (`default_delete<T>`), наивное хранение `T* ptr; Deleter deleter;` всё равно тратит на `deleter` минимум 1 байт. `CompressedPair<T1, T2>` наследуется от `T2`, когда тот пустой и не `final` — пустые базовые классы не увеличивают размер производного объекта. Выбор между наследованием и полем делается на этапе компиляции через `std::is_empty_v<T2> && !std::is_final_v<T2>`.

Подтверждено `static_assert`'ом и на практике:

```cpp
sizeof(unique_ptr<int, EmptyDeleter>) == sizeof(int*)   // == 8 на 64-битной платформе
```

### Exception safety

Библиотека построена на RAII, поэтому большинство операций exception-safe естественным образом — без явной логики отката. Отдельно обеспечено:

- **`shared_ptr(T* p)`** — если аллокация control block бросает `std::bad_alloc`, объект `p` был бы иначе утерян навсегда (owned, но никем не удалён). Реализация перехватывает исключение, удаляет `p` и пробрасывает исходную ошибку дальше.
- **`shared_from_this()`** — вызов на объекте, не находящемся под управлением `shared_ptr` бросает `mtk::bad_weak_ptr`.
- **Строгая гарантия исключений** в операциях вроде `reset()` — достигается естественно, через промежуточные временные объекты: если их конструирование бросает, исходный указатель остаётся неизменным.

## Осознанные ограничения

- Нет allocator-aware версий (`allocate_shared` не реализован).
- `intrusive_ptr` не имеет weak-ссылок .
- `shared_ptr`/`weak_ptr` не поддерживают массивы, в отличие от `unique_ptr`.
- Нет `atomic_load`/`atomic_store` для `shared_ptr` — потокобезопасны независимые экземпляры `shared_ptr`, но не конкурентная запись в один и тот же объект `shared_ptr` из разных потоков.

## Сборка и запуск тестов

```bash
mkdir build
cd build

cmake ..
cmake --build .
ctest --output-on-failure
```

## Структура проекта

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
└── test/
    ├── all_tests.cpp
    └── make_shared_allocation_check.cpp
```