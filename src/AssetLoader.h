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
    std::unordered_map<std::string, std::vector<Texture2D>> textureGroups; // animation frames are grouped together
    std::unordered_map<std::string, Tileset> tilesets;
    std::unordered_map<std::string, Font> fonts;
    std::unordered_map<std::string, std::unique_ptr<TileMap>> tileMaps;
    std::unordered_map<std::string, std::shared_ptr<Shader>> shaders;
    std::unordered_map<std::string, Music> musicTracks;
    std::unordered_map<std::string, Sound> sounds;
    nlohmann::json settings;
    nlohmann::json spriteData;
    
public:
    ~AssetLoader();
    void loadTexturesFromDirectory(const std::string& directory);
    void loadTextures(const std::unordered_map<std::string, std::vector<std::string>>& textureMap);
    void LoadTileset(const std::string& filename, int tileSize);
    void LoadTilesetFromTiled(const std::string& filename);
    void LoadTileMapFromTiled(const std::string& filename);
    void LoadFont(const std::string& filename);
    void LoadShaderFile(const std::string& filename);
    void loadSettings(const std::string& filename);
    void loadSpriteData(const std::string& filename);
    void LoadMusicFile(const std::string& filename, const float volume = 1.0f, const std::string& key = "");
    void LoadSoundFile(const std::string& filename, const float volume = 1.0f, const std::string& key = "");

    const std::vector<Texture2D>& getTextures(const std::string& key);
    const TileMap& getTilemap(const std::string& key);
    const Tileset& getTileset(const std::string& key);
    const Font& getFont(const std::string& key);
    const Shader& getShader(const std::string& key);
    const Music& getMusic(const std::string& key);
    const Sound& getSound(const std::string& key);
    const nlohmann::json& getSettings();
    const nlohmann::json& getSpriteData();
    Texture2D fallbackTexture;
};
