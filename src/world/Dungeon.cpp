#include "Dungeon.h"
#include "Game.h"
#include "WorldGraph.h"
#include "TilemapRenderer.h"

Dungeon::Dungeon(Game& game, size_t roomsW, size_t roomsH, size_t numLevels)
    : World(game, roomsW, roomsH, numLevels)
{
    minimapTextures.resize(numLevels);
}

void Dungeon::generate(const nlohmann::json& dungeonData)
{
    // set starting room from coordinates
    size_t startingLevel = dungeonData["starting_level"];
    std::vector<int> startCoords = dungeonData["starting_room"];
    size_t startIndex = startCoords[0] * roomsW + startCoords[1];
    setStartingRoomIndex(startIndex);
    currentLevel = startingLevel;

    // get item pool for this dungeon
    std::vector<std::string> itemPool = dungeonData["item_pool"];

    // collect edges and item nodes across all levels
    std::vector<std::tuple<std::string, std::string, std::vector<std::string>>> edges;
    std::unordered_set<std::string> itemNodes;

    // process each level
    for (size_t levelIndex = 0; levelIndex < dungeonData["levels"].size(); ++levelIndex)
    {
        const auto& levelData = dungeonData["levels"][levelIndex];

        // insert all rooms for this level
        for (const auto& roomData : levelData["rooms"])
        {
            size_t row = roomData["row"];
            size_t column = roomData["column"];
            std::string tilemapName = roomData["tilemap"];
            std::string doorsStr = roomData["doors"];
            uint8_t doors = static_cast<uint8_t>(std::stoi(doorsStr, nullptr, 2));

            insertRoom(levelIndex, row, column, Room{ game.loader.getTilemap(tilemapName), doors });
        }

        // collect edges with item requirements from this level
        for (const auto& edge : levelData["edges"])
        {
            std::vector<int> fromCoords = edge["from"];
            std::vector<int> toCoords = edge["to"];
            std::vector<std::string> requiredItems = edge["required_items"];
            std::string fromId = std::to_string(levelIndex) + "_" + std::to_string(fromCoords[0]) + "_" + std::to_string(fromCoords[1]);
            std::string toId = std::to_string(levelIndex) + "_" + std::to_string(toCoords[0]) + "_" + std::to_string(toCoords[1]);

            edges.emplace_back(fromId, toId, requiredItems);
        }

        // collect item nodes from this level
        for (const auto& coords : levelData["item_nodes"])
        {
            std::vector<int> coordPair = coords;
            std::string roomId = std::to_string(levelIndex) + "_" + std::to_string(coordPair[0]) + "_" + std::to_string(coordPair[1]);
            itemNodes.insert(roomId);
        }
    }

    // add level connections (bidirectional)
    for (const auto& conn : dungeonData["level_connections"])
    {
        size_t fromLevel = conn["from"];
        size_t toLevel = conn["to"];
        std::vector<int> roomCoords = conn["room"];

        std::string fromId = std::to_string(fromLevel) + "_" + std::to_string(roomCoords[0]) + "_" + std::to_string(roomCoords[1]);
        std::string toId = std::to_string(toLevel) + "_" + std::to_string(roomCoords[0]) + "_" + std::to_string(roomCoords[1]);

        edges.emplace_back(fromId, toId, std::vector<std::string>{});
        edges.emplace_back(toId, fromId, std::vector<std::string>{});
    }

    // build starting room identifier
    std::string startingRoomId = std::to_string(startingLevel) + "_" + std::to_string(startCoords[0]) + "_" + std::to_string(startCoords[1]);

    // build and fill world graph
    WorldGraph G = buildGraphFromDungeon(startingRoomId, edges, itemNodes);
    int attempts = 0;
    const int max_attempts = 100;

    do
    {
        G.initializeItems(itemPool);
        G.forwardFill();
        attempts++;
    } while (!G.itemPool.empty() && attempts < max_attempts);

    if (!G.itemPool.empty())
    {
        G.logDebug();

        TraceLog(LOG_INFO, "Dungeon Size");
        for (size_t lvl = 0; lvl < levels.size(); lvl++)
        {
            TraceLog(LOG_INFO, "Level %zu: %zu rooms.", lvl, levels[lvl].getRooms().size());
        }

        G.testReachability();

        size_t numItemsLeft = G.itemPool.size();
        TraceLog(LOG_INFO, "There are still %zu items in the pool.", numItemsLeft);

        throw std::runtime_error("Failed to place all items after " + std::to_string(max_attempts) + " attempts");
    }

    // place items into Tiled data
    for (const auto& [roomId, node] : G.nodes)
    {
        if (node->item.has_value())
        {
            TraceLog(LOG_INFO, "[%s] placed item: %s", roomId.c_str(), node->item->c_str());

            // parse level from roomId string "level_row_col"
            size_t firstUnderscore = roomId.find('_');
            size_t level = std::stoi(roomId.substr(0, firstUnderscore));

            Room* room = getRoomAt(level, node->id);
            if (!room)
                continue;

            TileMap& roomData = room->tilemap;
            std::vector<TileObject>& objects = roomData.getObjects();
            for (auto& obj : objects)
            {
                if (obj.name == "chest")
                {
                    obj.properties["item"] = node->item.value();
                    obj.properties["amount"] = 1;

                    // overwrite the object state
                    room->objectStates[obj.id].itemName = node->item.value();
                    room->objectStates[obj.id].itemAmount = 1;

                    break;
                }
            }
        }
    }
    // print Dungeon graph to the console
    G.logDebug();
}

void Dungeon::makeMinimapTextures()
{
    constexpr int miniWidth = 36;
    constexpr int miniHeight = 24;
    constexpr int tileSize = 16;

    minimapTextures.resize(levels.size());

    for (size_t level = 0; level < levels.size(); level++)
    {
        for (size_t i = 0; i < roomsW * roomsH; i++)
        {
            Room* room = getRoomAt(level, i);
            if (!room)
            {
                RenderTexture2D mini = LoadRenderTexture(miniWidth, miniHeight);
                BeginTextureMode(mini);
                ClearBackground(BLANK);
                EndTextureMode();
                minimapTextures[level].push_back(mini);
                continue;
            }
            auto& tileMap = room->tilemap;

            // Get all tileset information
            const auto& tilesetInfos = tileMap.getTilesetNames();

            // Use the existing TilesetData struct - no redefinition!
            std::vector<TilesetData> tilesetCache;
            for (const auto& info : tilesetInfos)
            {
                const Tileset& tileset = game.loader.getTileset(info.first);
                TilesetData data;
                data.name = info.first;
                data.tileset = &tileset;
                data.texture = &game.loader.getTextures(tileset.name)[0];
                data.tilesPerRow = tileset.columns;
                data.firstGid = info.second;
                tilesetCache.push_back(data);
            }

            size_t tilemapWidth = tileMap.width * tileSize;
            size_t tilemapHeight = tileMap.height * tileSize;
            size_t tilesX = tileMap.width;
            size_t tilesY = tileMap.height;

            // render at full size first
            RenderTexture2D normal = LoadRenderTexture(static_cast<int>(tilemapWidth), static_cast<int>(tilemapHeight));
            RenderTexture2D mini = LoadRenderTexture(miniWidth, miniHeight);

            BeginTextureMode(normal);
            ClearBackground(BLANK);
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

                        if (!tilesetData)
                            continue;

                        // sample the source rect from the tileset
                        float tileX = static_cast<float>(tileIndex % tilesetData->tilesPerRow) * tileSize;
                        float tileY = (static_cast<float>(tileIndex) / static_cast<float>(tilesetData->tilesPerRow)) * tileSize;
                        Rectangle src = { tileX, tileY, (float)tileSize, (float)tileSize };

                        float px = static_cast<float>(x) * static_cast<float>(tileSize);
                        float py = static_cast<float>(y) * static_cast<float>(tileSize);
                        Rectangle dst = { px, py, tileSize, tileSize };

                        DrawTexturePro(*tilesetData->texture, src, dst, { 0, 0 }, 0.0f, WHITE);
                    }
                }
            }
            EndTextureMode();

            // downsample to minimap size using mode filter
            Image fullImg = LoadImageFromTexture(normal.texture);
            Color* pixels = LoadImageColors(fullImg);

            BeginTextureMode(mini);
            ClearBackground(BLANK);
            for (size_t ty = 0; ty < tilesY; ++ty)
            {
                for (size_t tx = 0; tx < tilesX; ++tx)
                {
                    std::unordered_map<unsigned int, int> colorCount;
                    for (size_t py = 0; py < tileSize; ++py)
                    {
                        for (size_t px = 0; px < tileSize; ++px)
                        {
                            size_t ix = tx * tileSize + px;
                            size_t iy = ty * tileSize + py;
                            Color c = pixels[iy * fullImg.width + ix];
                            uint32_t key = *(uint32_t*)&c;
                            colorCount[key]++;
                        }
                    }

                    // find most frequent color
                    int maxCount = 0;
                    Color mode = BLANK;
                    for (const auto& [key, count] : colorCount)
                    {
                        if (count > maxCount)
                        {
                            maxCount = count;
                            mode = *(Color*)&key;
                        }
                    }

                    float sx = static_cast<float>(tx) * (static_cast<float>(mini.texture.width) / static_cast<float>(tilesX));
                    float sy = static_cast<float>(ty) * (static_cast<float>(mini.texture.height) / static_cast<float>(tilesY));
                    float sw = static_cast<float>(mini.texture.width) / static_cast<float>(tilesX);
                    float sh = static_cast<float>(mini.texture.height) / static_cast<float>(tilesY);
                    DrawRectangleRec({ sx, sy, sw, sh }, mode);
                }
            }
            EndTextureMode();

            UnloadImageColors(pixels);
            UnloadImage(fullImg);
            UnloadRenderTexture(normal);

            minimapTextures[level].push_back(mini);
        }
    }
}

void Dungeon::renderMinimap(float hudY, float gameScreenWidth)
{
    auto [cols, rows] = getSize();
    const size_t currentRoomIdx = currentRoomIndex;
    const int spacing = 1;
    const int cellWidth = 6;
    const int cellHeight = 4;
    const int mapX = static_cast<int>(gameScreenWidth) - static_cast<int>(cols) * (cellWidth + spacing) - 6;
    const int mapY = static_cast<int>(hudY) + 6;

    for (size_t i = 0; i < cols * rows; ++i)
    {
        int col = static_cast<int>(i % cols);
        int row = static_cast<int>(i / cols);
        int cellX = mapX + col * (cellWidth + spacing);
        int cellY = mapY + row * (cellHeight + spacing);

        Color color = { 0 };
        if (i == currentRoomIdx)
        {
            color = WHITE;
        }
        else
        {
            Room* room = getRoomAt(currentLevel, i);
            if (room && room->visited)
            {
                color = GRAY;
            }
            else
            {
                color = DARKGRAY;
            }
        }

        DrawRectangle(cellX, cellY, cellWidth, cellHeight, color);
    }
}

void Dungeon::renderMapScreen(const MapRenderParams& params)
{
    const auto [cols, rows] = getSize();
    size_t currentRoomIdx = currentRoomIndex;

    const size_t cellWidth = (static_cast<size_t>(params.width) - 2 * params.border - (cols - 1) * params.spacing - params.offsetX) / cols;
    const size_t cellHeight = (static_cast<size_t>(params.height) - 2 * params.border - (rows - 1) * params.spacing - params.offsetY) / rows;

    // calculate door offsets for minimap
    std::array<Vector2, 4> offsets;
    offsets[0].x = float(cellWidth);
    offsets[0].y = float(cellHeight / 2 - params.spacing / 2);
    offsets[1].x = float(cellWidth / 2 - params.spacing / 2);
    offsets[1].y = -1.0f * float(params.spacing);
    offsets[2].x = -1.0f * float(params.spacing);
    offsets[2].y = float(cellHeight / 2 - params.spacing / 2);
    offsets[3].x = float(cellWidth / 2 - params.spacing / 2);
    offsets[3].y = float(cellHeight);

    for (size_t i = 0; i < cols * rows; ++i)
    {
        size_t col = i % cols;
        size_t row = i / cols;
        size_t cellX = params.offsetX + static_cast<size_t>(params.x) + params.border + col * (cellWidth + params.spacing);
        size_t cellY = params.offsetY + static_cast<size_t>(params.y) + params.border + row * (cellHeight + params.spacing);
        Color color = DARKGRAY;
        DrawRectangle(int(cellX), int(cellY), int(cellWidth), int(cellHeight), color);

        Room* room = getRoomAt(params.displayLevel, i);
        if (room && room->visited && i < minimapTextures[params.displayLevel].size())
        {
            const auto& tex = minimapTextures[params.displayLevel][i].texture;
            Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
            Rectangle dst = { (float)cellX, (float)cellY, (float)cellWidth, (float)cellHeight };
            DrawTexturePro(tex, src, dst, { 0, 0 }, 0.0f, WHITE);

            // indicate connections between rooms
            uint8_t doors = room->doors;

            for (int j = 3; j >= 0; j--)
            {
                bool isDoor = (doors >> j) & 1;
                if (isDoor)
                {
                    Rectangle r = {
                        dst.x + offsets[3 - j].x,
                        dst.y + offsets[3 - j].y,
                        float(params.spacing),
                        float(params.spacing)
                    };
                    DrawRectangleRec(r, color);
                }
            }

            if (i == currentRoomIdx && currentLevel == params.displayLevel && params.showCursor)
            {
                // draw player as blinking sprite
                const Vector2& pos = game.getPlayer()->position;
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

WorldGraph Dungeon::buildGraphFromDungeon(const std::string& start, const std::vector<std::tuple<std::string, std::string, std::vector<std::string>>>& edges, const std::unordered_set<std::string>& itemNodes)
{
    WorldGraph graph;

    // turn each room from each level into a node
    for (size_t level = 0; level < levels.size(); ++level)
    {
        for (size_t i = 0; i < roomsW * roomsH; i++)
        {
            Room* room = getRoomAt(level, i);
            if (room)
            {
                size_t row = i / roomsW;
                size_t col = i % roomsW;
                std::string roomId = std::to_string(level) + "_" + std::to_string(row) + "_" + std::to_string(col);
                bool canHaveItem = itemNodes.count(roomId) > 0;
                graph.addNode(roomId, i, canHaveItem);
            }
        }
    }

    graph.setStart(start);

    // helper to get requirements for a specific edge
    auto get_requirements = [&](const std::string& from, const std::string& to)
        {
            std::unordered_set<std::string> reqs;
            for (const auto& [f, t, items] : edges)
            {
                if (f == from && t == to)
                {
                    reqs.insert(items.begin(), items.end());
                    break;
                }
            }
            return reqs;
        };

    // add edges based on doors for all levels
    for (size_t level = 0; level < levels.size(); ++level)
    {
        for (size_t i = 0; i < roomsW * roomsH; i++)
        {
            Room* room = getRoomAt(level, i);
            if (room)
            {
                size_t row = i / roomsW;
                size_t col = i % roomsW;
                std::string fromId = std::to_string(level) + "_" + std::to_string(row) + "_" + std::to_string(col);

                // RIGHT
                if (((room->doors >> 3) & 1) && (col != roomsW - 1))
                {
                    if (Room* right = getRoomAt(level, i + 1))
                    {
                        std::string toId = std::to_string(level) + "_" + std::to_string(row) + "_" + std::to_string(col + 1);
                        graph.addEdge(fromId, toId, get_requirements(fromId, toId));
                    }
                }
                // UP
                if (((room->doors >> 2) & 1) && (row > 0))
                {
                    if (Room* up = getRoomAt(level, i - roomsW))
                    {
                        std::string toId = std::to_string(level) + "_" + std::to_string(row - 1) + "_" + std::to_string(col);
                        graph.addEdge(fromId, toId, get_requirements(fromId, toId));
                    }
                }
                // LEFT
                if (((room->doors >> 1) & 1) && (col > 0))
                {
                    if (Room* left = getRoomAt(level, i - 1))
                    {
                        std::string toId = std::to_string(level) + "_" + std::to_string(row) + "_" + std::to_string(col - 1);
                        graph.addEdge(fromId, toId, get_requirements(fromId, toId));
                    }
                }
                // DOWN
                if ((room->doors & 1) && (row < roomsH - 1))
                {
                    if (Room* down = getRoomAt(level, i + roomsW))
                    {
                        std::string toId = std::to_string(level) + "_" + std::to_string(row + 1) + "_" + std::to_string(col);
                        graph.addEdge(fromId, toId, get_requirements(fromId, toId));
                    }
                }
            }
        }
    }

    // add custom edges (level connections and edges without corresponding doors)
    for (const auto& [from, to, requirements] : edges)
    {
        std::unordered_set<std::string> reqs(requirements.begin(), requirements.end());
        graph.addEdge(from, to, reqs);
    }

    return graph;
}