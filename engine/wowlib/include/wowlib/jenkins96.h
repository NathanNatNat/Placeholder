#pragma once

#include <cstdint>
#include <span>
#include <utility>

namespace wowlib
{

/// Jenkins 96-bit hash function. Returns (pc, pb) hash pair.
std::pair<uint32_t, uint32_t> jenkins96(std::span<const uint8_t> k, uint32_t init = 0, uint32_t init2 = 0);

} // namespace wowlib
