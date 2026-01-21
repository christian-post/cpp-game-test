#include "Overworld.h"
#include "Game.h"
#include "Utils.h"
#include <cmath>

Overworld::Overworld(Game& game, size_t roomsW, size_t roomsH, size_t numLevels, const std::string& name)
    : World(game, roomsW, roomsH, numLevels, name)
{
    isDungeon = false;
}

void Overworld::update(float deltaTime)
{
    blinkTimer += deltaTime;
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

void Overworld::renderMinimap(float hudY, float gameScreenWidth)
{
    // minimap dimensions and position
    const int minimapWidth = 48;
    const int minimapHeight = 24;
    const int mapX = static_cast<int>(gameScreenWidth) - minimapWidth - 6;
    const int mapY = static_cast<int>(hudY) + 4;

    // draw gray background
    DrawRectangle(mapX, mapY, minimapWidth, minimapHeight, GRAY);

    // get current room and player position
    Room* currentRoom = getCurrentRoom();
    if (!currentRoom)
        return;

    //const Vector2& playerPos = game.getPlayer()->position;
    const Vector2& playerPos = GetRectCenter(game.getPlayer()->rect);

    // calculate current room's grid position
    size_t currentCol = currentRoomIndex % roomsW;
    size_t currentRow = currentRoomIndex / roomsW;

    // calculate total world dimensions in pixels
    // assuming all rooms have same dimensions for simplicity
    size_t roomWidthPx = currentRoom->tilemap.width * currentRoom->tilemap.tileWidth;
    size_t roomHeightPx = currentRoom->tilemap.height * currentRoom->tilemap.tileHeight;
    size_t totalWorldWidth = roomsW * roomWidthPx;
    size_t totalWorldHeight = roomsH * roomHeightPx;

    // calculate absolute player position in world coordinates
    float absoluteX = (currentCol * roomWidthPx) + playerPos.x;
    float absoluteY = (currentRow * roomHeightPx) + playerPos.y;

    // map to minimap coordinates
    float u = absoluteX / totalWorldWidth;
    float v = absoluteY / totalWorldHeight;
    int dotX = mapX + static_cast<int>(u * minimapWidth);
    int dotY = mapY + static_cast<int>(v * minimapHeight);

    // draw white dot for player position
    if (fmod(blinkTimer, 1.0f) < 0.5f)
        //DrawPixel(dotX, dotY, WHITE);
        DrawRectangle(dotX - 1, dotY - 1, 2, 2, WHITE);
}

void Overworld::renderMapScreen(const MapRenderParams& params)
{
    // overwrite some params
    size_t border = params.border;
    size_t spacing = 1;

    // calculate minimap cell size based on max room dimensions
    int miniWidth = 0;
    int miniHeight = 0;
    for (size_t level = 0; level < levels.size(); level++)
    {
        for (size_t i = 0; i < roomsW * roomsH; i++)
        {
            Room* room = getRoomAt(level, i);
            if (!room)
                continue;
            miniWidth = std::max(miniWidth, (int)room->tilemap.width);
            miniHeight = std::max(miniHeight, (int)room->tilemap.height);
        }
    }

    const auto [cols, rows] = getSize();
    size_t currentRoomIdx = currentRoomIndex;
    const size_t cellWidth = (static_cast<size_t>(params.width) - 2 * border - (cols - 1) * spacing - params.offsetX) / cols;
    const size_t cellHeight = (static_cast<size_t>(params.height) - 2 * border - (rows - 1) * spacing - params.offsetY) / rows;

    for (size_t i = 0; i < cols * rows; ++i)
    {
        size_t col = i % cols;
        size_t row = i / cols;
        size_t cellX = params.offsetX + static_cast<size_t>(params.x) + border + col * (cellWidth + spacing);
        size_t cellY = params.offsetY + static_cast<size_t>(params.y) + border + row * (cellHeight + spacing);
        Color color = DARKGRAY;
        DrawRectangle(int(cellX), int(cellY), int(cellWidth), int(cellHeight), color);
        Room* room = getRoomAt(params.displayLevel, i);
        if (room && room->visited && params.displayLevel < mapTextures.size())
        {
            // calculate source rectangle in the atlas
            int roomX = (i % cols) * miniWidth;
            int roomY = (i / cols) * miniHeight;

            // get actual room dimensions for proper sampling
            int roomTilesX = room->tilemap.width;
            int roomTilesY = room->tilemap.height;

            Rectangle src = { (float)roomX, (float)roomY, (float)roomTilesX, (float)roomTilesY };
            Rectangle dst = { (float)cellX, (float)cellY, (float)cellWidth, (float)cellHeight };

            DrawTexturePro(mapTextures[params.displayLevel].texture, src, dst, { 0, 0 }, 0.0f, WHITE);

            // draw player as blinking sprite if they are in this room
            if (i == currentRoomIdx && currentLevel == params.displayLevel && params.showCursor)
            {
                const Vector2& pos = game.getPlayer()->position; // world position as (x,y)
                size_t roomW = room->tilemap.width * room->tilemap.tileWidth;
                size_t roomH = room->tilemap.height * room->tilemap.tileHeight;
                float u = pos.x / (float)roomW;
                float v = pos.y / (float)roomH;
                float px = cellX + u * cellWidth;
                float py = cellY + v * cellHeight;
                const auto& tex = game.loader.getTextures("knight_map_mini")[0];
                DrawTexture(tex, (int)px, (int)py, WHITE);
            }
        }
    }
}