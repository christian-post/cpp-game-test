#pragma once
#include "Scene.h"
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
    uint32_t height = 0;
    uint32_t width = 0;
    float speed = 0.0f;
    float slideDuration = 0.5f;
    float blinkTimer = 0.0f;
    float blinkSpeed = 0.5f;
    bool cursorOn = false;
    std::array<Vector2, 4> offsets; // used for minimap formatting
};
