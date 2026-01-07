#include "Overworld.h"
#include "Game.h"

Overworld::Overworld(Game& game, size_t roomsW, size_t roomsH)
    : World(game, roomsW, roomsH, 1) // overworld is single-level
{
}

void Overworld::generate(const nlohmann::json& overworldData)
{
    // TODO: implement overworld generation
    // for now, stub implementation
}

void Overworld::makeMinimapTextures()
{
    // TODO: implement overworld minimap texture generation
    // different from dungeon (interconnected style)
}

void Overworld::renderMinimap(float hudY, float gameScreenWidth)
{
    // TODO: implement overworld HUD minimap rendering
    // different layout than dungeon grid
}

void Overworld::renderMapScreen(const MapRenderParams& params)
{
    // TODO: implement overworld map screen rendering
    // scrollable interconnected map instead of grid
}