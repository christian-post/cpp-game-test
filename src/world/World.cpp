#include "World.h"
#include "Game.h"
#include "TilemapRenderer.h"
#include <filesystem>


World::World(Game& game, size_t roomsW, size_t roomsH, size_t numLevels, const std::string& name)
    : game{ game }, roomsW{ roomsW }, roomsH{ roomsH }, name{ name }
{
    levels.reserve(numLevels);
    for (size_t i = 0; i < numLevels; ++i)
    {
        levels.emplace_back(roomsW, roomsH);
    }
}

Room* World::getRoomAt(size_t level, size_t index)
{
    if (level >= levels.size()) // TODO is this check sensible here? 
        return nullptr;

    return levels[level].getRoomAt(index);
}

Room* World::getCurrentRoom()
{
    return getRoomAt(currentLevel, currentRoomIndex);
}

const TileMap* World::getTileMap(size_t level, size_t index)
{
    Room* room = getRoomAt(level, index);
    if (!room)
    {
        TraceLog(LOG_ERROR, "getTileMap(): No room on level %zu at index %zu", level, index);
        return nullptr;
    }
    return &room->tilemap;
}

const TileMap* World::getCurrentTileMap()
{
    Room* room = getCurrentRoom();
    if (!room)
        return nullptr;

    return &room->tilemap;
}

std::pair<size_t, size_t> World::getSize() const
{
    return { roomsW, roomsH };
}

size_t World::getNumLevels() const
{
    return levels.size();
}

void World::insertRoom(size_t level, size_t row, size_t col, Room&& room)
{
    size_t index = row * roomsW + col;

    // resize levels vector if necessary
    if (level >= levels.size())
    {
        while (level >= levels.size())
        {
            levels.emplace_back(roomsW, roomsH);
        }
    }

    if (!getRoomAt(level, index))
    {
        levels[level].insertRoom(index, std::move(room));
    }
    else
    {
        TraceLog(LOG_WARNING, "insertRoom(): A room already exists on level %zu at index %zu", level, index);
    }
}

void World::setStartingRoomIndex(size_t index)
{
    startingRoomIndex = index;
    if (!playerHasBeenPlaced)
    {
        playerHasBeenPlaced = true;
        currentRoomIndex = index;
    }
}

void World::advanceRoomState()
{
    advanceRoomState(currentLevel, currentRoomIndex);
}

void World::advanceRoomState(size_t level, size_t index)
{
    Room* room = getRoomAt(level, index);
    if (!room)
    {
        TraceLog(LOG_ERROR, "advanceRoomState(): No room on level %zu at index %zu", level, index);
        return;
    }
    // make sure the room state is at least 1
    (room->state <<= 1) || (room->state = 1);
    TraceLog(LOG_INFO, "Room state of %s is now %d", room->tilemap.getName().c_str(), room->state);
}

void World::makeMapTextures()
{
    // find max room dimensions across all levels
    // smaller rooms get padded
    int maxTilesX = 0;
    int maxTilesY = 0;
    for (size_t level = 0; level < levels.size(); level++)
    {
        for (size_t i = 0; i < roomsW * roomsH; i++)
        {
            Room* room = getRoomAt(level, i);
            if (!room)
                continue;
            maxTilesX = std::max(maxTilesX, (int)room->tilemap.width);
            maxTilesY = std::max(maxTilesY, (int)room->tilemap.height);
        }
    }

    const int miniWidth = maxTilesX;
    const int miniHeight = maxTilesY;

    mapTextures.resize(levels.size());

    for (size_t level = 0; level < levels.size(); level++)
    {
        // create one large texture for all rooms in this level
        int atlasWidth = roomsW * miniWidth;
        int atlasHeight = roomsH * miniHeight;
        mapTextures[level] = LoadRenderTexture(atlasWidth, atlasHeight);

        BeginTextureMode(mapTextures[level]);
        ClearBackground(BLANK);
        EndTextureMode();

        for (size_t i = 0; i < roomsW * roomsH; i++)
        {
            Room* room = getRoomAt(level, i);
            if (!room)
                continue;

            auto& tileMap = room->tilemap;

            // get all tileset information
            const auto& tilesetInfos = tileMap.getTilesetNames();

            std::vector<TilesetData> tilesetCache;
            for (const auto& info : tilesetInfos)
            {
                const Tileset& tileset = game.loader.getTileset(info.first);
                TilesetData data;
                data.name = info.first;
                data.tileset = &tileset;

                std::string baseImageName = std::filesystem::path(tileset.image).stem().string();
                std::string minimapTextureName = baseImageName + "_mini";

                auto& textures = game.loader.getTextures(minimapTextureName);
                if (!textures.empty())
                    data.mapTexture = &textures[0];
                else
                    data.mapTexture = nullptr;

                data.tilesPerRow = tileset.columns;
                data.firstGid = info.second;
                tilesetCache.push_back(data);
            }

            size_t tilesX = tileMap.width;
            size_t tilesY = tileMap.height;

            // calculate position in atlas
            int roomX = (i % roomsW) * miniWidth;
            int roomY = (i / roomsW) * miniHeight;

            BeginTextureMode(mapTextures[level]);

            // render minimap directly from preloaded minimap textures
            for (size_t layerIndex = 0; layerIndex < tileMap.layers.size(); ++layerIndex)
            {
                const auto& layer = tileMap.getLayer(layerIndex);
                if (!layer.visible)
                    continue;

                for (size_t y = 0; y < tileMap.height; ++y)
                {
                    for (size_t x = 0; x < tileMap.width; ++x)
                    {
                        int tileId = layer.data[y][x];
                        if (tileId == 0)
                            continue;

                        // find which tileset this tile belongs to
                        const TilesetData* tilesetData = nullptr;
                        int tileIndex = 0;

                        for (size_t i = 0; i < tilesetCache.size(); i++)
                        {
                            int currentFirstGid = tilesetCache[i].firstGid;
                            int nextFirstGid = (i + 1 < tilesetCache.size()) ? tilesetCache[i + 1].firstGid : INT_MAX;

                            if (tileId >= currentFirstGid && tileId < nextFirstGid)
                            {
                                tilesetData = &tilesetCache[i];
                                tileIndex = tileId - currentFirstGid;
                                break;
                            }
                        }

                        if (!tilesetData || !tilesetData->mapTexture)
                            continue;

                        // sample the pixel color from the minimap texture
                        int pixelX = tileIndex % tilesetData->tilesPerRow;
                        int pixelY = tileIndex / tilesetData->tilesPerRow;
                        Rectangle src = { (float)pixelX, (float)pixelY, 1.0f, 1.0f };

                        // draw the tile as a single colored rectangle
                        float sx = roomX + static_cast<float>(x);
                        float sy = roomY + static_cast<float>(y);
                        Rectangle dst = { sx, sy, 1.0f, 1.0f };

                        DrawTexturePro(*tilesetData->mapTexture, src, dst, { 0, 0 }, 0.0f, WHITE);
                    }
                }
            }
            EndTextureMode();
        }

        // flip the texture vertically to correct orientation
        Image img = LoadImageFromTexture(mapTextures[level].texture);
        ImageFlipVertical(&img);
        Texture2D flippedTex = LoadTextureFromImage(img);
        UnloadRenderTexture(mapTextures[level]);
        mapTextures[level].texture = flippedTex;

        // debug: save atlas to file
        std::filesystem::path debugPath = "debug";
        std::filesystem::create_directories(debugPath);
        std::string filename = debugPath.string() + "/map_atlas_level_" + std::to_string(level) + "_" + name + ".png";
        ExportImage(img, filename.c_str());
        UnloadImage(img);
    }
}
