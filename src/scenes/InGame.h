#pragma once
#include "Scene.h"
#include "Sprite.h"
#include "TileMap.h"
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

    void drawTilemapChunks(int layerIndex);

    std::unordered_map<std::string, std::shared_ptr<Sprite>> spriteMap; // keep named references to certain sprites
    std::shared_ptr<Sprite> player;  // keep a player variable for direct frequent access
    Sprite* getSprite(const std::string& name);
    std::optional<std::string> currentWeapon = std::nullopt;
    // functions that handle changing rooms
    const TileMap* tileMap;
    size_t tileSize = 0;
    void loadTilemap();
    // behaviors
    void addBehaviorsToSprite(std::shared_ptr<Sprite> sprite, const std::vector<std::string>& behaviors, const nlohmann::json& behaviorData);

    Camera2D camera = {};
    CameraShake cameraShake;

    // light effects
    Light lights[1] = {
        {{0.0f, 0.0f}, 12.0f},
    };
    int lightCount = 1;

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
