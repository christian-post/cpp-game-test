#pragma once
#include "raylib.h"
#include "json.hpp"
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <utility>
#include <optional>

class Game;

struct ObjectState {
    bool isOpened = false;
    bool isDefeated = false;
    size_t dialogIndex = 0;
    std::string itemName;
    size_t itemAmount = 0;
};

inline void to_json(nlohmann::json& jsonOutput, const ObjectState& state) {
    jsonOutput = {
        { "isOpened", state.isOpened },
        { "isDefeated", state.isDefeated },
        { "dialogIndex", state.dialogIndex },
        { "itemName", state.itemName },
        { "itemAmount", state.itemAmount },
    };
}

inline void from_json(const nlohmann::json& jsonInput, ObjectState& state) {
    jsonInput.at("isOpened").get_to(state.isOpened);
    jsonInput.at("isDefeated").get_to(state.isDefeated);
    jsonInput.at("dialogIndex").get_to(state.dialogIndex);
    jsonInput.at("itemName").get_to(state.itemName);
    jsonInput.at("itemAmount").get_to(state.itemAmount);
}

struct Tileset {
    std::string name, image;
    uint32_t imagewidth, imageheight, tilecount, tileheight, tilewidth, columns;

    Tileset() = default;

    Tileset(const nlohmann::json& objJson) :
        name(objJson["name"]),
        image(std::filesystem::path(objJson["image"].get<std::string>()).filename().string()),
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
    std::unordered_map<std::string, std::string> properties;
    TileLayer(const nlohmann::json& layerJson);
};

struct TileObject {
    std::string type, name;
    float x, y, width, height;
    bool visible;
    uint32_t id;
    nlohmann::json properties;

    TileObject() = default;

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
        if (objJson.contains("properties") && objJson["properties"].is_array())
        {
            for (const auto& p : objJson["properties"])
            {
                if (p.contains("name") && p.contains("value"))
                {
                    properties[p["name"]] = p["value"];
                }
            }
        }
        else
        {
            properties = nlohmann::json::object();
        }
    }
};

struct CollisionObject {
    int layer = 0;
    float x, y, width, height;
    Rectangle getRect() { return Rectangle{ x, y, width, height }; }
};

class TileMap {
public:
    TileMap(const nlohmann::json& jsonMap, std::string mapName);
    const TileLayer& getLayer(size_t index) const;
    const std::vector<TileObject>& getObjects() const { return objects; }
    std::vector<TileObject>& getObjects() { return objects; }
    const std::string& getName() const { return mapName; }
    const std::vector<std::pair<std::string, int>>& getTilesetNames() const { return tilesetNames; }
    const std::string& getRoomID() const { return roomID; }
    const std::string& getMusicKey() const { return music; }
    bool isDark() const { return dark; }

    size_t width; // width of the map in tiles
    size_t height; // height of the map in tiles
    size_t tileWidth; // width of one tile in pixels
    size_t tileHeight; // height of one tile in pixels
    std::vector<TileLayer> layers;
    mutable std::vector<TileObject> dynamicObjects;

private:
    std::string mapName;
    std::vector<std::pair<std::string, int>> tilesetNames;
    std::string music;
    bool dark = false;
    std::vector<TileObject> objects;
    std::string roomID;
};

class Room {
public:
    uint8_t doors;
    TileMap tilemap;
    uint8_t state = 1;
    bool visited = false;
    bool dark = false;
    std::unordered_map<uint32_t, ObjectState> objectStates;

    Room(TileMap tilemap, uint8_t doors = 0b0000)
        : doors(doors), tilemap(std::move(tilemap)), dark(this->tilemap.isDark())
    {
    }
};

class Level {
public:
    Level(size_t roomsW, size_t roomsH);
    std::vector<std::optional<Room>>& getRooms();
    Room* getRoomAt(size_t index);
    void insertRoom(size_t index, Room&& room);

private:
    size_t roomsW;
    size_t roomsH;
    std::vector<std::optional<Room>> rooms;
};

void processTileObject(Game& game, const TileObject& obj, uint8_t currentState, std::unordered_map<uint32_t, ObjectState>& objectStates, const nlohmann::json& spriteData);