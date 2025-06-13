#pragma once
#include "Scene.h"


enum inventoryState {
    NONE,
    OPENING,
    OPENED,
    CLOSING,
    SLIDING_LEFT,
    SLIDING_RIGHT
};

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
    float slideDuration = 1.0f;
    inventoryState state = NONE;
};
