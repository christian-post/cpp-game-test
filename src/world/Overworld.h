#pragma once
#include "World.h"

class Overworld : public World {
public:
    Overworld(Game& game, size_t roomsW, size_t roomsH, size_t numLevels, const std::string& name);

    void update(float deltaTime) override;
    void generate(const nlohmann::json& overworldData) override;
    void renderMinimap(float hudY, float gameScreenWidth) override;
    void renderMapScreen(const MapRenderParams& params) override;

private:
    float blinkTimer = 0.0f;

    static constexpr int   OW_MINIMAP_WIDTH = 48;
    static constexpr int   OW_MINIMAP_HEIGHT = 24;
    static constexpr int   OW_MINIMAP_MARGIN_X = 6; // right-edge inset
    static constexpr int   OW_MINIMAP_MARGIN_Y = 4; // top-edge inset below HUD

    static constexpr int   OW_MINIMAP_DOT_SIZE = 2; // Player dot radius (drawn as a 2x2 rectangle)

    // Blink period and on-fraction for the player dot
    static constexpr float OW_MINIMAP_BLINK_PERIOD = 1.0f;
    static constexpr float OW_MINIMAP_BLINK_ON_FRAC = 0.5f;
};