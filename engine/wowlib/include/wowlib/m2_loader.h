#pragma once

#include "wowlib/data_buffer.h"
#include "wowlib/m2_types.h"

namespace wowlib
{

/// M2 model parser. Parses WoW's M2 model format into structured data.
/// Handles the chunked outer structure (MD21, SFID, TXID, SKID, BFID, AFID)
/// and the MD21 interior header with all geometry, texture, and material data.
class M2Loader
{
public:
    explicit M2Loader(DataBuffer data);

    /// Parse the M2 file. Must be called before accessing data().
    void load();

    /// Parse a skin file and store it in the provided M2SkinData.
    static void loadSkin(DataBuffer skinData, M2SkinData& skin);

    const M2Data& data() const { return m_data; }
    M2Data& data() { return m_data; }

private:
    static constexpr uint32_t CHUNK_MD21 = 0x3132444D;
    static constexpr uint32_t CHUNK_MD20 = 0x3032444D;
    static constexpr uint32_t CHUNK_SFID = 0x44494653;
    static constexpr uint32_t CHUNK_TXID = 0x44495854;
    static constexpr uint32_t CHUNK_SKID = 0x44494B53;
    static constexpr uint32_t CHUNK_BFID = 0x44494642;
    static constexpr uint32_t CHUNK_AFID = 0x44494641;

    static constexpr uint32_t SKIN_MAGIC = 0x4E494B53;

    void parseMD21();
    void parseSFID(uint32_t chunkSize);
    void parseTXID();
    void parseBFID(uint32_t chunkSize);
    void parseAFID(uint32_t chunkSize);

    void parseMD21_modelName(uint32_t ofs);
    void parseMD21_globalLoops(uint32_t ofs);
    void parseMD21_animations(uint32_t ofs);
    void parseMD21_animationLookup(uint32_t ofs);
    void parseMD21_bones(uint32_t ofs);
    void parseMD21_vertices(uint32_t ofs);
    void parseMD21_colors(uint32_t ofs);
    void parseMD21_textures(uint32_t ofs);
    void parseMD21_materials(uint32_t ofs);
    void parseMD21_textureCombos(uint32_t ofs);
    void parseMD21_transparencyLookup(uint32_t ofs);
    void parseMD21_textureTransformLookup(uint32_t ofs);
    void parseMD21_replaceableTextureLookup(uint32_t ofs);
    void parseMD21_collision(uint32_t ofs);

    DataBuffer m_buffer;
    M2Data m_data;
    bool m_loaded = false;
    bool m_md21Parsed = false;
};

} // namespace wowlib
