#pragma once

#include <cstdint>
#include <functional>

namespace placeholder::resources
{

/// Opaque, type-safe handle to a managed resource.
///
/// Contains an index into a ResourcePool and a generation counter
/// for use-after-free detection. A handle with generation 0 is null.
template<typename T>
struct ResourceHandle
{
    uint32_t index = 0;
    uint32_t generation = 0;

    /// @return true if the handle has been assigned (generation > 0).
    bool isValid() const { return generation != 0; }

    explicit operator bool() const { return isValid(); }

    bool operator==(const ResourceHandle&) const = default;
    bool operator!=(const ResourceHandle&) const = default;
};

} // namespace placeholder::resources

namespace std
{

template<typename T>
struct hash<placeholder::resources::ResourceHandle<T>>
{
    size_t operator()(const placeholder::resources::ResourceHandle<T>& h) const noexcept
    {
        size_t seed = hash<uint32_t>{}(h.index);
        seed ^= hash<uint32_t>{}(h.generation) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

} // namespace std
