#include <gtest/gtest.h>
#include <thread>
#include <vector>
 
#include "unique_ptr.hpp"
#include "shared_ptr.hpp"
#include "weak_ptr.hpp"
#include "enable_shared_from_this.hpp"
#include "intrusive_ptr.hpp"
 
// ============================================================================
// unique_ptr
// ============================================================================
namespace {
 
struct Logger {
    bool* freed_flag = nullptr;
    void operator()(int* p) const {
        if (freed_flag) *freed_flag = true;
        delete p;
    }
};
 
struct UPBase {
    virtual ~UPBase() = default;
    virtual int id() const { return 0; }
};
struct UPDerived : UPBase {
    int id() const override { return 1; }
    ~UPDerived() override { *destroyed = true; }
    bool* destroyed;
    explicit UPDerived(bool* flag) : destroyed(flag) {}
};
 
} // namespace
 
TEST(UniquePtr, DefaultIsEmpty) {
    mtk::unique_ptr<int> p;
    EXPECT_FALSE(p);
    EXPECT_EQ(p.get(), nullptr);
}
 
TEST(UniquePtr, TakesOwnershipAndDereferences) {
    mtk::unique_ptr<int> p(new int(42));
    EXPECT_TRUE(p);
    EXPECT_EQ(*p, 42);
}
 
TEST(UniquePtr, MoveConstructorTransfersOwnership) {
    mtk::unique_ptr<int> p(new int(10));
    mtk::unique_ptr<int> p2 = std::move(p);
    EXPECT_FALSE(p);
    EXPECT_TRUE(p2);
    EXPECT_EQ(*p2, 10);
}
 
TEST(UniquePtr, MoveAssignmentReleasesOldResource) {
    mtk::unique_ptr<int> a(new int(1));
    mtk::unique_ptr<int> b(new int(2));
    a = std::move(b);
    EXPECT_EQ(*a, 2);
    EXPECT_FALSE(b);
}
 
TEST(UniquePtr, SelfMoveIsSafe) {
    mtk::unique_ptr<int> p(new int(7));
    p = std::move(p);
    SUCCEED();
}
 
TEST(UniquePtr, ReleaseDetachesOwnership) {
    mtk::unique_ptr<int> p(new int(5));
    int* raw = p.release();
    EXPECT_FALSE(p);
    EXPECT_EQ(*raw, 5);
    delete raw;
}
 
TEST(UniquePtr, ResetReplacesResource) {
    mtk::unique_ptr<int> p(new int(1));
    p.reset(new int(2));
    EXPECT_EQ(*p, 2);
}
 
TEST(UniquePtr, ResetSelfIsNoop) {
    mtk::unique_ptr<int> p(new int(9));
    int* raw = p.get();
    p.reset(raw);
    EXPECT_EQ(p.get(), raw);
    EXPECT_EQ(*p, 9);
}
 
TEST(UniquePtr, CustomDeleterIsInvoked) {
    bool freed = false;
    {
        mtk::unique_ptr<int, Logger> p(new int(99), Logger{&freed});
        EXPECT_FALSE(freed);
    }
    EXPECT_TRUE(freed);
}
 
TEST(UniquePtr, PolymorphicDeletionCallsDerivedDtor) {
    bool destroyed = false;
    {
        mtk::unique_ptr<UPBase> p(new UPDerived(&destroyed));
        EXPECT_EQ(p->id(), 1);
    }
    EXPECT_TRUE(destroyed);
}
 
TEST(UniquePtr, ComparisonWithNullptr) {
    mtk::unique_ptr<int> p;
    EXPECT_TRUE(p == nullptr);
    EXPECT_TRUE(nullptr == p);
    p.reset(new int(1));
    EXPECT_TRUE(p != nullptr);
}
 
TEST(UniquePtr, SwapExchangesOwnership) {
    mtk::unique_ptr<int> a(new int(1));
    mtk::unique_ptr<int> b(new int(2));
    swap(a, b);
    EXPECT_EQ(*a, 2);
    EXPECT_EQ(*b, 1);
}
 
TEST(UniquePtr, MakeUniqueScalar) {
    auto p = mtk::make_unique<int>(123);
    EXPECT_EQ(*p, 123);
}
 
TEST(UniquePtrArray, IndexingWorks) {
    mtk::unique_ptr<int[]> arr(new int[3]{1, 2, 3});
    EXPECT_EQ(arr[0], 1);
    EXPECT_EQ(arr[1], 2);
    EXPECT_EQ(arr[2], 3);
}
 
TEST(UniquePtrArray, MakeUniqueValueInitializes) {
    auto arr = mtk::make_unique<int[]>(4);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(arr[i], 0);
    }
    arr[0] = 42;
    EXPECT_EQ(arr[0], 42);
}
 
TEST(UniquePtrArray, MoveWorks) {
    mtk::unique_ptr<int[]> a(new int[2]{5, 6});
    mtk::unique_ptr<int[]> b = std::move(a);
    EXPECT_FALSE(a);
    EXPECT_EQ(b[0], 5);
    EXPECT_EQ(b[1], 6);
}
 
TEST(CompressedPairEBO, EmptyDeleterAddsNoOverhead) {
    struct EmptyDeleter { void operator()(int* p) const { delete p; } };
    static_assert(sizeof(mtk::unique_ptr<int, EmptyDeleter>) == sizeof(int*),
                  "EBO should make unique_ptr with empty deleter as small as a raw pointer");
}
 
// ============================================================================
// shared_ptr + make_shared + casts
// ============================================================================
namespace {
 
struct SPBase {
    virtual ~SPBase() = default;
    virtual int id() const { return 0; }
};
struct SPDerived : SPBase {
    int id() const override { return 1; }
    int extra = 42;
};
struct SPUnrelated : SPBase {
    int id() const override { return 2; }
};
 
} // namespace
 
TEST(SharedPtr, DefaultIsEmpty) {
    mtk::shared_ptr<int> p;
    EXPECT_FALSE(p);
    EXPECT_EQ(p.use_count(), 0u);
}
 
TEST(SharedPtr, CopyIncrementsUseCount) {
    mtk::shared_ptr<int> a(new int(1));
    EXPECT_EQ(a.use_count(), 1u);
    {
        mtk::shared_ptr<int> b = a;
        EXPECT_EQ(a.use_count(), 2u);
        EXPECT_EQ(b.use_count(), 2u);
    }
    EXPECT_EQ(a.use_count(), 1u);
}
 
TEST(SharedPtr, MoveTransfersOwnershipWithoutTouchingCount) {
    mtk::shared_ptr<int> a(new int(3));
    mtk::shared_ptr<int> b = std::move(a);
    EXPECT_FALSE(a);
    EXPECT_EQ(b.use_count(), 1u);
}
 
TEST(SharedPtr, SelfAssignmentIsSafe) {
    mtk::shared_ptr<int> p(new int(9));
    p = p;
    EXPECT_TRUE(p);
    EXPECT_EQ(*p, 9);
    EXPECT_EQ(p.use_count(), 1u);
}
 
TEST(SharedPtr, ResetReplacesResource) {
    mtk::shared_ptr<int> p(new int(1));
    p.reset(new int(2));
    EXPECT_EQ(*p, 2);
}
 
TEST(MakeShared, ProducesWorkingObject) {
    auto p = mtk::make_shared<std::pair<int, int>>(3, 4);
    EXPECT_EQ(p->first, 3);
    EXPECT_EQ(p->second, 4);
    EXPECT_EQ(p.use_count(), 1u);
}
 
TEST(PointerCast, StaticPointerCastSharesControlBlock) {
    mtk::shared_ptr<SPDerived> d = mtk::make_shared<SPDerived>();
    mtk::shared_ptr<SPBase> b = mtk::static_pointer_cast<SPBase>(d);
    EXPECT_EQ(d.use_count(), 2u);
    EXPECT_EQ(b->id(), 1);
}
 
TEST(PointerCast, DynamicPointerCastSucceeds) {
    mtk::shared_ptr<SPBase> b = mtk::static_pointer_cast<SPBase>(mtk::make_shared<SPDerived>());
    mtk::shared_ptr<SPDerived> d = mtk::dynamic_pointer_cast<SPDerived>(b);
    ASSERT_TRUE(d);
    EXPECT_EQ(d->extra, 42);
}
 
TEST(PointerCast, DynamicPointerCastFailsWithoutLeakOrCountChange) {
    mtk::shared_ptr<SPBase> b = mtk::static_pointer_cast<SPBase>(mtk::make_shared<SPUnrelated>());
    mtk::shared_ptr<SPDerived> d = mtk::dynamic_pointer_cast<SPDerived>(b);
    EXPECT_FALSE(d);
    EXPECT_EQ(b.use_count(), 1u);
}
 
TEST(PointerCast, ConstPointerCastRemovesConstness) {
    mtk::shared_ptr<const int> c = mtk::make_shared<const int>(9);
    mtk::shared_ptr<int> nc = mtk::const_pointer_cast<int>(c);
    *nc = 10;
    EXPECT_EQ(*c, 10);
}
 
// ============================================================================
// weak_ptr
// ============================================================================
 
TEST(WeakPtr, LockWhileAliveReturnsValidSharedPtr) {
    mtk::shared_ptr<int> sp(new int(55));
    mtk::weak_ptr<int> wp(sp);
    EXPECT_FALSE(wp.expired());
    auto locked = wp.lock();
    ASSERT_TRUE(locked);
    EXPECT_EQ(*locked, 55);
    EXPECT_EQ(sp.use_count(), 2u);
}
 
TEST(WeakPtr, ExpiredAfterObjectDestroyed) {
    mtk::weak_ptr<int> wp;
    {
        mtk::shared_ptr<int> sp(new int(1));
        wp = mtk::weak_ptr<int>(sp);
        EXPECT_FALSE(wp.expired());
    }
    EXPECT_TRUE(wp.expired());
    EXPECT_FALSE(wp.lock());
}
 
TEST(WeakPtr, ControlBlockSurvivesUntilLastWeakPtrGone) {
    mtk::weak_ptr<int> wp;
    {
        mtk::shared_ptr<int> sp(new int(1));
        wp = mtk::weak_ptr<int>(sp);
    }
    EXPECT_EQ(wp.use_count(), 0u);
}
 
TEST(WeakPtr, CopyAndMoveSemantics) {
    mtk::shared_ptr<int> sp(new int(1));
    mtk::weak_ptr<int> wp1(sp);
    mtk::weak_ptr<int> wp2 = wp1;
    EXPECT_FALSE(wp1.expired());
    EXPECT_FALSE(wp2.expired());
 
    mtk::weak_ptr<int> wp3 = std::move(wp1);
    EXPECT_FALSE(wp3.expired());
}
 
TEST(WeakPtr, SelfAssignmentIsSafe) {
    mtk::shared_ptr<int> sp(new int(1));
    mtk::weak_ptr<int> wp(sp);
    wp = wp;
    EXPECT_FALSE(wp.expired());
}
 
// ============================================================================
// enable_shared_from_this
// ============================================================================
namespace {
 
struct Node : mtk::enable_shared_from_this<Node> {
    int value;
    explicit Node(int v) : value(v) {}
    mtk::shared_ptr<Node> get_self() { return shared_from_this(); }
};
 
} // namespace
 
TEST(EnableSharedFromThis, WorksWithRegularConstructor) {
    mtk::shared_ptr<Node> n(new Node(1));
    mtk::shared_ptr<Node> self = n->get_self();
    EXPECT_EQ(self.get(), n.get());
    EXPECT_EQ(n.use_count(), 2u);
}
 
TEST(EnableSharedFromThis, WorksWithMakeShared) {
    mtk::shared_ptr<Node> n = mtk::make_shared<Node>(2);
    mtk::shared_ptr<Node> self = n->get_self();
    EXPECT_EQ(self.get(), n.get());
    EXPECT_EQ(n.use_count(), 2u);
}

TEST(EnableSharedFromThis, DestructionDoesNotCrashUnderAsan) {
    { mtk::shared_ptr<Node> n = mtk::make_shared<Node>(3); }
    SUCCEED();
}

TEST(EnableSharedFromThis, ThrowsBadWeakPtrIfNotOwnedByAnySharedPtr) {
    Node stack_node(4);
    EXPECT_THROW(stack_node.get_self(), mtk::bad_weak_ptr);
}

TEST(EnableSharedFromThis, DoesNotThrowWhenProperlyOwned) {
    mtk::shared_ptr<Node> n(new Node(5));
    EXPECT_NO_THROW(n->get_self());
}

// ============================================================================
// intrusive_ptr
// ============================================================================
namespace {
 
struct IPWidget : mtk::RefCounter {
    int value;
    explicit IPWidget(int v) : value(v) {}
};
 
struct IPBase : mtk::RefCounter {
    virtual ~IPBase() = default;
    virtual int id() const { return 0; }
};
struct IPDerived : IPBase {
    int id() const override { return 1; }
    bool* destroyed;
    explicit IPDerived(bool* flag) : destroyed(flag) {}
    ~IPDerived() override { *destroyed = true; }
};
 
} // namespace
 
TEST(IntrusivePtr, BasicOwnershipAndCopy) {
    mtk::intrusive_ptr<IPWidget> p(new IPWidget(1));
    EXPECT_EQ(p->value, 1);
    mtk::intrusive_ptr<IPWidget> p2 = p;
    EXPECT_EQ(p2->value, 1);
}
 
TEST(IntrusivePtr, Move) {
    mtk::intrusive_ptr<IPWidget> p(new IPWidget(2));
    mtk::intrusive_ptr<IPWidget> p2 = std::move(p);
    EXPECT_FALSE(p);
    EXPECT_EQ(p2->value, 2);
}
 
TEST(IntrusivePtr, CopyingEmptyIsSafe) {
    mtk::intrusive_ptr<IPWidget> empty;
    mtk::intrusive_ptr<IPWidget> copy = empty;
    EXPECT_FALSE(copy);
}
 
TEST(IntrusivePtr, SelfAssignmentIsSafe) {
    mtk::intrusive_ptr<IPWidget> p(new IPWidget(3));
    p = p;
    EXPECT_TRUE(p);
    EXPECT_EQ(p->value, 3);
}
 
TEST(IntrusivePtr, PolymorphicDeletionCallsDerivedDtor) {
    bool destroyed = false;
    {
        mtk::intrusive_ptr<IPBase> b(new IPDerived(&destroyed));
        EXPECT_EQ(b->id(), 1);
    }
    EXPECT_TRUE(destroyed);
}
 
TEST(IntrusivePtr, Comparisons) {
    mtk::intrusive_ptr<IPWidget> a(new IPWidget(9));
    mtk::intrusive_ptr<IPWidget> b = a;
    EXPECT_TRUE(a == b);
    mtk::intrusive_ptr<IPWidget> c(new IPWidget(9));
    EXPECT_TRUE(a != c);
    mtk::intrusive_ptr<IPWidget> n;
    EXPECT_TRUE(n == nullptr);
    EXPECT_TRUE(a != nullptr);
}
 
// ============================================================================
// multithreading (shared_ptr / weak_ptr / intrusive_ptr)
// ============================================================================
namespace {
struct ThreadPayload : mtk::RefCounter { int value = 0; };
constexpr int kThreads = 8;
constexpr int kIters = 20000;
} // namespace
 
TEST(ThreadSafety, SharedPtrConcurrentCopyAndLock) {
    mtk::shared_ptr<int> master(new int(100));
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&master]() {
            for (int i = 0; i < kIters; ++i) {
                mtk::shared_ptr<int> copy = master;
                mtk::weak_ptr<int> w(copy);
                auto locked = w.lock();
                (void)locked;
            }
        });
    }
    for (auto& th : threads) th.join();
    EXPECT_EQ(master.use_count(), 1u);
}
 
TEST(ThreadSafety, IntrusivePtrConcurrentCopy) {
    mtk::intrusive_ptr<ThreadPayload> master(new ThreadPayload());
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&master]() {
            for (int i = 0; i < kIters; ++i) {
                mtk::intrusive_ptr<ThreadPayload> copy = master;
                (void)copy;
            }
        });
    }
    for (auto& th : threads) th.join();
    SUCCEED();
}
 