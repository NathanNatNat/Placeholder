#include "wowlib/casc_remote.h"

#include <stdexcept>

namespace wowlib
{

CascRemote::CascRemote(const std::string& region, TactKeys& keys, uint32_t locale)
    : CascSource(keys, locale), m_region(region)
{
}

BLTEReader CascRemote::getFileAsBLTE(uint32_t fileDataID, bool partialDecrypt)
{
    // TODO: Implement CDN file fetching
    throw std::runtime_error("CascRemote::getFileAsBLTE not yet implemented");
}

DataBuffer CascRemote::getDataFile(const std::string& file)
{
    // TODO: Implement CDN data download
    throw std::runtime_error("CascRemote::getDataFile not yet implemented");
}

} // namespace wowlib
