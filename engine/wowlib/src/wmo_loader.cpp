#include "wowlib/wmo_loader.h"

#include <stdexcept>

namespace wowlib
{

WmoLoader::WmoLoader(DataBuffer data)
    : m_buffer(std::move(data))
{
}

void WmoLoader::load()
{
    if (m_loaded)
        return;

    // TODO: Implement full WMO chunk parsing from wow.export.cpp
    // Parse MVER, MOHD, MOTX, MOMT, MOGI, MODS, MODI, MODD, GFID chunks

    m_loaded = true;
}

} // namespace wowlib
