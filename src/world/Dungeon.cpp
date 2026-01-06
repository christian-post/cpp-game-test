#include "Dungeon.h"
#include "Game.h"
#include "WorldGraph.h"

Level::Level(size_t roomsW, size_t roomsH) : roomsW { roomsW }, roomsH{ roomsH }
{
    rooms.resize(roomsW * roomsH);
}

std::vector<std::optional<Room>>& Level::getRooms()
{
    return rooms;
}

Room* Level::getRoomAt(size_t index)
{
    if (index >= rooms.size())
        return nullptr;

    return rooms[index].has_value() ? &*rooms[index] : nullptr;
}

void Level::insertRoom(size_t index, Room&& room) 
{
    rooms[index] = std::move(room);
}


// DUNGEON 

Dungeon::Dungeon(Game& game, size_t roomsW, size_t roomsH, size_t numLevels) : game{ game }, roomsW{ roomsW }, roomsH{ roomsH } 
{
    levels.reserve(numLevels);
    for (size_t i = 0; i < numLevels; ++i)
        levels.emplace_back(roomsW, roomsH);

    minimapTextures.resize(numLevels);
}

std::vector<std::optional<Room>>& Dungeon::getRooms() 
{
    return levels[currentLevel].getRooms();
}

std::vector<std::optional<Room>>& Dungeon::getRooms(size_t level) 
{
    return levels[level].getRooms();
}

Room* Dungeon::getRoomAt(size_t level, size_t index) 
{
    return levels[level].getRoomAt(index);
}

void Dungeon::setStartingRoomIndex(size_t index)
{
    startingRoomIndex = index;
    if (!playerHasBeenPlaced)
    {
        playerHasBeenPlaced = true;
        currentRoomIndex = index;
    }
}

void Dungeon::advanceRoomState() 
{
    advanceRoomState(currentLevel, currentRoomIndex);
}

void Dungeon::advanceRoomState(size_t level, size_t index) 
{
    Room* room = getRoomAt(level, index);

    if (!room)
    {
        TraceLog(LOG_ERROR, "advanceRoomState(): No room on level %d at index %d", level, index);
        return;
    }
    // make sure the room state is at least 1
    (room->state <<= 1) || (room->state = 1);
    TraceLog(LOG_INFO, "Room state of %s is now %d", room->tilemap.getName().c_str(), room->state);
}

uint8_t Dungeon::getCurrentRoomState()
{
    return levels[currentLevel].getRoomAt(currentRoomIndex)->state;
}

bool Dungeon::isRoomDark()
{
    Room* room = getRoomAt(currentLevel, currentRoomIndex);
    return room ? room->dark : false;
}

uint8_t Dungeon::getRoomDoors(size_t level, size_t index)
{
    Room* room = getRoomAt(level, index);
    if (!room)
    {
        TraceLog(LOG_ERROR, "getRoomDoors(): No room on level %d at index %d", level, index);
        return 0;
    }
    return room->doors;
}

std::unordered_map<uint32_t, ObjectState>& Dungeon::getCurrentRoomObjectStates()
{
    Room* room = getRoomAt(currentLevel, currentRoomIndex);
    return room->objectStates;
}

std::unordered_map<uint32_t, ObjectState>& Dungeon::getRoomObjectStates(size_t level, size_t index)
{
    Room* room = getRoomAt(level, index);
    if (!room)
    {
        throw std::runtime_error("getRoomObjectStates(): No room on level" + std::to_string(level) + "at index" + std::to_string(index));  // TODO can I handle this elegantly, or should the game always crash?
    }
    return room->objectStates;
}

const TileMap* Dungeon::loadTileMap(bool setRoomVisited)
{
    Room* room = getRoomAt(currentLevel, currentRoomIndex);
    if (!room)
    {
        // TODO this prints indefinitely when entering a non-valid room 
        //TraceLog(LOG_ERROR, "loadTileMap(): No room on level %d at index %d", currentLevel, currentRoomIndex);
        return nullptr;
    }
    if (setRoomVisited)
        setVisited(currentLevel, currentRoomIndex); // TODO: is it too obscure to set this here?
    return &room->tilemap;
}

const TileMap* Dungeon::loadTileMap(size_t level, size_t index)
{
    Room* room = getRoomAt(level, index);
    if (!room)
    {
        TraceLog(LOG_ERROR, "loadTileMap(): No room on level %d at index %d", level, index);
        return nullptr;
    }
    return &room->tilemap;
}

void Dungeon::insertRoom(size_t level, size_t row, size_t col, Room&& room) 
{
    // check if there is a room already
    size_t index = row * roomsW + col;

    // resize levels vector if necessary
    if (level >= levels.size())
    {
        while (level >= levels.size())
            levels.emplace_back(roomsW, roomsH);
    }

    if (!getRoomAt(level, index))
    {
        levels[level].insertRoom(index, std::move(room)); // TODO this room will be moved twice if I do it like this
    }
    else
    {
        TraceLog(LOG_WARNING, "insertRoom(): A room already exists on level %d at index %s", level, index);
    }
}

std::pair<size_t, size_t> Dungeon::getSize() const
{
    return { roomsW, roomsH };
}

std::pair<size_t, size_t> Dungeon::getRoomSize(size_t level, size_t index)
{
    Room* room = getRoomAt(level, index);
    if (!room)
    {
        TraceLog(LOG_ERROR, "getRoomSize(): No room on level %d at index %d", level, index);
        return { 0, 0 };
    }
    size_t w = room->tilemap.width;
    size_t h = room->tilemap.height;
    size_t ts = room->tilemap.tileWidth;
    return { w * ts, h * ts };
}

bool Dungeon::hasVisited(size_t level, size_t index)
{
    Room* room = getRoomAt(level, index);
    return room? room->visited : false;
}

void Dungeon::setVisited(size_t level, size_t index)
{
    Room* room = getRoomAt(level, index);
    if (!room)
    {
        TraceLog(LOG_ERROR, "setVisited(): No room on level %d at index %d", level, index);
        return;
    }
    room->visited = true;
}

void Dungeon::makeMinimapTextures()
{
    // creates downscaled images of the rooms for the mini map
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
                // store an empty texture for nonexistent rooms
                RenderTexture2D mini = LoadRenderTexture(miniWidth, miniHeight);
                BeginTextureMode(mini);
                ClearBackground(BLANK);
                EndTextureMode();
                minimapTextures[level].push_back(mini); // TODO crashes when loading a save
                continue;
            }
            auto& tileMap = room->tilemap;

            // get all tileset information and prepare lookup cache
            const auto& tilesetInfos = tileMap.getTilesetNames();

            struct TilesetData {
                std::string name;
                const Tileset* tileset;
                const Texture2D* texture;
                size_t tilesPerRow;
                int firstGid;
            };

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

            std::pair<size_t, size_t> tilemapSize = getRoomSize(level, i);
            size_t tilesX = tilemapSize.first / tileSize;
            size_t tilesY = tilemapSize.second / tileSize;

            // NEW: scaling down the room image AFTER all tiles have been drawn
            RenderTexture2D normal = LoadRenderTexture(static_cast<int>(tilemapSize.first), static_cast<int>(tilemapSize.second)); // 1:1 size
            RenderTexture2D mini = LoadRenderTexture(miniWidth, miniHeight); // downscaled room texture
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

                        // sample the source rect from the normally sized tilemap
                        float tileX = static_cast<float>(tileIndex % tilesetData->tilesPerRow) * tileSize;
                        float tileY = (static_cast<float>(tileIndex) / static_cast<float>(tilesetData->tilesPerRow)) * tileSize;
                        Rectangle src = { tileX, tileY, (float)tileSize, (float)tileSize };

                        float px = static_cast<float>(x) * static_cast<float>(tileSize);
                        float py = static_cast<float>(y) * static_cast<float>(tileSize);
                        Rectangle dst = { px, py, tileSize, tileSize };
                        // draw the tile
                        DrawTexturePro(*tilesetData->texture, src, dst, { 0, 0 }, 0.0f, WHITE);
                    }
                }
            }
            EndTextureMode();
            // draw to the small surface
            BeginTextureMode(mini);
            // TODO: testing mode filtering for less noisy images
            Image fullImg = LoadImageFromTexture(normal.texture);
            Color* pixels = LoadImageColors(fullImg);

            BeginTextureMode(mini);
            ClearBackground(BLANK);
            for (size_t ty = 0; ty < tilesY; ++ty)
            {
                for (size_t tx = 0; tx < tilesX; ++tx)
                {
                    std::unordered_map<unsigned int, int> colorCount; // hash table that counts pixel colors
                    for (size_t py = 0; py < tileSize; ++py)
                    {
                        for (size_t px = 0; px < tileSize; ++px)
                        {
                            size_t ix = tx * tileSize + px;
                            size_t iy = ty * tileSize + py;
                            Color c = pixels[iy * fullImg.width + ix];
                            uint32_t key = *(uint32_t*)&c; // use raw bytes of color as hash
                            colorCount[key]++;
                        }
                    }
                    // find most frequent color in hash table
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

            minimapTextures[level].push_back(mini);
        }
    }
}

WorldGraph Dungeon::buildGraphFromDungeon(const std::string& start, const std::vector<std::tuple<std::string, std::string, std::vector<std::string>>>& edges, const std::unordered_set<std::string>& itemNodes)
{
    WorldGraph graph;

    // Turn each room from each level into a node
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

    // Helper to get requirements for a specific edge from edges parameter
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

    // Add edges based on doors for all levels
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

    // Add custom edges (level connections and edges without corresponding doors)
    for (const auto& [from, to, requirements] : edges)
    {
        std::unordered_set<std::string> reqs(requirements.begin(), requirements.end());
        graph.addEdge(from, to, reqs);
    }

    return graph;
}

void Dungeon::generate(const nlohmann::json& dungeonData)
{
    // Set starting room from coordinates
    size_t startingLevel = dungeonData["starting_level"];
    std::vector<int> startCoords = dungeonData["starting_room"];
    size_t startIndex = startCoords[0] * roomsW + startCoords[1];
    setStartingRoomIndex(startIndex);
    currentLevel = startingLevel;

    // Get item pool for this dungeon
    std::vector<std::string> itemPool = dungeonData["item_pool"];

    // Collect edges and item nodes across all levels
    std::vector<std::tuple<std::string, std::string, std::vector<std::string>>> edges;
    std::unordered_set<std::string> itemNodes;

    // Process each level
    for (size_t levelIndex = 0; levelIndex < dungeonData["levels"].size(); ++levelIndex)
    {
        const auto& levelData = dungeonData["levels"][levelIndex];

        // Insert all rooms for this level into the levels vector
        for (const auto& roomData : levelData["rooms"])
        {
            size_t row = roomData["row"];
            size_t column = roomData["column"];
            std::string tilemapName = roomData["tilemap"];
            std::string doorsStr = roomData["doors"];
            uint8_t doors = static_cast<uint8_t>(std::stoi(doorsStr, nullptr, 2));

            insertRoom(levelIndex, row, column, Room{ game.loader.getTilemap(tilemapName), doors });
        }

        // Collect edges with item requirements from this level for the graph
        for (const auto& edge : levelData["edges"])
        {
            std::vector<int> fromCoords = edge["from"];
            std::vector<int> toCoords = edge["to"];
            std::vector<std::string> requiredItems = edge["required_items"];
            // edge identifiers are build like "0_3_1" (level 0, row 3, column 1)
            std::string fromId = std::to_string(levelIndex) + "_" + std::to_string(fromCoords[0]) + "_" + std::to_string(fromCoords[1]);
            std::string toId = std::to_string(levelIndex) + "_" + std::to_string(toCoords[0]) + "_" + std::to_string(toCoords[1]);

            edges.emplace_back(fromId, toId, requiredItems);
        }

        // Collect item nodes from this level
        for (const auto& coords : levelData["item_nodes"])
        {
            std::vector<int> coordPair = coords;
            std::string roomId = std::to_string(levelIndex) + "_" + std::to_string(coordPair[0]) + "_" + std::to_string(coordPair[1]);
            itemNodes.insert(roomId);
        }
    }

    // Add level connections (also bidirectional)
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

    // Build starting room identifier
    std::string startingRoomId = std::to_string(startingLevel) + "_" + std::to_string(startCoords[0]) + "_" + std::to_string(startCoords[1]);

    // Build and fill world graph
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
        // TODO help finding the reason why the generation failed
        G.logDebug();

        TraceLog(LOG_INFO, "Dungeon Size");
        for (size_t lvl = 0; lvl < levels.size(); lvl++)
        {
            TraceLog(LOG_INFO, "Level %d: %d rooms.", lvl, levels[lvl].getRooms().size());
        }

        G.testReachability();

        size_t numItemsLeft = G.itemPool.size();
        TraceLog(LOG_INFO, "There are still %d items in the pool.", numItemsLeft);

        throw std::runtime_error("Failed to place all items after " + std::to_string(max_attempts) + " attempts");
    }

    // Place items into Tiled data
    for (const auto& [roomId, node] : G.nodes)
    {
        if (node->item.has_value())
        {
            TraceLog(LOG_INFO, "[%s] placed item: %s", roomId.c_str(), node->item->c_str());

            // Parse level from roomId string "level_row_col"
            size_t firstUnderscore = roomId.find('_');
            size_t level = std::stoi(roomId.substr(0, firstUnderscore));

            auto& rooms = levels[level].getRooms();
            TileMap& roomData = rooms[node->id]->tilemap;
            std::vector<TileObject>& objects = roomData.getObjects();
            for (auto& obj : objects)
            {
                // replace empty chests with items
                if (obj.name == "chest" && obj.properties["item"] == "")
                //if (obj.name == "chest")
                {
                    obj.properties["item"] = node->item.value();
                    obj.properties["amount"] = 1;

                    // overwrite the object state
                    // TODO unify this and the json data??
                    auto& objectStates = rooms[node->id]->objectStates;
                    objectStates[obj.id].itemName = node->item.value();
                    objectStates[obj.id].itemAmount = 1;

                    break; // chooses the first chest in this room
                }
            }
        }
    }
    // print Dungeon graph to the console
    G.logDebug();
}
