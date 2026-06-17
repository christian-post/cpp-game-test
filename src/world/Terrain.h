#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

// terrain types a floor tile can represent
// ground is the implicit default for any tile not listed in tile_properties.json
enum class TerrainType
{
    Ground,
    Water
};

// capabilities a sprite can have (e.g., granted by items)
// stored as a bitmask in Sprite::traversal
enum Traversal : uint32_t
{
    TRAVERSE_NONE = 0,
    TRAVERSE_WATER = 1 << 0
};

inline TerrainType terrainTypeFromString(const std::string& name)
{
    if (name == "water")
        return TerrainType::Water;
    return TerrainType::Ground;
}

inline TerrainType getTerrainType(int gid, const std::unordered_map<int, TerrainType>& lookup)
{
    auto it = lookup.find(gid);
    if (it != lookup.end())
        return it->second;
    return TerrainType::Ground;
}

inline bool isBlocking(TerrainType type, uint32_t traversal)
{
    switch (type)
    {
    case TerrainType::Water:
        return !(traversal & TRAVERSE_WATER);
    default:
        return false;
    }
}