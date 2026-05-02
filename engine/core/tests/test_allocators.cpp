#include <gtest/gtest.h>
#include "core/allocators.h"

#include <cstdint>
#include <set>

using namespace placeholder::core;

// --- LinearAllocator ---

TEST(LinearAllocatorTest, BasicAllocation)
{
    LinearAllocator alloc(1024);
    EXPECT_EQ(alloc.capacity(), 1024u);
    EXPECT_EQ(alloc.used(), 0u);

    void* ptr = alloc.allocate(64);
    EXPECT_NE(ptr, nullptr);
    EXPECT_GE(alloc.used(), 64u);
}

TEST(LinearAllocatorTest, MultipleAllocations)
{
    LinearAllocator alloc(1024);

    void* a = alloc.allocate(100);
    void* b = alloc.allocate(200);
    void* c = alloc.allocate(300);

    EXPECT_NE(a, nullptr);
    EXPECT_NE(b, nullptr);
    EXPECT_NE(c, nullptr);
    EXPECT_NE(a, b);
    EXPECT_NE(b, c);
}

TEST(LinearAllocatorTest, AlignmentIsRespected)
{
    LinearAllocator alloc(1024);

    alloc.allocate(1);
    void* aligned = alloc.allocate(16, 16);

    EXPECT_NE(aligned, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(aligned) % 16, 0u);
}

TEST(LinearAllocatorTest, CapacityExhaustion)
{
    LinearAllocator alloc(128);

    void* a = alloc.allocate(64);
    void* b = alloc.allocate(64);
    EXPECT_NE(a, nullptr);

    void* c = alloc.allocate(64);
    EXPECT_EQ(c, nullptr);
    static_cast<void>(b);
}

TEST(LinearAllocatorTest, ResetReclaimsMemory)
{
    LinearAllocator alloc(256);

    alloc.allocate(128);
    EXPECT_GE(alloc.used(), 128u);

    alloc.reset();
    EXPECT_EQ(alloc.used(), 0u);

    void* ptr = alloc.allocate(256);
    EXPECT_NE(ptr, nullptr);
}

TEST(LinearAllocatorTest, TypedAllocation)
{
    LinearAllocator alloc(1024);

    int* ints = alloc.allocate<int>(10);
    EXPECT_NE(ints, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ints) % alignof(int), 0u);

    for (int i = 0; i < 10; ++i)
    {
        ints[i] = i * 100;
    }
    for (int i = 0; i < 10; ++i)
    {
        EXPECT_EQ(ints[i], i * 100);
    }
}

// --- PoolAllocator ---

TEST(PoolAllocatorTest, BasicAllocateAndFree)
{
    PoolAllocator pool(64, 10);
    EXPECT_EQ(pool.blockSize(), 64u);
    EXPECT_EQ(pool.blockCount(), 10u);
    EXPECT_EQ(pool.freeCount(), 10u);

    void* ptr = pool.allocate();
    EXPECT_NE(ptr, nullptr);
    EXPECT_EQ(pool.freeCount(), 9u);

    pool.free(ptr);
    EXPECT_EQ(pool.freeCount(), 10u);
}

TEST(PoolAllocatorTest, AllocateAllBlocks)
{
    const size_t count = 8;
    PoolAllocator pool(32, count);

    std::vector<void*> ptrs;
    for (size_t i = 0; i < count; ++i)
    {
        void* p = pool.allocate();
        EXPECT_NE(p, nullptr);
        ptrs.push_back(p);
    }
    EXPECT_EQ(pool.freeCount(), 0u);

    void* overflow = pool.allocate();
    EXPECT_EQ(overflow, nullptr);

    for (void* p : ptrs)
    {
        pool.free(p);
    }
    EXPECT_EQ(pool.freeCount(), count);
}

TEST(PoolAllocatorTest, AllPointersUnique)
{
    PoolAllocator pool(64, 16);
    std::set<void*> ptrs;

    for (int i = 0; i < 16; ++i)
    {
        void* p = pool.allocate();
        EXPECT_TRUE(ptrs.insert(p).second) << "Duplicate pointer at allocation " << i;
    }

    for (void* p : ptrs)
    {
        pool.free(p);
    }
}

TEST(PoolAllocatorTest, FreeAndReallocate)
{
    PoolAllocator pool(64, 4);

    void* a = pool.allocate();
    void* b = pool.allocate();
    pool.free(a);

    void* c = pool.allocate();
    EXPECT_NE(c, nullptr);
    EXPECT_EQ(c, a);

    pool.free(b);
    pool.free(c);
}

TEST(PoolAllocatorTest, MinimumBlockSize)
{
    PoolAllocator pool(1, 4);
    EXPECT_GE(pool.blockSize(), sizeof(void*));

    void* ptr = pool.allocate();
    EXPECT_NE(ptr, nullptr);
    pool.free(ptr);
}

// --- StackAllocator ---

TEST(StackAllocatorTest, BasicAllocation)
{
    StackAllocator alloc(1024);
    EXPECT_EQ(alloc.capacity(), 1024u);
    EXPECT_EQ(alloc.used(), 0u);

    void* ptr = alloc.allocate(64);
    EXPECT_NE(ptr, nullptr);
    EXPECT_GE(alloc.used(), 64u);
}

TEST(StackAllocatorTest, MarkerAndRollback)
{
    StackAllocator alloc(1024);

    alloc.allocate(128);
    auto marker = alloc.getMarker();

    alloc.allocate(256);
    EXPECT_GE(alloc.used(), 384u);

    alloc.rollback(marker);
    EXPECT_EQ(alloc.used(), marker);
}

TEST(StackAllocatorTest, ResetToEmpty)
{
    StackAllocator alloc(512);

    alloc.allocate(100);
    alloc.allocate(100);
    alloc.reset();

    EXPECT_EQ(alloc.used(), 0u);

    void* ptr = alloc.allocate(512);
    EXPECT_NE(ptr, nullptr);
}

TEST(StackAllocatorTest, CapacityExhaustion)
{
    StackAllocator alloc(128);

    void* a = alloc.allocate(100);
    EXPECT_NE(a, nullptr);

    void* b = alloc.allocate(100);
    EXPECT_EQ(b, nullptr);
}

TEST(StackAllocatorTest, AlignmentIsRespected)
{
    StackAllocator alloc(1024);

    alloc.allocate(3);
    void* aligned = alloc.allocate(16, 16);

    EXPECT_NE(aligned, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(aligned) % 16, 0u);
}

TEST(StackAllocatorTest, NestedScopes)
{
    StackAllocator alloc(1024);

    auto outer = alloc.getMarker();
    alloc.allocate(64);

    auto inner = alloc.getMarker();
    alloc.allocate(128);

    alloc.rollback(inner);
    EXPECT_EQ(alloc.used(), inner);

    alloc.allocate(64);
    alloc.rollback(outer);
    EXPECT_EQ(alloc.used(), outer);
}

TEST(StackAllocatorTest, TypedAllocation)
{
    StackAllocator alloc(1024);

    double* vals = alloc.allocate<double>(5);
    EXPECT_NE(vals, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(vals) % alignof(double), 0u);

    for (int i = 0; i < 5; ++i)
    {
        vals[i] = static_cast<double>(i) * 1.5;
    }
    for (int i = 0; i < 5; ++i)
    {
        EXPECT_DOUBLE_EQ(vals[i], static_cast<double>(i) * 1.5);
    }
}
