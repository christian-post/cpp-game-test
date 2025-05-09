#include "TileMap.h"


TileLayer::TileLayer(const nlohmann::json& layerJson) {
    name = layerJson["name"];
    width = layerJson["width"];
    height = layerJson["height"];

    // Get the flat 1D array from JSON
    std::vector<int> flatData = layerJson["data"].get<std::vector<int>>();

    // Resize the 2D vector to fit width x height
    data.resize(height, std::vector<int>(width));

    // Convert 1D array into 2D
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            data[y][x] = flatData[y * width + x];
        }
    }

    // Handle properties
    if (layerJson.contains("properties")) {
        for (const auto& prop : layerJson["properties"]) {
            properties[prop["name"]] = prop["value"].dump();
        }
    }
}

TileMap::TileMap(const nlohmann::json& jsonMap, std::string mapName) 
    : mapName(mapName)
{
    width = jsonMap["width"];
    height = jsonMap["height"];
    tileWidth = jsonMap["tilewidth"];
    tileHeight = jsonMap["tileheight"];

    std::string srcName = jsonMap["tilesets"][0]["source"];
    tilesetName = srcName.substr(0, srcName.size() - 4); // strip the ".tsj"

    if (jsonMap.contains("layers")) {
        for (const auto& layer : jsonMap["layers"]) {
            if (layer["type"] == "tilelayer") {
                layers.emplace_back(layer);
            }
            else if (layer["type"] == "objectgroup") {
                for (const auto& obj : layer["objects"]) {
                    objects.emplace_back(obj);
                }
            }
        }
    }
}

const TileLayer& TileMap::getLayer(size_t index) const {
    if (index >= layers.size()) throw std::out_of_range("Layer index out of bounds");
    return layers[index];
}

const std::vector<TileObject>& TileMap::getObjects() const {
    return objects;
}
