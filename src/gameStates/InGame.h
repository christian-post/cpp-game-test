#pragma once

#include "Scene.h"
#include "Sprite.h"
#include "TileMap.h"
#include "CutsceneManager.h"
#include <memory>

class InGame : public Scene {
public:
    InGame(const std::string& name) : Scene(name), tileMap(nullptr), worldHeight(0), worldWidth(0) {}
    void startup(Game* game) override;
    void events(Game* game) override;
    void update(Game* game, float dt) override;
    void draw(Game* game) override;
    void end() override;

    void drawTiles(const TileMap* tileMap, const std::vector<Texture2D>& tiles, int layerIndex);

    std::unordered_map<std::string, std::shared_ptr<Sprite>> spriteMap; // keep named references to certain sprites
    std::shared_ptr<Sprite> player;  // keep a player variable for direct frequent access
    Sprite* getSprite(const std::string& name);
    std::shared_ptr<Sprite> spawnEnemy(Game& game, const std::string& name, int tileX, int tileY); // helper function, TODO: gets replaced later
    const TileMap* tileMap;
    Camera2D camera = {};
    CutsceneManager cutsceneManager;

private:
    int worldWidth;
    int worldHeight;
    int tileSize = 0;

    bool cutsceneFlag = false;
};
