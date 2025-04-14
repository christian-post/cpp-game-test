#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "raylib.h"
#include "json.hpp"
#include "TileMap.h"

namespace fs = std::filesystem;

class AssetLoader {
private:
    std::unordered_map<std::string, std::vector<Texture2D>> textureGroups;
    std::unordered_map<std::string, Font> fonts;
    std::unordered_map<std::string, std::unique_ptr<TileMap>> tileMaps;
    
public:
    ~AssetLoader();
    void loadTexturesFromDirectory(const std::string& directory);
    void loadTextures(const std::unordered_map<std::string, std::vector<std::string>>& textureMap);
    void LoadTileset(const std::string& filename, int tileSize);
    void LoadTilesetFromTiled(const std::string& filename);
    void LoadTileMapFromTiled(const std::string& filename);
    void LoadFont(const std::string& filename);

    const std::vector<Texture2D>& getTextures(const std::string& key);
    const TileMap& getTilemap(const std::string& key);
    const Font& getFont(const std::string& key);
    Texture2D fallbackTexture;
};
