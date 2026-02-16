#pragma once
#include "TileMap.h"
#include "raylib.h"
#include <vector>
#include <memory>

class Game;

struct TilesetData {
    std::string name;
    const Tileset* tileset;
    const Texture2D* texture;
    const Texture2D* mapTexture; // texture for the map that is drawn in the menu
    size_t tilesPerRow;
    int firstGid;
}; // encapsulates Tileset info read from TileMap
// TODO maybe I can put this in TileMap directly

class TilemapRenderer {
public:
    TilemapRenderer(Game& game);
    ~TilemapRenderer();

    // Main interface
    void loadTilemap(const TileMap* tilemap);
    void drawLayer(int layerIndex, const Camera2D& camera);
    void drawAllLayersExceptTop(const Camera2D& camera); // TODO probably obsolete?
    void drawTopLayer(const Camera2D& camera);
    void cleanup();

    // Getters
    size_t getWorldWidth() const { return worldWidth; }
    size_t getWorldHeight() const { return worldHeight; }
    size_t getTileSize() const { return tileSize; }

private:
    Game& game;
    const TileMap* currentTilemap = nullptr;

    // World dimensions
    size_t worldWidth = 0;
    size_t worldHeight = 0;
    size_t tileSize = 0;

    // Chunking system
    size_t tileChunkSize = 256;
    size_t numChunksX = 0;
    size_t numChunksY = 0;
    std::vector<std::vector<RenderTexture2D>> tilemapChunks;

    // Private methods
    void calculateDimensions();
    void generateChunks();
    void generateChunkForLayer(size_t layerIndex, Color clearColor);
    void drawChunks(int layerIndex, const Camera2D& camera);
    bool isChunkVisible(size_t chunkX, size_t chunkY, const Camera2D& camera) const;
    void cleanupChunks();
};