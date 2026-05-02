#pragma once

#include <cstddef>
#include <cstdint>
#include <cassert>
#include <memory>
#include <vector>

namespace placeholder::core
{

/// Bump-pointer allocator that hands out memory linearly.
/// Reset it each frame to reclaim all memory at once.
class LinearAllocator
{
public:
    explicit LinearAllocator(size_t capacityBytes);
    ~LinearAllocator();

    LinearAllocator(const LinearAllocator&) = delete;
    LinearAllocator& operator=(const LinearAllocator&) = delete;

    /// Allocate aligned memory. Returns nullptr if capacity is exhausted.
    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t));

    /// Typed allocation helper.
    template<typename T>
    T* allocate(size_t count = 1)
    {
        return static_cast<T*>(allocate(sizeof(T) * count, alignof(T)));
    }

    /// Reclaim all allocated memory without freeing the backing buffer.
    void reset();

    size_t capacity() const { return m_capacity; }
    size_t used() const { return m_offset; }

private:
    uint8_t* m_buffer = nullptr;
    size_t m_capacity = 0;
    size_t m_offset = 0;
};

/// Fixed-size block allocator. All blocks have the same size.
/// Free blocks are tracked via an embedded free list.
class PoolAllocator
{
public:
    /// @param blockSize Size of each block (minimum 8 bytes for free-list pointer).
    /// @param blockCount Number of blocks to pre-allocate.
    PoolAllocator(size_t blockSize, size_t blockCount);
    ~PoolAllocator();

    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;

    /// Allocate a single block. Returns nullptr if the pool is exhausted.
    void* allocate();

    /// Free a previously allocated block.
    void free(void* ptr);

    /// Typed helpers.
    template<typename T>
    T* allocate() { return static_cast<T*>(allocate()); }

    template<typename T>
    void free(T* ptr) { free(static_cast<void*>(ptr)); }

    size_t blockSize() const { return m_blockSize; }
    size_t blockCount() const { return m_blockCount; }
    size_t freeCount() const { return m_freeCount; }

private:
    struct FreeBlock
    {
        FreeBlock* next;
    };

    uint8_t* m_buffer = nullptr;
    size_t m_blockSize = 0;
    size_t m_blockCount = 0;
    size_t m_freeCount = 0;
    FreeBlock* m_freeList = nullptr;
};

/// LIFO stack allocator. Allocations must be freed in reverse order.
class StackAllocator
{
public:
    explicit StackAllocator(size_t capacityBytes);
    ~StackAllocator();

    StackAllocator(const StackAllocator&) = delete;
    StackAllocator& operator=(const StackAllocator&) = delete;

    /// Opaque marker for rolling back allocations.
    using Marker = size_t;

    /// Get the current stack position.
    Marker getMarker() const { return m_offset; }

    /// Allocate aligned memory from the top of the stack.
    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t));

    /// Typed allocation helper.
    template<typename T>
    T* allocate(size_t count = 1)
    {
        return static_cast<T*>(allocate(sizeof(T) * count, alignof(T)));
    }

    /// Roll back to a previous marker, freeing everything allocated after it.
    void rollback(Marker marker);

    /// Reset to empty (equivalent to rollback(0)).
    void reset();

    size_t capacity() const { return m_capacity; }
    size_t used() const { return m_offset; }

private:
    uint8_t* m_buffer = nullptr;
    size_t m_capacity = 0;
    size_t m_offset = 0;
};

} // namespace placeholder::core
