#pragma once
#include "Scene.h"
#include "Sprite.h"
#include "TileMap.h"
#include "Dungeon.h"
#include "Utils.h"
#include "CircleOverlay.h"
#include <memory>
#include "json.hpp"

class InGame : public Scene {
public:
    InGame(Game& game, const std::string& name) : Scene(game, name), tileMap(nullptr), worldHeight(0), worldWidth(0) {}
    void startup() override;
    void update(float deltaTime) override;
    void draw() override;
    void end() override;

    void processTileObject(const TileObject& obj, uint8_t currentState, std::unordered_map<uint32_t, ObjectState>& objectStates, const nlohmann::json& spriteData); // helper function that turns Tiled data into sprites etc
    void loadTilemap(); // function that handles room transitions
    void drawTilemapChunks(int layerIndex);
    Sprite* getSprite(const std::string& name);
    void addBehaviorsToSprite(std::shared_ptr<Sprite> sprite, const std::vector<std::string>& behaviors, const nlohmann::json& behaviorData);
    // methods for collision handling
    void resolveAxisX(const std::shared_ptr<Sprite>& sprite, const Rectangle& obstacle);
    void resolveAxisY(const std::shared_ptr<Sprite>& sprite, const Rectangle& obstacle);

    const TileMap* tileMap;
    size_t tileSize = 0; // value is read from Tiled data 
    Camera2D camera = {};
    CameraShake cameraShake;
    std::unordered_map<std::string, std::shared_ptr<Sprite>> spriteMap; // keep named references to certain sprites
    std::shared_ptr<Sprite> player;  // keep a player variable for direct frequent access
    std::optional<std::string> currentWeapon = std::nullopt;
    // light effects
    // TODO: these need to be dynamic and shouldn't be hard-coded here.
    // > use a vector instead
    Light lights[MAX_LIGHTS] = {
        {{0.0f, 0.0f}, 12.0f},
        {{0.0f, 0.0f}, 12.0f},
        {{0.0f, 0.0f}, 12.0f},
        {{0.0f, 0.0f}, 12.0f},
        {{0.0f, 0.0f}, 12.0f},
        {{0.0f, 0.0f}, 12.0f},
        {{0.0f, 0.0f}, 12.0f},
        {{0.0f, 0.0f}, 12.0f},
        {{0.0f, 0.0f}, 12.0f},
        {{0.0f, 0.0f}, 12.0f}
    };
    static const int lightCount = MAX_LIGHTS;

private:
    size_t worldWidth;
    size_t worldHeight;
    size_t tileChunkSize = 256; // limit the size of the textures that hold the tilemap layers
    size_t numChunksX = 0;
    size_t numChunksY = 0;
    std::vector<std::vector<RenderTexture2D>> tilemapChunks; // stores chunks of eachs of the layers of a map
};
