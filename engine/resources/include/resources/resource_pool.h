#pragma once

#include "resource_handle.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace placeholder::resources
{

/// Type-safe pool of resources accessed via generational handles.
///
/// Resources are stored contiguously. Removed slots are recycled via a
/// free list for O(1) reuse. Each slot carries a generation counter that
/// increments on removal, so stale handles safely return nullptr.
template<typename T>
class ResourcePool
{
public:
    /// Add a resource to the pool and return a handle.
    ResourceHandle<T> add(T resource)
    {
        uint32_t idx;

        if (!m_freeList.empty())
        {
            idx = m_freeList.back();
            m_freeList.pop_back();
            m_entries[idx].resource = std::move(resource);
            m_entries[idx].alive = true;
        }
        else
        {
            idx = static_cast<uint32_t>(m_entries.size());
            auto& entry = m_entries.emplace_back();
            entry.resource = std::move(resource);
            entry.generation = 1;
            entry.alive = true;
        }

        ResourceHandle<T> handle;
        handle.index = idx;
        handle.generation = m_entries[idx].generation;
        ++m_liveCount;
        return handle;
    }

    /// Remove the resource. Bumps the generation to invalidate stale handles.
    void remove(ResourceHandle<T> handle)
    {
        if (!isValid(handle))
        {
            return;
        }

        auto& entry = m_entries[handle.index];
        entry.alive = false;
        entry.resource = T{};
        ++entry.generation;
        m_freeList.push_back(handle.index);
        --m_liveCount;
    }

    /// @return Mutable pointer to the resource, or nullptr if stale/invalid.
    T* get(ResourceHandle<T> handle)
    {
        if (!isValid(handle))
        {
            return nullptr;
        }
        return &m_entries[handle.index].resource;
    }

    /// @return Const pointer to the resource, or nullptr if stale/invalid.
    const T* get(ResourceHandle<T> handle) const
    {
        if (!isValid(handle))
        {
            return nullptr;
        }
        return &m_entries[handle.index].resource;
    }

    /// Replace resource data in-place (for hot-reload). Handle stays valid.
    bool replace(ResourceHandle<T> handle, T resource)
    {
        if (!isValid(handle))
        {
            return false;
        }
        m_entries[handle.index].resource = std::move(resource);
        return true;
    }

    /// @return true if the handle points to a live resource.
    bool isValid(ResourceHandle<T> handle) const
    {
        if (handle.index >= static_cast<uint32_t>(m_entries.size()))
        {
            return false;
        }
        const auto& entry = m_entries[handle.index];
        return entry.alive && entry.generation == handle.generation;
    }

    /// @return Number of live resources.
    size_t count() const { return m_liveCount; }

    /// @return Total allocated slots (live + free).
    size_t capacity() const { return m_entries.size(); }

    /// Iterate all live resources. Callback: void(ResourceHandle<T>, T&).
    template<typename Fn>
    void forEach(Fn&& fn)
    {
        for (uint32_t i = 0; i < static_cast<uint32_t>(m_entries.size()); ++i)
        {
            if (m_entries[i].alive)
            {
                ResourceHandle<T> handle;
                handle.index = i;
                handle.generation = m_entries[i].generation;
                fn(handle, m_entries[i].resource);
            }
        }
    }

    /// Const iteration. Callback: void(ResourceHandle<T>, const T&).
    template<typename Fn>
    void forEach(Fn&& fn) const
    {
        for (uint32_t i = 0; i < static_cast<uint32_t>(m_entries.size()); ++i)
        {
            if (m_entries[i].alive)
            {
                ResourceHandle<T> handle;
                handle.index = i;
                handle.generation = m_entries[i].generation;
                fn(handle, m_entries[i].resource);
            }
        }
    }

private:
    struct Entry
    {
        T resource{};
        uint32_t generation = 0;
        bool alive = false;
    };

    std::vector<Entry> m_entries;
    std::vector<uint32_t> m_freeList;
    size_t m_liveCount = 0;
};

} // namespace placeholder::resources
