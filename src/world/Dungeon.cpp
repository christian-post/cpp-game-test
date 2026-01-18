#include "Dungeon.h"
#include "Game.h"
#include "WorldGraph.h"
#include "TilemapRenderer.h"

Dungeon::Dungeon(Game& game, size_t roomsW, size_t roomsH, size_t numLevels, const std::string& name)
    : World(game, roomsW, roomsH, numLevels, name)
{
    isDungeon = true;
    //mapTextures.resize(numLevels);
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

    // save the player starting position
    if (dungeonData.contains("starting_position"))
    {
        startingPosition.x = dungeonData["starting_position"]["x"];
        startingPosition.y = dungeonData["starting_position"]["y"];
    }
    else
    {
        // if not specified, use the center of the first room
        TileMap& tm = levels[startingLevel].getRoomAt(startingRoomIndex)->tilemap;
        startingPosition.x = (tm.width * tm.tileWidth) * 0.5f;
        startingPosition.y = (tm.height * tm.tileHeight) * 0.5f;
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