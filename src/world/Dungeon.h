#pragma once
#include "World.h"
#include "WorldGraph.h"
#include <unordered_set>

class Dungeon : public World {
public:
    Dungeon(Game& game, size_t roomsW, size_t roomsH, size_t numLevels);

    std::vector<std::vector<RenderTexture2D>> minimapTextures;

    // override virtual methods
    void generate(const nlohmann::json& dungeonData) override;
    void makeMinimapTextures() override;
    void renderMinimap(float hudY, float gameScreenWidth) override;
    void renderMapScreen(const MapRenderParams& params) override;

    // dungeon-specific methods
    WorldGraph buildGraphFromDungeon(
        const std::string& start,
        const std::vector<std::tuple<std::string, std::string, std::vector<std::string>>>& edges,
        const std::unordered_set<std::string>& itemNodes
    );
};