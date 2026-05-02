#pragma once

#include "wowlib/data_buffer.h"

#include <cstdint>
#include <string>
#include <map>
#include <vector>
#include <variant>
#include <optional>

namespace wowlib
{

/// Value type for a DB2 field. Matches the original wow.export data model.
using FieldValue = std::variant<
    int64_t,
    uint64_t,
    float,
    std::string,
    std::vector<int64_t>,
    std::vector<uint64_t>,
    std::vector<float>,
    std::vector<std::string>
>;

/// A single DB2 record as a map of field name -> value.
using DataRecord = std::map<std::string, FieldValue>;

/// Stub — DB2/WDC reader for WoW database tables.
/// Full implementation will be migrated from wow.export.cpp.
class Db2Reader
{
public:
    explicit Db2Reader(DataBuffer data);

    size_t size() const;
    std::optional<DataRecord> getRow(uint32_t recordID);
    const std::map<uint32_t, DataRecord>& getAllRows();

private:
    DataBuffer m_data;
    std::map<uint32_t, DataRecord> m_rows;
    bool m_parsed = false;
};

} // namespace wowlib
