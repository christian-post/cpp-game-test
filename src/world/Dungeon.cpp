#include "Dungeon.h"
#include <raylib.h>
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
    if (!playerHasBeenPlaced) {
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

    if (!room) {
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
    if (!room) {
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
    if (!room) {
        TraceLog(LOG_ERROR, "getRoomObjectStates(): No room on level %d at index %d", level, index);
        throw;  // TODO can I handle this elegantly, or should the game always crash?
    }
    return room->objectStates;
}

const TileMap* Dungeon::loadTileMap()
{
    Room* room = getRoomAt(currentLevel, currentRoomIndex);
    if (!room) {
        TraceLog(LOG_ERROR, "loadTileMap(): No room on level %d at index %d", currentLevel, currentRoomIndex);
        return nullptr;
    }
    setVisited(currentLevel, currentRoomIndex); // TODO: is it always correct to set this here?
    //return &rooms[currentRoomIndex]->tilemap;
    return &room->tilemap;
}

const TileMap* Dungeon::loadTileMap(size_t level, size_t index)
{
    Room* room = getRoomAt(level, index);
    if (!room) {
        TraceLog(LOG_ERROR, "loadTileMap(): No room on level %d at index %d", level, index);
        return nullptr;
    }
    return &room->tilemap;
}

void Dungeon::insertRoom(size_t level, size_t row, size_t col, Room&& room) 
{
    // check if there is a room already
    size_t index = row * roomsW + col;

    if (!getRoomAt(level, index)) {
        levels[level].insertRoom(index, std::move(room)); // TODO this room will be moved twice if I do it like this
    }
    else {
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
    if (!room) {
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
    if (!room) {
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

    for (size_t level = 0; level < levels.size(); level++) {
        for (size_t i = 0; i < roomsW * roomsH; i++) {
            Room* room = getRoomAt(level, i);
            if (!room) {
                // store an empty texture for nonexistent rooms
                RenderTexture2D mini = LoadRenderTexture(miniWidth, miniHeight);
                BeginTextureMode(mini);
                ClearBackground(BLANK);
                EndTextureMode();
                minimapTextures[level].push_back(mini);
                continue;
            }
            auto& tileMap = room->tilemap;
            const Tileset& tileset = game.loader.getTileset(tileMap.getTilesetName());
            const Texture2D& texture = game.loader.getTextures(tileset.name)[0];
            const size_t tilesPerRow = tileset.columns;
            std::pair<size_t, size_t> tilemapSize = getRoomSize(level, i);
            size_t tilesX = tilemapSize.first / tileSize;
            size_t tilesY = tilemapSize.second / tileSize;

            // NEW: scaling down the room image AFTER all tiles have been drawn
            RenderTexture2D normal = LoadRenderTexture(static_cast<int>(tilemapSize.first), static_cast<int>(tilemapSize.second)); // 1:1 size
            RenderTexture2D mini = LoadRenderTexture(miniWidth, miniHeight); // downscaled room texture
            BeginTextureMode(normal);
            ClearBackground(BLANK);
            for (size_t layerIndex = 0; layerIndex < tileMap.layers.size(); ++layerIndex) {
                const auto& layer = tileMap.getLayer(layerIndex);
                if (!layer.visible) continue;
                for (size_t y = 0; y < tileMap.height; ++y) {
                    for (size_t x = 0; x < tileMap.width; ++x) {
                        if (!layer.data[y][x]) continue;
                        // sample the source rect from the normally sized Tilemap
                        size_t tileIndex = static_cast<size_t>(layer.data[y][x] - 1);
                        float tileX = static_cast<float>(tileIndex % tilesPerRow) * tileSize;
                        float tileY = (static_cast<float>(tileIndex) / static_cast<float>(tilesPerRow)) * tileSize;
                        Rectangle src = { tileX, tileY, (float)tileSize, (float)tileSize };

                        float px = static_cast<float>(x) * static_cast<float>(tileSize);
                        float py = static_cast<float>(y) * static_cast<float>(tileSize);
                        Rectangle dst = { px, py, tileSize, tileSize };
                        // draw the tile
                        DrawTexturePro(texture, src, dst, { 0, 0 }, 0.0f, WHITE);
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
            for (size_t ty = 0; ty < tilesY; ++ty) {
                for (size_t tx = 0; tx < tilesX; ++tx) {
                    std::unordered_map<unsigned int, int> colorCount; // hash table that counts pixel colors
                    for (size_t py = 0; py < tileSize; ++py) {
                        for (size_t px = 0; px < tileSize; ++px) {
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
                    for (const auto& [key, count] : colorCount) {
                        if (count > maxCount) {
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

    // turn each room into a node
    // TODO connections between levels

    for (size_t i = 0; i < roomsW * roomsH; i++) {
        Room* room = getRoomAt(currentLevel, i);
        if (room) {
            std::string key = room->tilemap.getName();
            bool canHaveItem = itemNodes.count(key) > 0;
            graph.add_node(key, i, canHaveItem);
        }
    }
    graph.set_start(start);

    // add edges (connections between rooms)
    auto get_requirements = [&](const std::string& from, const std::string& to) {
        std::unordered_set<std::string> reqs;
        for (const auto& [f, t, items] : edges) {
            if (f == from && t == to) {
                reqs.insert(items.begin(), items.end());
                break;
            }
        }
        return reqs;
        };

    for (size_t i = 0; i < roomsW * roomsH; i++) {
        Room* room = getRoomAt(currentLevel, i);
        if (room) {
            std::string key = room->tilemap.getName();
            // check the room's doors
            // RIGHT
            if (((room->doors >> 3) & 1) && (i % roomsW != roomsW - 1)) {
                if (Room* right = getRoomAt(currentLevel, i + 1)) {
                    auto& to = right->tilemap.getName();
                    graph.add_edge(key, to, get_requirements(key, to));
                }
            }
            // UP
            if (((room->doors >> 2) & 1) && (i >= roomsW)) {
                if (Room* up = getRoomAt(currentLevel, i - roomsW)) {
                    auto& to = up->tilemap.getName();
                    graph.add_edge(key, to, get_requirements(key, to));
                }
            }
            // LEFT
            if (((room->doors >> 1) & 1) && (i % roomsW != 0)) {
                if (Room* left = getRoomAt(currentLevel, i - 1)) {
                    auto& to = left->tilemap.getName();
                    graph.add_edge(key, to, get_requirements(key, to));
                }
            }
            // DOWN
            if ((room->doors & 1) && (i + roomsW < levels[currentLevel].getRooms().size())) {
                if (Room* down = getRoomAt(currentLevel, i + roomsW)) {
                    auto& to = down->tilemap.getName();
                    graph.add_edge(key, to, get_requirements(key, to));
                }
            }
        }
    }

    return graph;
}

void Dungeon::generate()
{
    // TODO: I'm hardcoding this here for now

    // coordinates are row, column
    // Room: second argument is the directions of the doors, starting at the right and going counter clockwise

    // hard-coding the first dungeon
    // TODO get this from JSON data
    insertRoom(0, 0, 1, Room{ game.loader.getTilemap("dungeon_shop"), 0b0001 });
    insertRoom(0, 0, 2, Room{ game.loader.getTilemap("test_map_small"), 0b0000 }); // test room, hidden
    insertRoom(0, 0, 5, Room{ game.loader.getTilemap("dungeon_final_boss_0001"), 0b0001 });

    insertRoom(0, 1, 1, Room{ game.loader.getTilemap("dungeon_hallway_1101"), 0b1101 });
    insertRoom(0, 1, 2, Room{ game.loader.getTilemap("dungeon_turrets_1010"), 0b1010 });
    insertRoom(0, 1, 3, Room{ game.loader.getTilemap("dungeon_spikes_1011"), 0b1011 });
    insertRoom(0, 1, 4, Room{ game.loader.getTilemap("dungeon_4skelets_1010"), 0b1010 });
    insertRoom(0, 1, 5, Room{ game.loader.getTilemap("dungeon_before_boss_0110"), 0b0110 });

    insertRoom(0, 2, 1, Room{ game.loader.getTilemap("dungeon006"), 0b0101 });
    insertRoom(0, 2, 3, Room{ game.loader.getTilemap("dungeon_turrets_0101"), 0b0101 });
    insertRoom(0, 2, 4, Room{ game.loader.getTilemap("dungeon_empty_1101"), 0b1101 });
    insertRoom(0, 2, 5, Room{ game.loader.getTilemap("dungeon_chest_0010"), 0b0010 });

    insertRoom(0, 3, 0, Room{ game.loader.getTilemap("dungeon007"), 0b1000 });
    insertRoom(0, 3, 1, Room{ game.loader.getTilemap("dungeon003"), 0b1111 });
    insertRoom(0, 3, 2, Room{ game.loader.getTilemap("dungeon002"), 0b0011 });
    insertRoom(0, 3, 3, Room{ game.loader.getTilemap("dungeon_hallway_1100"), 0b1100 });
    insertRoom(0, 3, 4, Room{ game.loader.getTilemap("dungeon_empty_1110"), 0b1110 });
    insertRoom(0, 3, 5, Room{ game.loader.getTilemap("dungeon_fight_0011"), 0b0011 });

    insertRoom(0, 4, 1, Room{ game.loader.getTilemap("dungeon004"), 0b1100 });
    insertRoom(0, 4, 2, Room{ game.loader.getTilemap("dungeon001"), 0b1111 }); // starting room
    insertRoom(0, 4, 3, Room{ game.loader.getTilemap("dungeon_2skelets_1010"), 0b1010 });
    insertRoom(0, 4, 4, Room{ game.loader.getTilemap("dungeon005"), 0b0010 });
    insertRoom(0, 4, 5, Room{ game.loader.getTilemap("dungeon_fight_chest_0100"), 0b0100 });

    setStartingRoomIndex(26); // start in R1

    // TODO: testing item requirements
    std::vector<std::tuple<std::string, std::string, std::vector<std::string>>> edges = {
        { "dungeon001", "dungeon002", { "__impossible__" }}, // should only be opened from the top
        { "dungeon002", "dungeon001", { "key" }},
        { "dungeon003", "dungeon006", { "key" }},
        { "dungeon_before_boss_0110", "dungeon_final_boss_0001", { "key" }},
        { "dungeon004", "dungeon003", { "weapon_sword" }},
        { "dungeon006", "dungeon_shop", { "weapon_sword" }},
        { "dungeon003", "dungeon007", { "item_lamp" }},
        { "dungeon_turrets_0101", "dungeon_spikes_1011", { "item_lamp" }},
        { "dungeon_turrets_0101", "dungeon_hallway_1100", { "weapon_bow" }},
    };
    std::unordered_set<std::string> itemNodes = { "dungeon002", "dungeon005", "dungeon007", "dungeon_shop", "dungeon_4skelets_1010", "dungeon_chest_0010", "dungeon_fight_chest_0100"};  // TODO shop has two chests (for now)

    WorldGraph G = buildGraphFromDungeon("dungeon001", edges, itemNodes);
    int attempts = 0;
    const int max_attempts = 100;

    do {
        G.initialize_items({ "key", "key", "key", "weapon_sword", "item_lamp", "weapon_bow" });
        G.forward_fill();
        attempts++;
    } while (!G.item_pool.empty() && attempts < max_attempts);

    if (!G.item_pool.empty()) {
        throw std::runtime_error("Failed to place all items after 100 attempts");
    }

    // put the items into the Tiled data
    // TODO do this for every level
    auto& rooms = levels[0].getRooms();
    for (const auto& [name, node] : G.nodes) {
        if (node->item.has_value()) {
            TraceLog(LOG_INFO, "[%s] placed item: %s", name.c_str(), node->item->c_str());
            TileMap& roomData = rooms[node->id]->tilemap;
            std::vector<TileObject>& objects = roomData.getObjects();
            for (auto& obj : objects) {
                if (obj.name == "chest") {
                    // put the item here
                    obj.properties["item"] = node->item.value();
                    obj.properties["amount"] = 1;
                    break;  // break in case there are multiple chests (TODO: does this work?)
                }
            }
        }
    }
    G.log_debug();
}
