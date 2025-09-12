#include "Dungeon.h"
#include <raylib.h>
#include "Game.h"
#include "WorldGraph.h"

// Debug flags (comment in/out)
#define TEST_ROOM


Dungeon::Dungeon(Game& game, size_t roomsW, size_t roomsH) : game{ game }, roomsW { roomsW }, roomsH{ roomsH }
{
    rooms.resize(roomsW * roomsH);
}

std::vector<std::optional<Room>>& Dungeon::getRooms()
{
    return rooms;
}

Room* Dungeon::getRoomAt(size_t index) {
    if (index < 0 || index >= rooms.size() || !rooms[index])
        return nullptr;
    return &*rooms[index];
}

void Dungeon::setStartingRoomIndex(size_t index)
{
    startingRoomIndex = index;
    if (!playerHasBeenPlaced) {
        playerHasBeenPlaced = true;
        currentRoomIndex = index;
    }
}

void Dungeon::advanceRoomState() {
    advanceRoomState(currentRoomIndex);
}

void Dungeon::advanceRoomState(size_t index) {
    rooms[index]->state <<= 1;
    if (rooms[index]->state == 0)
        rooms[index]->state = 1;
    TraceLog(LOG_INFO, "Room state of %s is now %d", rooms[index]->tilemap.getName().c_str(), rooms[index]->state);
}

uint8_t Dungeon::getCurrentRoomState()
{
    return rooms[currentRoomIndex]->state;
}

bool Dungeon::isRoomDark()
{
    if (currentRoomIndex > rooms.size() || !rooms[currentRoomIndex])
        return false;
    return rooms[currentRoomIndex]->dark;
}

uint8_t Dungeon::getRoomDoors(size_t index)
{
    return rooms[index]->doors;
}

std::unordered_map<uint32_t, ObjectState>& Dungeon::getCurrentRoomObjectStates()
{
    return rooms[currentRoomIndex]->objectStates;
}

std::unordered_map<uint32_t, ObjectState>& Dungeon::getRoomObjectStates(size_t index)
{
    if (currentRoomIndex > rooms.size()) {
        TraceLog(LOG_ERROR, "Index %d is out of bounds for Rooms array with size %d", currentRoomIndex, rooms.size());
        throw; // TODO: handle this
    }
    if (!rooms[currentRoomIndex]) {
        throw;
    }
    return rooms[index]->objectStates;
}

const TileMap* Dungeon::loadCurrentTileMap()
{
    if (currentRoomIndex > rooms.size()) {
        TraceLog(LOG_ERROR, "Index %d is out of bounds for Rooms array with size %d", currentRoomIndex, rooms.size());
        return nullptr;
    }
    if (!rooms[currentRoomIndex]) {
        return nullptr;
    }
    setVisited(currentRoomIndex); // TODO: is it always correct to set this here?
    return &rooms[currentRoomIndex]->tilemap;
}

const TileMap* Dungeon::loadTileMapByIndex(size_t index)
{
    if (index > rooms.size()) {
        TraceLog(LOG_ERROR, "Index %d is out of bounds for Rooms array with size %d", index, rooms.size());
        return nullptr;
    }
    if (!rooms[index]) {
        return nullptr;
    }
    return &rooms[index]->tilemap;
}

void Dungeon::insertRoom(size_t row, size_t col, Room&& room) {
    size_t index = row * roomsW + col;
    if (!rooms[index]) {
        rooms[index] = std::move(room);
    }
    else {
        TraceLog(LOG_WARNING, "A room already exists at index %s", index);
    }
}

std::pair<size_t, size_t> Dungeon::getSize() const
{
    return { roomsW, roomsH };
}

std::pair<size_t, size_t> Dungeon::getRoomSize(size_t index) const
{
    size_t w = rooms[index]->tilemap.width;
    size_t h = rooms[index]->tilemap.height;
    size_t ts = rooms[index]->tilemap.tileWidth;
    return { w * ts, h * ts };
}

bool Dungeon::hasVisited(size_t index) const
{
    if (!rooms[index] || index > rooms.size()) return false;
    return rooms[index]->visited;
}

void Dungeon::setVisited(size_t index)
{
    if (index < rooms.size() && rooms[index]) {
        rooms[index]->visited = true;
    }
}

void Dungeon::makeMinimapTextures()
{
    // creates downscaled images of the rooms for the mini map
    constexpr int miniWidth = 36;
    constexpr int miniHeight = 24;
    constexpr int tileSize = 16;
    for (int i = 0; i < roomsW * roomsH; i++) {
        if (!rooms[i]) {
            // store an empty texture for nonexistent rooms
            RenderTexture2D mini = LoadRenderTexture(miniWidth, miniHeight);
            BeginTextureMode(mini);
            ClearBackground(BLANK);
            EndTextureMode();
            minimapTextures.push_back(mini);
            continue;
        }
        auto tileMap = &rooms[i]->tilemap;
        const Tileset& tileset = game.loader.getTileset(tileMap->getTilesetName());
        const Texture2D& texture = game.loader.getTextures(tileset.name)[0];
        const size_t tilesPerRow = tileset.columns;

        // NEW: scaling down the room image AFTER all tiles have been drawn
        std::pair<size_t, size_t> tilemapSize = getRoomSize(currentRoomIndex);
        RenderTexture2D normal = LoadRenderTexture(static_cast<int>(tilemapSize.first), static_cast<int>(tilemapSize.second)); // 1:1 size
        RenderTexture2D mini = LoadRenderTexture(miniWidth, miniHeight); // downscaled room texture
        BeginTextureMode(normal);
        ClearBackground(BLANK);
        for (size_t layerIndex = 0; layerIndex < tileMap->layers.size(); ++layerIndex) {
            const auto& layer = tileMap->getLayer(layerIndex);
            if (!layer.visible) continue;
            for (size_t y = 0; y < tileMap->height; ++y) {
                for (size_t x = 0; x < tileMap->width; ++x) {
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
        size_t tilesX = tilemapSize.first / tileSize;
        size_t tilesY = tilemapSize.second / tileSize;

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

        minimapTextures.push_back(mini);
    }
}

WorldGraph Dungeon::buildGraphFromDungeon(const std::string& start, const std::vector<std::tuple<std::string, std::string, std::vector<std::string>>>& edges, const std::unordered_set<std::string>& itemNodes)
{
    WorldGraph graph;

    // turn each room into a node
    for (size_t i = 0; i < roomsW * roomsH; i++) {
        if (rooms[i]) {
            std::string key = rooms[i]->tilemap.getName();
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
        if (rooms[i]) {
            std::string key = rooms[i]->tilemap.getName();
            // check the room's doors
            // RIGHT
            if (((rooms[i]->doors >> 3) & 1) && (i % roomsW != roomsW - 1)) {
                if (Room* right = getRoomAt(i + 1)) {
                    auto& to = right->tilemap.getName();
                    graph.add_edge(key, to, get_requirements(key, to));
                }
            }
            // UP
            if (((rooms[i]->doors >> 2) & 1) && (i >= roomsW)) {
                if (Room* up = getRoomAt(i - roomsW)) {
                    auto& to = up->tilemap.getName();
                    graph.add_edge(key, to, get_requirements(key, to));
                }
            }
            // LEFT
            if (((rooms[i]->doors >> 1) & 1) && (i % roomsW != 0)) {
                if (Room* left = getRoomAt(i - 1)) {
                    auto& to = left->tilemap.getName();
                    graph.add_edge(key, to, get_requirements(key, to));
                }
            }
            // DOWN
            if ((rooms[i]->doors & 1) && (i + roomsW < rooms.size())) {
                if (Room* down = getRoomAt(i + roomsW)) {
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

    /*
    ## test dungeon ##
      0    1    2    3    4
    0 .    0    .    .    .
    1 .    0    .    .    .
    2 0    0    0    .    .
    3 .    0    0    0    0
    */
    // coordinates are row, column
    // second argument is the directions of the doors, starting at the right and going counter clockwise

#ifdef TEST_ROOM
    insertRoom(0, 0, Room{ game.loader.getTilemap("test_map_small"), 0b0000 }); // test dungeon
#else
    insertRoom(3, 2, Room{ game.loader.getTilemap("dungeon001"), 0b1111 });
    insertRoom(2, 2, Room{ game.loader.getTilemap("dungeon002"), 0b0011 });
    insertRoom(2, 1, Room{ game.loader.getTilemap("dungeon003"), 0b1111 });
    insertRoom(3, 1, Room{ game.loader.getTilemap("dungeon004"), 0b1100 });
    insertRoom(3, 4, Room{ game.loader.getTilemap("dungeon005"), 0b0010 });
    insertRoom(1, 1, Room{ game.loader.getTilemap("dungeon006"), 0b0101 });
    insertRoom(2, 0, Room{ game.loader.getTilemap("dungeon007"), 0b1000 });
    insertRoom(0, 1, Room{ game.loader.getTilemap("dungeon_shop"), 0b0001 });
    insertRoom(3, 3, Room{ game.loader.getTilemap("dungeon_2skelets_1010"), 0b1010 });

#endif // TEST_ROOM

#ifdef TEST_ROOM
    setStartingRoomIndex(0); // TODO: testing
#else 
    setStartingRoomIndex(17); // start in R1
    // TODO: testing item requirements
    std::vector<std::tuple<std::string, std::string, std::vector<std::string>>> edges = {
        { "dungeon001", "dungeon002", { "__impossible__" }}, // should only be opened from the top
        { "dungeon002", "dungeon001", { "key" }},
        { "dungeon003", "dungeon006", { "key" }},
        { "dungeon004", "dungeon003", { "weapon_sword" }},
        { "dungeon003", "dungeon007", { "lamp" }}, // TODO 
        { "dungeon006", "dungeon_shop", { "weapon_sword" }}
    };
    std::unordered_set<std::string> itemNodes = { "dungeon002", "dungeon005", "dungeon007", "dungeon_shop"};

    WorldGraph G = buildGraphFromDungeon("dungeon001", edges, itemNodes);
    int attempts = 0;
    const int max_attempts = 100;

    do {
        G.initialize_items({ "key", "key", "weapon_sword", "lamp"});
        G.forward_fill();
        attempts++;
    } while (!G.item_pool.empty() && attempts < max_attempts);

    if (!G.item_pool.empty()) {
        throw std::runtime_error("Failed to place all items after 100 attempts");
    }

    // put the items into the Tiled data
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
                    break;  // break in case there are multiple chests (which shouldn't happen but whatever)
                }
            }
        }
    }
    G.log_debug();
#endif // TEST_ROOM
}
