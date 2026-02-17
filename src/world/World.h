#pragma once
#include "TileMap.h"
#include "json.hpp"
#include <raylib.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>
#include <string>

class Game;

struct MapRenderParams {
    float x, y, width, height;
    size_t displayLevel;
    bool showCursor;
    size_t border, spacing, offsetX, offsetY;
};

class World {
public:
    World(Game& game, size_t roomsW, size_t roomsH, size_t numLevels, const std::string& name);
    virtual ~World() = default;

    std::string name; // needed for save game data, has to match an entry in dungeons.json

    bool isDungeon = true;
    bool showMap = true; // some areas have no map (e.g., house interior)
    size_t currentLevel = 0;
    size_t currentRoomIndex = 0;
    size_t startingRoomIndex = 0;
    Vector2 startingPosition = { 0.0f, 0.0f };

    // room access
    Room* getRoomAt(size_t level, size_t index);
    Room* getCurrentRoom();
    const TileMap* getTileMap(size_t level, size_t index);
    const TileMap* getCurrentTileMap();

    // world structure
    std::pair<size_t, size_t> getSize() const;
    size_t getNumLevels() const;

    // room building
    void insertRoom(size_t level, size_t row, size_t col, Room&& room);
    void setStartingRoomIndex(size_t index);

    // room state
    void advanceRoomState(); 
    void advanceRoomState(size_t level, size_t index);

    // create textures for drawing the map in the menu
    // one texture per Level, from which the rooms get sampled when drawing
    std::vector<RenderTexture2D> mapTextures;
    void makeMapTextures();

    // child methods
    virtual void update(float deltaTime) {}
    virtual void generate(const nlohmann::json& worldData) = 0;
    virtual void renderMinimap(float hudY, float gameScreenWidth) = 0;
    virtual void renderMapScreen(const MapRenderParams& params) = 0;

protected:
    Game& game;
    size_t roomsW;
    size_t roomsH;
    bool playerHasBeenPlaced = false;
    std::vector<Level> levels;
};