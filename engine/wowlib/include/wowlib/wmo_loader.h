#pragma once

#include "wowlib/data_buffer.h"
#include "wowlib/wmo_types.h"

namespace wowlib
{

/// WMO root file parser. Parses the root WMO which contains materials,
/// doodad placements, and group file references.
class WmoLoader
{
public:
    explicit WmoLoader(DataBuffer data);

    void load();
    const WMOData& getData() const { return m_data; }

    /// Parse a WMO group file into geometry data.
    static WMOGroupData loadGroup(DataBuffer data);

private:
    void handleChunk(uint32_t chunkID, uint32_t chunkSize);
    void parseMOHD();
    void parseMOMT(uint32_t chunkSize);
    void parseMOGI(uint32_t chunkSize);
    void parseMODS(uint32_t chunkSize);
    void parseMODI(uint32_t chunkSize);
    void parseMODD(uint32_t chunkSize);
    void parseGFID(uint32_t chunkSize);
    void parseMOTX(uint32_t chunkSize);

    DataBuffer m_buffer;
    WMOData m_data;
    bool m_loaded = false;
};

} // namespace wowlib
