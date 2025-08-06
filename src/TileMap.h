#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <filesystem>
#include "json.hpp"

struct Tileset {
    // used to store the data from *.tsj files
    std::string name, image;
    uint32_t imagewidth, imageheight, tilecount, tileheight, tilewidth, columns;

    Tileset() = default;

    Tileset(const nlohmann::json& objJson) :
        name(objJson["name"]),
        image(std::filesystem::path(objJson["image"].get<std::string>()).filename().string()), // basename, used as key for the texture
        imagewidth(objJson["imagewidth"]),
        imageheight(objJson["imageheight"]),
        tilecount(objJson["tilecount"]),
        tileheight(objJson["tileheight"]),
        tilewidth(objJson["tilewidth"]),
        columns(objJson["columns"]) {
    }
};

struct TileLayer {
    std::string name;
    bool visible;
    int width, height;
    std::vector<std::vector<int>> data;
    std::unordered_map<std::string, std::string> properties;  // TODO parse properties as JSON
    TileLayer(const nlohmann::json& layerJson);
};

struct TileObject {
    // Tiled map objects (sprites etc.)
    std::string type, name;
    float x, y, width, height;
    bool visible;
    uint32_t id;
    nlohmann::json properties;

    TileObject(const nlohmann::json& objJson) :
        type(objJson["type"]),
        visible(objJson["visible"]),
        name(objJson["name"]),
        x(objJson["x"]),
        y(objJson["y"]),
        width(objJson["width"]),
        height(objJson["height"]),
        id(objJson["id"])
    {
        if (objJson.contains("properties") && objJson["properties"].is_array()) {
            for (const auto& p : objJson["properties"]) {
                if (p.contains("name") && p.contains("value")) {
                    properties[p["name"]] = p["value"];
                }
            }
        }
        else {
            properties = nlohmann::json::object();
        }
    }
};

class TileMap {
public:
    TileMap(const nlohmann::json& jsonMap, std::string mapName);
    const TileLayer& getLayer(size_t index) const;
    const std::vector<TileObject>& getObjects() const;
    const std::string& getName() const { return mapName; }
    const std::string& getTilesetName() const { return tilesetName; }
    const std::string& getMusicKey() const { return music; }

    size_t width, height, tileWidth, tileHeight;
    std::vector<TileLayer> layers;
    std::vector<TileObject> objects;

private:
    std::string mapName;
    std::string tilesetName;
    std::string music;
};
