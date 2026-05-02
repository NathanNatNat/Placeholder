#include "core/allocators.h"

#include <cstdlib>
#include <cstring>

namespace placeholder::core
{

// -- LinearAllocator --

LinearAllocator::LinearAllocator(size_t capacityBytes)
    : m_capacity(capacityBytes)
{
    m_buffer = static_cast<uint8_t*>(std::malloc(capacityBytes));
}

LinearAllocator::~LinearAllocator()
{
    std::free(m_buffer);
}

void* LinearAllocator::allocate(size_t size, size_t alignment)
{
    uintptr_t current = reinterpret_cast<uintptr_t>(m_buffer + m_offset);
    uintptr_t aligned = (current + alignment - 1) & ~(alignment - 1);
    size_t padding = aligned - current;

    if (m_offset + padding + size > m_capacity)
    {
        return nullptr;
    }

    m_offset += padding + size;
    return reinterpret_cast<void*>(aligned);
}

void LinearAllocator::reset()
{
    m_offset = 0;
}

// -- PoolAllocator --

PoolAllocator::PoolAllocator(size_t blockSize, size_t blockCount)
    : m_blockSize(blockSize < sizeof(FreeBlock) ? sizeof(FreeBlock) : blockSize)
    , m_blockCount(blockCount)
    , m_freeCount(blockCount)
{
    m_buffer = static_cast<uint8_t*>(std::malloc(m_blockSize * blockCount));

    m_freeList = nullptr;
    for (size_t i = blockCount; i > 0; --i)
    {
        auto* block = reinterpret_cast<FreeBlock*>(m_buffer + (i - 1) * m_blockSize);
        block->next = m_freeList;
        m_freeList = block;
    }
}

PoolAllocator::~PoolAllocator()
{
    std::free(m_buffer);
}

void* PoolAllocator::allocate()
{
    if (!m_freeList)
    {
        return nullptr;
    }

    FreeBlock* block = m_freeList;
    m_freeList = block->next;
    --m_freeCount;
    return block;
}

void PoolAllocator::free(void* ptr)
{
    if (!ptr)
    {
        return;
    }

    auto* block = static_cast<FreeBlock*>(ptr);
    block->next = m_freeList;
    m_freeList = block;
    ++m_freeCount;
}

// -- StackAllocator --

StackAllocator::StackAllocator(size_t capacityBytes)
    : m_capacity(capacityBytes)
{
    m_buffer = static_cast<uint8_t*>(std::malloc(capacityBytes));
}

StackAllocator::~StackAllocator()
{
    std::free(m_buffer);
}

void* StackAllocator::allocate(size_t size, size_t alignment)
{
    uintptr_t current = reinterpret_cast<uintptr_t>(m_buffer + m_offset);
    uintptr_t aligned = (current + alignment - 1) & ~(alignment - 1);
    size_t padding = aligned - current;

    if (m_offset + padding + size > m_capacity)
    {
        return nullptr;
    }

    m_offset += padding + size;
    return reinterpret_cast<void*>(aligned);
}

void StackAllocator::rollback(Marker marker)
{
    assert(marker <= m_offset);
    m_offset = marker;
}

void StackAllocator::reset()
{
    m_offset = 0;
}

} // namespace placeholder::core
