#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace wowlib
{

/// Parse a CDN/Build config file into a map of normalized key-value pairs.
/// Hyphenated keys are converted to camelCase (e.g., "encoding-sizes" -> "encodingSizes").
std::unordered_map<std::string, std::string> parseCdnConfig(std::string_view data);

} // namespace wowlib
