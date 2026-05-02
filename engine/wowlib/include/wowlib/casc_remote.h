#pragma once

#include "wowlib/casc_source.h"
#include "wowlib/cdn_resolver.h"

#include <string>

namespace wowlib
{

/// CASC source using Blizzard's CDN for remote file access.
/// Stub — full implementation deferred until local CASC is verified working.
class CascRemote : public CascSource
{
public:
    CascRemote(const std::string& region, TactKeys& keys,
               uint32_t locale = locale_flags::EN_US);

    BLTEReader getFileAsBLTE(uint32_t fileDataID, bool partialDecrypt = false) override;
    DataBuffer getDataFile(const std::string& file) override;

private:
    std::string m_region;
    CdnResolver m_resolver;
};

} // namespace wowlib
