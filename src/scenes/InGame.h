#pragma once

#include "Scene.h"
#include "Sprite.h"
#include "TileMap.h"
#include <memory>
#include "json.hpp"

class InGame : public Scene {
public:
    InGame(Game& game, const std::string& name) : Scene(game, name), tileMap(nullptr), worldHeight(0), worldWidth(0) {}
    void startup() override;
    void update(float dt) override;
    void draw() override;
    void end() override;

    void drawTilemapChunks(int layerIndex);

    std::unordered_map<std::string, std::shared_ptr<Sprite>> spriteMap; // keep named references to certain sprites
    std::shared_ptr<Sprite> player;  // keep a player variable for direct frequent access
    Sprite* getSprite(const std::string& name);
    std::optional<std::string> currentWeapon = std::nullopt;
    const TileMap* tileMap;
    size_t tileSize = 0;
    void loadTilemap(const std::string& name);
    void addBehaviorsToSprite(std::shared_ptr<Sprite> sprite, const std::vector<std::string>& behaviors, const nlohmann::json& behaviorData);
    std::unordered_map<std::string, uint8_t> roomStates; // store map progression as bitmasks
    void advanceRoomState(const std::string& name);

    // TODO: debug stuff 
    // lets you cycle the rooms faster
    std::array<std::pair<std::string, Vector2>, 3> roomDebug = { {
        { "test_dungeon2", { 9.0f, 16.0f } },
        { "test_dungeon", { 17.0f, 20.0f } },
        { "test_fields", { 18.0f, 35.0f } }
    }};
    uint32_t currentRoomIndex = 0;

    Camera2D camera = {};

    // collision
    void resolveAxisX(const std::shared_ptr<Sprite>& sprite, const Rectangle& obstacle);
    void resolveAxisY(const std::shared_ptr<Sprite>& sprite, const Rectangle& obstacle);

private:
    size_t worldWidth;
    size_t worldHeight;
    static const size_t tileChunkSize = 256; // limit the size of the textures that hold the tilemap layers
    size_t numChunksX = 0;
    size_t numChunksY = 0;
    std::vector<std::vector<RenderTexture2D>> tilemapChunks; // stores chunks of eachs of the layers of a map
};
