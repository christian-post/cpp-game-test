#pragma once
#include "Scene.h"
#include <cstdint>
#include <array>

class MapUI : public Scene {
    // scene responsible for drawing the dungeon map
public:
    MapUI(Game& game, const std::string& name);
    void startup() override;
    void update(float deltaTime) override;
    void draw() override;
    void end() override;

private:
    float x = 0.0f;
    float topY = 0.0f; // gets adjusted to the HUD height
    float y = 0.0f;
    // dimensions of the whole menu
    uint32_t height = 0;
    uint32_t width = 0;
    // map formatting
    const size_t spacing = 4;
    const size_t border = 24;
    // this will offset the map from the center
    const size_t offsetX = 48;
    const size_t offsetY = 0;
    // animations
    float speed = 0.0f;
    float slideDuration = 0.5f;
    float blinkTimer = 0.0f;
    float blinkSpeed = 0.5f;
    bool cursorOn = false;
    size_t currentLevel = 0;  // which level of the Dungeon to display
    std::array<Vector2, 4> offsets; // used for minimap formatting
};
