#include "Dungeon.h"
#include <raylib.h>
#include "Game.h"

Dungeon::Dungeon(Game& game, size_t roomsW, size_t roomsH) : game{ game }, roomsW { roomsW }, roomsH{ roomsH }
{
	rooms.resize(roomsW * roomsH);
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

uint8_t Dungeon::getRoomDoors(size_t index)
{
    return rooms[index]->doors;
}

std::unordered_map<uint32_t, ObjectState>& Dungeon::getCurrentRoomObjectStates()
{
    return rooms[currentRoomIndex]->objectStates;
}

const TileMap* Dungeon::loadCurrentTileMap()
{
    if (currentRoomIndex > rooms.size()) {
        TraceLog(LOG_ERROR, "Index %d is out of bounds for Rooms array with size %d", currentRoomIndex, rooms.size());
        return nullptr;
    }
    if (!rooms[currentRoomIndex]) {
        TraceLog(LOG_ERROR, "No room at current index");
        return nullptr;
    }
    setVisited(currentRoomIndex); // TODO: is it always correct to set this here?
    return &rooms[currentRoomIndex]->tilemap;
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
        //float scaleX = (float)miniWidth / (tileMap->width * tileSize);
        //float scaleY = (float)miniHeight / (tileMap->height * tileSize);

        // NEW: scaling down the room image AFTER all tiles have been drawn
        std::pair<size_t, size_t> tilemapSize = getRoomSize(currentRoomIndex);
        RenderTexture2D normal = LoadRenderTexture(tilemapSize.first, tilemapSize.second); // 1:1 size
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

                    float px = x * tileSize;
                    float py = y * tileSize;
                    Rectangle dst = { px, py, tileSize, tileSize };
                    // draw the tile
                    DrawTexturePro(texture, src, dst, { 0, 0 }, 0.0f, WHITE);
                }
            }
        }
        EndTextureMode();
        BeginTextureMode(mini);
        Rectangle src = { 0.0f, 0.0f, static_cast<float>(normal.texture.width), static_cast<float>(normal.texture.height) };
        Rectangle dst = { 0.0f, 0.0f, static_cast<float>(mini.texture.width), static_cast<float>(mini.texture.height) };
        DrawTexturePro(normal.texture, src, dst, { 0, 0 }, 0.0f, WHITE);
        EndTextureMode();
        minimapTextures.push_back(mini);
    }
}
