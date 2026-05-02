#pragma once

#include "wowlib/data_buffer.h"
#include "wowlib/wmo_types.h"

namespace wowlib
{

/// Stub — WMO (World Map Object) parser.
/// Full implementation will be migrated from wow.export.cpp.
class WmoLoader
{
public:
    explicit WmoLoader(DataBuffer data);

    void load();

    const WMOData& getData() const { return m_data; }

private:
    DataBuffer m_buffer;
    WMOData m_data;
    bool m_loaded = false;
};

} // namespace wowlib
