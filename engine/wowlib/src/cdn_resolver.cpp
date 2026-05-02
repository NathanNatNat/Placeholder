#include "wowlib/cdn_resolver.h"

#include <httplib.h>
#include <chrono>
#include <algorithm>

namespace wowlib
{

std::string CdnResolver::getBestHost(std::string_view hosts, std::string_view cdnPath,
                                       std::string_view configKey)
{
    std::string cacheKey = std::string(hosts) + "|" + std::string(cdnPath);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_cache.find(cacheKey);
        if (it != m_cache.end())
            return it->second;
    }

    auto ranked = resolveHosts(hosts, cdnPath);
    if (ranked.empty())
        throw std::runtime_error("No CDN hosts available");

    std::string bestUrl = "http://" + ranked[0].host + "/" + std::string(cdnPath) + "/";

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cache[cacheKey] = bestUrl;
    }

    return bestUrl;
}

void CdnResolver::markHostFailed(const std::string& host)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_failedHosts.insert(host);
    m_cache.clear();
}

std::vector<CdnResolver::HostResult> CdnResolver::resolveHosts(std::string_view hosts,
                                                                  std::string_view cdnPath)
{
    std::vector<std::string> hostList;
    size_t start = 0;
    for (size_t i = 0; i <= hosts.size(); i++)
    {
        if (i == hosts.size() || hosts[i] == ' ')
        {
            if (i > start)
                hostList.emplace_back(hosts.substr(start, i - start));
            start = i + 1;
        }
    }

    std::vector<HostResult> results;
    for (const auto& host : hostList)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_failedHosts.count(host))
                continue;
        }

        auto startTime = std::chrono::steady_clock::now();
        try
        {
            httplib::Client cli("http://" + host);
            cli.set_connection_timeout(3, 0);
            cli.set_read_timeout(3, 0);
            auto res = cli.Head("/" + std::string(cdnPath) + "/");
            auto endTime = std::chrono::steady_clock::now();
            int pingMs = static_cast<int>(
                std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
            results.push_back({ host, pingMs });
        }
        catch (...)
        {
            // Host unreachable, skip it
        }
    }

    std::sort(results.begin(), results.end(),
        [](const HostResult& a, const HostResult& b) { return a.pingMs < b.pingMs; });

    return results;
}

} // namespace wowlib
