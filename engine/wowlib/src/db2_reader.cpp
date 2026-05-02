#include "wowlib/db2_reader.h"

#include <stdexcept>

namespace wowlib
{

Db2Reader::Db2Reader(DataBuffer data)
    : m_data(std::move(data))
{
}

size_t Db2Reader::size() const
{
    return m_rows.size();
}

std::optional<DataRecord> Db2Reader::getRow(uint32_t recordID)
{
    // TODO: Implement WDC3/WDC4/WDC5 parsing from wow.export.cpp
    auto it = m_rows.find(recordID);
    if (it != m_rows.end())
        return it->second;
    return std::nullopt;
}

const std::map<uint32_t, DataRecord>& Db2Reader::getAllRows()
{
    return m_rows;
}

} // namespace wowlib
