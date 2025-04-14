#include "AssetLoader.h"
#include <iostream>


AssetLoader::~AssetLoader() {
    for (auto& pair : textureGroups) {
        for (Texture2D& texture : pair.second) {
            UnloadTexture(texture);
        }
    }
    for (auto& pair : fonts) {
        UnloadFont(pair.second);
    }
    fonts.clear();
}

void AssetLoader::loadTexturesFromDirectory(const std::string& directory) {
    // Looks for png files in a given directory that match the pattern key_n.png
    // and automatically groups and loads them
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
                std::cerr << "ERROR: File not found: " << filename << std::endl;
            }
        }
    }
}

void AssetLoader::LoadTileset(const std::string& filename, int tileSize) {
    // Loads the tiles directly from an image, give the correct tile size
    std::vector<Texture2D> tiles;
    Image tilesetImg = LoadImage(filename.c_str());

    if (tilesetImg.width == 0 || tilesetImg.height == 0) {
        return; // Return if loading failed
    }

    int tilesX = tilesetImg.width / tileSize;
    int tilesY = tilesetImg.height / tileSize;

    for (int y = 0; y < tilesY; y++) {
        for (int x = 0; x < tilesX; x++) {
            Rectangle tileRect = { x * tileSize, y * tileSize, tileSize, tileSize };
            Image tileImg = ImageFromImage(tilesetImg, tileRect);
            tiles.push_back(LoadTextureFromImage(tileImg));
            UnloadImage(tileImg);
        }
    }

    UnloadImage(tilesetImg);
    std::string baseName = std::filesystem::path(filename).stem().string();
    textureGroups[baseName] = tiles;
}

void AssetLoader::LoadTilesetFromTiled(const std::string& filename) {
    // loads a Tiled tileset file (.json or .tsj)
    std::ifstream file(filename); // Open JSON file
    if (!file) {
        std::cerr << "Failed to open tileset file \"" << filename << "\"\n";
        return;
    }
    nlohmann::json j;
    file >> j; // construct a json-like object from the file data

    if (!j.contains("image") || !j.contains("tilewidth")) {
        std::cerr << "Invalid tileset format: missing 'image' or 'tilewidth'.\n";
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
        std::cerr << "Failed to open tilemap file \"" << filename << "\"\n";
        return;
    }
    nlohmann::json j;
    file >> j;

    TileMap tileMap(j);
    std::string baseName = std::filesystem::path(filename).stem().string();
    tileMaps[baseName] = std::make_unique<TileMap>(j);
}

void  AssetLoader::LoadFont(const std::string& filename) {
    Font fontTtf = LoadFontEx(filename.c_str(), 32, 0, NULL);
    std::string baseName = std::filesystem::path(filename).stem().string();
    fonts[baseName] = fontTtf;
    SetTextureFilter(fonts[baseName].texture, TEXTURE_FILTER_POINT);
    // TODO: still no idea how to disable anti-aliasing
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
