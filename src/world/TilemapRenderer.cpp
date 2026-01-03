#include "TilemapRenderer.h"
#include "Game.h"
#include <cmath>
#include <algorithm>

TilemapRenderer::TilemapRenderer(Game& game) : game(game) {
}

TilemapRenderer::~TilemapRenderer() {
    cleanup();
}

void TilemapRenderer::loadTilemap(const TileMap* tilemap) {
    if (tilemap == currentTilemap) {
        return; // Already loaded
    }

    // Cleanup previous tilemap if exists
    cleanup();

    currentTilemap = tilemap;

    if (currentTilemap == nullptr) {
        return;
    }

    calculateDimensions();
    generateChunks();
}

void TilemapRenderer::drawLayer(int layerIndex, const Camera2D& camera) {
    if (currentTilemap == nullptr || layerIndex < 0 ||
        layerIndex >= static_cast<int>(currentTilemap->layers.size())) {
        return;
    }

    drawChunks(layerIndex, camera);
}

void TilemapRenderer::drawAllLayersExceptTop(const Camera2D& camera) {
    if (currentTilemap == nullptr) {
        return;
    }

    int layerCount = static_cast<int>(currentTilemap->layers.size());

    for (int i = 0; i < layerCount - 1; i++) {
        drawLayer(i, camera);
    }
}

void TilemapRenderer::drawTopLayer(const Camera2D& camera) {
    if (currentTilemap == nullptr) {
        return;
    }

    int layerCount = static_cast<int>(currentTilemap->layers.size());

    if (layerCount > 0) {
        drawLayer(layerCount - 1, camera);
    }
}

void TilemapRenderer::cleanup() {
    cleanupChunks();
    currentTilemap = nullptr;
    worldWidth = 0;
    worldHeight = 0;
    tileSize = 0;
    numChunksX = 0;
    numChunksY = 0;
}

void TilemapRenderer::calculateDimensions() {
    if (currentTilemap == nullptr) {
        return;
    }

    worldWidth = currentTilemap->width;
    worldHeight = currentTilemap->height;
    tileSize = currentTilemap->tileWidth;

    // Calculate number of chunks needed
    size_t worldPixelWidth = worldWidth * tileSize;
    size_t worldPixelHeight = worldHeight * tileSize;

    numChunksX = (worldPixelWidth + tileChunkSize - 1) / tileChunkSize; // Ceiling division
    numChunksY = (worldPixelHeight + tileChunkSize - 1) / tileChunkSize;
}

void TilemapRenderer::generateChunks() {
    if (currentTilemap == nullptr) {
        return;
    }

    cleanupChunks();

    size_t layerCount = currentTilemap->layers.size();
    tilemapChunks.resize(layerCount);

    for (size_t layerIndex = 0; layerIndex < layerCount; layerIndex++) {
        generateChunkForLayer(layerIndex);
    }
}

void TilemapRenderer::generateChunkForLayer(size_t layerIndex) {
    if (currentTilemap == nullptr || layerIndex >= currentTilemap->layers.size()) {
        return;
    }

    tilemapChunks[layerIndex].resize(numChunksX * numChunksY);

    const TileLayer& layer = currentTilemap->getLayer(layerIndex);

    if (!layer.visible) {
        return;
    }

    // Get tileset information
    const auto& tilesetInfos = currentTilemap->getTilesetNames();

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

    for (size_t chunkY = 0; chunkY < numChunksY; chunkY++)
    {
        for (size_t chunkX = 0; chunkX < numChunksX; chunkX++)
        {
            size_t chunkIndex = chunkY * numChunksX + chunkX;

            // create render texture for this chunk
            tilemapChunks[layerIndex][chunkIndex] = LoadRenderTexture(
                static_cast<int>(tileChunkSize),
                static_cast<int>(tileChunkSize)
            );

            // begin rendering to this chunk
            BeginTextureMode(tilemapChunks[layerIndex][chunkIndex]);
            ClearBackground(BLANK);

            // calculate tile boundaries for this chunk
            size_t startTileX = (chunkX * tileChunkSize) / tileSize;
            size_t startTileY = (chunkY * tileChunkSize) / tileSize;
            size_t endTileX = std::min(((chunkX + 1) * tileChunkSize) / tileSize, worldWidth);
            size_t endTileY = std::min(((chunkY + 1) * tileChunkSize) / tileSize, worldHeight);

            // render tiles in this chunk
            for (size_t tileY = startTileY; tileY < endTileY; tileY++)
            {
                for (size_t tileX = startTileX; tileX < endTileX; tileX++)
                {
                    if (tileY >= layer.data.size() || tileX >= layer.data[tileY].size())
                        continue;

                    int tileId = layer.data[tileY][tileX];
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

                    // calculate position within chunk
                    int drawX = static_cast<int>((tileX * tileSize) - (chunkX * tileChunkSize));
                    int drawY = static_cast<int>((tileY * tileSize) - (chunkY * tileChunkSize));

                    // calculate source rectangle from tileset
                    size_t srcTileX = (static_cast<size_t>(tileIndex) % tilesetData->tilesPerRow) * tileSize;
                    size_t srcTileY = (static_cast<size_t>(tileIndex) / tilesetData->tilesPerRow) * tileSize;

                    Rectangle sourceRect = {
                        static_cast<float>(srcTileX),
                        static_cast<float>(srcTileY),
                        static_cast<float>(tileSize),
                        static_cast<float>(tileSize)
                    };

                    Rectangle destRect = {
                        static_cast<float>(drawX),
                        static_cast<float>(drawY),
                        static_cast<float>(tileSize),
                        static_cast<float>(tileSize)
                    };

                    DrawTexturePro(*tilesetData->texture, sourceRect, destRect, { 0, 0 }, 0.0f, WHITE);
                }
            }

            EndTextureMode();
        }
    }
}

void TilemapRenderer::drawChunks(int layerIndex, const Camera2D& camera) {
    if (currentTilemap == nullptr || layerIndex < 0 ||
        layerIndex >= static_cast<int>(tilemapChunks.size())) {
        return;
    }

    for (size_t chunkY = 0; chunkY < numChunksY; chunkY++) {
        for (size_t chunkX = 0; chunkX < numChunksX; chunkX++) {
            if (!isChunkVisible(chunkX, chunkY, camera)) {
                continue;
            }

            size_t chunkIndex = chunkY * numChunksX + chunkX;

            if (chunkIndex >= tilemapChunks[layerIndex].size()) {
                continue;
            }

            RenderTexture2D& chunk = tilemapChunks[layerIndex][chunkIndex];

            if (chunk.id == 0) {
                continue; // Invalid texture
            }

            // Calculate world position for this chunk
            float worldX = static_cast<float>(chunkX * tileChunkSize);
            float worldY = static_cast<float>(chunkY * tileChunkSize);

            // Draw the chunk texture
            Rectangle sourceRect = {
                0, 0,
                static_cast<float>(chunk.texture.width),
                -static_cast<float>(chunk.texture.height) // Negative height to flip Y
            };

            Rectangle destRect = {
                worldX, worldY,
                static_cast<float>(tileChunkSize),
                static_cast<float>(tileChunkSize)
            };

            DrawTexturePro(chunk.texture, sourceRect, destRect, { 0, 0 }, 0.0f, WHITE);
        }
    }
}

bool TilemapRenderer::isChunkVisible(size_t chunkX, size_t chunkY, const Camera2D& camera) const {
    // Calculate chunk world bounds
    float chunkWorldX = static_cast<float>(chunkX * tileChunkSize);
    float chunkWorldY = static_cast<float>(chunkY * tileChunkSize);
    float chunkWorldRight = chunkWorldX + static_cast<float>(tileChunkSize);
    float chunkWorldBottom = chunkWorldY + static_cast<float>(tileChunkSize);

    // Calculate camera view bounds in world coordinates
    float screenWidth = static_cast<float>(GetScreenWidth());
    float screenHeight = static_cast<float>(GetScreenHeight());

    Vector2 topLeft = GetScreenToWorld2D({ 0, 0 }, camera);
    Vector2 bottomRight = GetScreenToWorld2D({ screenWidth, screenHeight }, camera);

    float cameraLeft = topLeft.x;
    float cameraTop = topLeft.y;
    float cameraRight = bottomRight.x;
    float cameraBottom = bottomRight.y;

    // Check for intersection between chunk bounds and camera view
    bool intersects = !(chunkWorldRight < cameraLeft ||
        chunkWorldX > cameraRight ||
        chunkWorldBottom < cameraTop ||
        chunkWorldY > cameraBottom);

    return intersects;
}

void TilemapRenderer::cleanupChunks() {
    for (auto& layer : tilemapChunks) {
        for (auto& chunk : layer) {
            if (chunk.id > 0) {
                UnloadRenderTexture(chunk);
            }
        }
        layer.clear();
    }
    tilemapChunks.clear();
}