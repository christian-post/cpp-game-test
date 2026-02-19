#pragma once
#include "World.h"
#include "WorldGraph.h"
#include <unordered_set>

class Dungeon : public World {
public:
    Dungeon(Game& game, size_t roomsW, size_t roomsH, size_t numLevels, const std::string& name);

    // override virtual methods
    void generate(const nlohmann::json& dungeonData) override;
    //void makeMinimapTextures() override;
    void renderMinimap(float hudY, float gameScreenWidth) override;
    void renderMapScreen(const MapRenderParams& params) override;

    // dungeon-specific methods
    WorldGraph buildGraphFromDungeon(
        const std::string& start,
        const std::vector<std::tuple<std::string, std::string, std::vector<std::string>>>& edges,
        const std::unordered_set<std::string>& itemNodes
    );

private:
    static constexpr int   DUNGEON_MINIMAP_CELL_W = 6;   // cell width in pixels
    static constexpr int   DUNGEON_MINIMAP_CELL_H = 4;   // cell height in pixels
    static constexpr int   DUNGEON_MINIMAP_SPACING = 1;  // gap between cells
    static constexpr int   DUNGEON_MINIMAP_MARGIN_X = 6; // right-edge inset from screen edge
    static constexpr int   DUNGEON_MINIMAP_MARGIN_Y = 6; // top-edge inset below HUD
};