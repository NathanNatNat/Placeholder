#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace wowlib
{

/// Resolves and ranks CDN hosts by latency for optimal download speed.
class CdnResolver
{
public:
    struct HostResult
    {
        std::string host;
        int pingMs = 0;
    };

    /// Get the fastest responding CDN host for the given hosts list.
    /// @param hosts Space-separated list of CDN hostnames
    /// @param cdnPath CDN path prefix
    /// @param configKey A config key to use as a ping test file
    /// @return Full URL of the best host (e.g., "http://host/cdnPath/")
    std::string getBestHost(std::string_view hosts, std::string_view cdnPath,
                             std::string_view configKey = "");

    /// Mark a host as failed so it won't be retried.
    void markHostFailed(const std::string& host);

private:
    std::vector<HostResult> resolveHosts(std::string_view hosts, std::string_view cdnPath);

    std::mutex m_mutex;
    std::unordered_set<std::string> m_failedHosts;
    std::unordered_map<std::string, std::string> m_cache;
};

} // namespace wowlib
