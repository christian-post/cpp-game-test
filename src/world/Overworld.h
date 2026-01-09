#pragma once
#include "World.h"

class Overworld : public World {
public:
    Overworld(Game& game, size_t roomsW, size_t roomsH, size_t numLevels);

    void generate(const nlohmann::json& overworldData) override;
    void makeMinimapTextures() override;
    void renderMinimap(float hudY, float gameScreenWidth) override;
    void renderMapScreen(const MapRenderParams& params) override;
};