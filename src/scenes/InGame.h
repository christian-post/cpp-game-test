#pragma once
#include "Scene.h"
#include "Sprite.h"
#include "TileMap.h"
#include "Dungeon.h"
#include "Utils.h"
#include "CircleOverlay.h"
#include "TilemapRenderer.h"
#include <memory>
#include "json.hpp"

class InGame : public Scene {
public:
    InGame(Game& game, const std::string& name);
    void startup() override;
    void update(float deltaTime) override;
    void draw() override;
    void end() override;

    void loadTilemap(); // function that handles room transitions
    Sprite* getSprite(const std::string& name);

    const TileMap* tileMap;
    size_t tileSize = 0; // value is read from Tiled data 
    Camera2D camera = {};
    CameraShake cameraShake;
    std::shared_ptr<Sprite> player;  // keep a player variable for direct frequent access
    std::array<std::optional<std::string>, 2> currentWeapon = { std::nullopt, std::nullopt };
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

    bool lampIsOn = false; // dark rooms become lit up when the player equips the lamp

private:
    void spawnWeapon(size_t index);
    TilemapRenderer tilemapRenderer;
};
