#include "Overworld.h"
#include "Game.h"

Overworld::Overworld(Game& game, size_t roomsW, size_t roomsH, size_t numLevels, std::string& name)
    : World(game, roomsW, roomsH, numLevels, name)
{
    isDungeon = false;
    //minimapTextures.resize(numLevels);
}

void Overworld::generate(const nlohmann::json& overworldData)
{
    size_t startingLevel = overworldData["starting_level"];
    std::vector<int> startCoords = overworldData["starting_room"];
    size_t startIndex = startCoords[0] * roomsW + startCoords[1];
    setStartingRoomIndex(startIndex);
    currentLevel = startingLevel;

    // TODO edges and item requirements?

    // process each level
    for (size_t levelIndex = 0; levelIndex < overworldData["levels"].size(); ++levelIndex)
    {
        const auto& levelData = overworldData["levels"][levelIndex];

        // insert all "rooms" for this level
        for (const auto& roomData : levelData["rooms"])
        {
            size_t row = roomData["row"];
            size_t column = roomData["column"];
            std::string tilemapName = roomData["tilemap"];
            insertRoom(levelIndex, row, column, Room{ game.loader.getTilemap(tilemapName), 0b1111 }); // for now, every map chunk has all connections
        }
    }

    // save the player starting position
    if (overworldData.contains("starting_position"))
    {
        startingPosition.x = overworldData["starting_position"]["x"];
        startingPosition.y = overworldData["starting_position"]["y"];
    }
    else
    {
        // if not specified, use the center of the first room
        TileMap& tm = levels[startingLevel].getRoomAt(startingRoomIndex)->tilemap;
        startingPosition.x = (tm.width * tm.tileWidth) * 0.5f;
        startingPosition.y = (tm.height * tm.tileHeight) * 0.5f;
    }
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