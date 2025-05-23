#include "AssetLoader.h"
#include <iostream>


AssetLoader::~AssetLoader() {
    // textures
    for (auto& it : textureGroups) {
        for (Texture2D& texture : it.second) {
            UnloadTexture(texture);
        }
    }
    // fonts
    for (auto& it : fonts) {
        UnloadFont(it.second);
    }
    fonts.clear();
    // unload the music and sounds
    for (auto& it : musicTracks) {
        UnloadMusicStream(it.second);
    }
    for (auto& it : sounds) {
        UnloadSound(it.second);
    }
}

void AssetLoader::loadTexturesFromDirectory(const std::string& directory) {
    // Looks for png files in a given directory that match the pattern key_n.png
    // and automatically groups and loads them
    // TODO: currently unused
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.path().extension() == ".png") {
            std::string filename = entry.path().filename().string();

            size_t pos = filename.find('_');
            if (pos != std::string::npos) {
                std::string key = filename.substr(0, pos);
                textureGroups[key].push_back(LoadTexture(entry.path().string().c_str()));
            }
        }
    }
}

void AssetLoader::loadTextures(const std::unordered_map<std::string, std::vector<std::string>>& textureMap) {
    // loads textures by filenames
    /*  Example:
    assetLoader.loadTextures({
        {"player_idle", {"assets/player_idle_1.png", "assets/player_idle_2.png"}},
        {"player_run", {"assets/player_run_1.png", "assets/player_run_2.png", "assets/player_run_3.png"}}
    });
    */
    namespace fs = std::filesystem;

    for (const auto& pair : textureMap) {
        const std::string& key = pair.first;
        for (const std::string& filename : pair.second) {
            // TODO: maybe raylib does check this internally?
            if (fs::exists(filename)) {
                textureGroups[key].push_back(LoadTexture(filename.c_str()));
            }
            else {
                TraceLog(LOG_ERROR, "ERROR: File not found:  %s", filename.c_str());
            }
        }
    }
}

void AssetLoader::LoadTileset(const std::string& filename, int tileSize) {
    // Loads the tiles directly from an image, give the correct tile size
    // TODO: depreacted
    std::vector<Texture2D> tiles;
    Image tilesetImg = LoadImage(filename.c_str());

    if (tilesetImg.width == 0 || tilesetImg.height == 0) {
        return; // Return if loading failed
    }

    int tilesX = tilesetImg.width / tileSize;
    int tilesY = tilesetImg.height / tileSize;

    for (int y = 0; y < tilesY; y++) {
        for (int x = 0; x < tilesX; x++) {
            Rectangle tileRect = { float(x * tileSize), float(y * tileSize), float(tileSize), float(tileSize) };
            Image tileImg = ImageFromImage(tilesetImg, tileRect);
            tiles.push_back(LoadTextureFromImage(tileImg));
            UnloadImage(tileImg);
        }
    }

    UnloadImage(tilesetImg);
    std::string baseName = std::filesystem::path(filename).stem().string();
    textureGroups[baseName] = tiles;
    TraceLog(LOG_INFO, "Tileset loaded successfully: %s", baseName.c_str());
}

void AssetLoader::LoadTilesetFromTiled(const std::string& filename) {
    // loads a Tiled tileset file (.json or .tsj)
    std::ifstream file(filename); // Open JSON file
    if (!file) {
        TraceLog(LOG_ERROR, "Failed to open tileset file %s", filename.c_str());
        return;
    }
    nlohmann::json j;
    file >> j; // construct a json-like object from the file data

    if (!j.contains("image") || !j.contains("tilewidth")) {
        TraceLog(LOG_ERROR, "Invalid tileset format: missing 'image' or 'tilewidth'.");
        return;
    }

    std::string imageFile = j["image"];
    int tileSize = j["tilewidth"];  // assume tiles are square

    // Resolve relative to the .tsj file directory
    std::filesystem::path tilesetDir = std::filesystem::path(filename).parent_path();
    std::filesystem::path fullImagePath = tilesetDir / imageFile;
    fullImagePath = fullImagePath.lexically_normal(); // Clean up '..' parts

    LoadTileset(fullImagePath.string(), tileSize);
}

void AssetLoader::LoadTileMapFromTiled(const std::string& filename) {
    // Loads a Tiled map file (json or tsj)
    // a TileMap is a class that stores collections of TileLayer (2D arrays of tile indices)
    // and TileObjects (contains information to construct game entities)
    std::ifstream file(filename);
    if (!file) {
        TraceLog(LOG_ERROR, "Failed to open tilemap file %s", filename.c_str());
        return;
    }
    nlohmann::json j;
    file >> j;

    std::string baseName = std::filesystem::path(filename).stem().string();
    tileMaps[baseName] = std::make_unique<TileMap>(j, baseName);
    TraceLog(LOG_INFO, "Tilemap file loaded successfully: %s", baseName.c_str());
}

void  AssetLoader::LoadFont(const std::string& filename) {
    Font fontTtf = LoadFontEx(filename.c_str(), 32, NULL, 0);
    std::string baseName = std::filesystem::path(filename).stem().string();
    fonts[baseName] = fontTtf;
    SetTextureFilter(fonts[baseName].texture, TEXTURE_FILTER_POINT);
    // TODO: still no idea how to disable anti-aliasing
}

void AssetLoader::LoadShaderFile(const std::string& filename) {
    auto shader = std::make_shared<Shader>(LoadShader(0, filename.c_str()));
    std::string baseName = std::filesystem::path(filename).stem().string();
    shaders[baseName] = shader;
}

void AssetLoader::loadSettings(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        TraceLog(LOG_ERROR, "Failed to open settings file %s", filename.c_str());
        return;
    }
    file >> settings;
}

const nlohmann::json& AssetLoader::getSettings() {
    return settings;
}

void AssetLoader::loadSpriteData(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        TraceLog(LOG_ERROR, "Failed to open sprite data file %s", filename.c_str());
        return;
    }

    nlohmann::json newData;
    file >> newData;

    for (auto& [key, value] : newData.items()) {
        spriteData[key] = std::move(value); // overwrite if the data already exists
    }
}

const nlohmann::json& AssetLoader::getSpriteData() {
    return spriteData;
}

const std::vector<Texture2D>& AssetLoader::getTextures(const std::string& key) {
    return textureGroups[key]; // Returns and empty vector if key doesn't exist
}

const TileMap& AssetLoader::getTilemap(const std::string& key) {
    auto it = tileMaps.find(key);
    if (it != tileMaps.end()) {
        return *it->second;
    }
    throw std::out_of_range("TileMap key not found: " + key);
}

const Font& AssetLoader::getFont(const std::string& key) {
    return fonts.at(key);
}

const Shader& AssetLoader::getShader(const std::string& key) {
    auto it = shaders.find(key);
    if (it == shaders.end()) {
        throw std::runtime_error("Shader not found: " + key);
    }
    return *(it->second);
}

void AssetLoader::LoadMusicFile(const std::string& filename, const float volume, const std::string& key) {
    Music music = LoadMusicStream(filename.c_str());
    SetMusicVolume(music, volume);
    std::string id = key.empty() ? std::filesystem::path(filename).stem().string() : key;
    musicTracks[id] = music;
}

const Music& AssetLoader::getMusic(const std::string& key) {
    return musicTracks.at(key);
}

void AssetLoader::LoadSoundFile(const std::string& filename, const float volume, const std::string& key) {
    Sound sound = LoadSound(filename.c_str());
    SetSoundVolume(sound, volume);
    std::string id = key.empty() ? std::filesystem::path(filename).stem().string() : key;
    sounds[id] = sound;
}

const Sound& AssetLoader::getSound(const std::string& key) {
    return sounds.at(key);
}
