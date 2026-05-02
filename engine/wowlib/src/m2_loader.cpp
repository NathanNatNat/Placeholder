#include "wowlib/m2_loader.h"

#include <stdexcept>

namespace wowlib
{

M2Loader::M2Loader(DataBuffer data)
    : m_buffer(std::move(data))
{
}

void M2Loader::load()
{
    if (m_loaded)
        return;

    // TODO: Implement full M2 chunk parsing from wow.export.cpp
    // Parse MD21, SFID, TXID, SKID, BFID, AFID chunks

    m_loaded = true;
}

} // namespace wowlib
