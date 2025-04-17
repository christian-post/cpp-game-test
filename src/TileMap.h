#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include "json.hpp"

struct TileLayer {
    std::string name;
    int width, height;
    std::vector<std::vector<int>> data;
    std::unordered_map<std::string, std::string> properties;  // TODO parse properties as JSON

    TileLayer(const nlohmann::json& layerJson);
};

struct TileObject {
    std::string type, name;
    float x, y, width, height;
    nlohmann::json properties;

    TileObject(const nlohmann::json& objJson) :
        type(objJson["type"]),
        name(objJson["name"]),
        x(objJson["x"]),
        y(objJson["y"]),
        width(objJson["width"]),
        height(objJson["height"])
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
    int width, height, tileWidth, tileHeight;
    std::vector<TileLayer> layers;
    std::vector<TileObject> objects;

    TileMap(const nlohmann::json& jsonMap);
    const TileLayer& getLayer(size_t index) const;
    const std::vector<TileObject>& getObjects() const;
};
