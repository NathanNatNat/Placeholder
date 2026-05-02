#pragma once

#include "wowlib/data_buffer.h"
#include "wowlib/m2_types.h"

namespace wowlib
{

/// Stub — M2 model parser. Full implementation will be migrated from wow.export.cpp.
class M2Loader
{
public:
    explicit M2Loader(DataBuffer data);

    /// Parse the M2 data. Must be called before accessing m2Data.
    void load();

    const M2Data& getData() const { return m_data; }

private:
    DataBuffer m_buffer;
    M2Data m_data;
    bool m_loaded = false;
};

} // namespace wowlib
