#pragma once

#include "Scene.h"
#include "Sprite.h"
#include "TileMap.h"
#include "CutsceneManager.h"
#include <memory>

class InGame : public Scene {
public:
    InGame(Game& game, const std::string& name) : Scene(game, name), tileMap(nullptr), worldHeight(0), worldWidth(0) {}
    void startup() override;
    void events(const std::unordered_map<std::string, std::any>& events) override;
    void update(float dt) override;
    void draw() override;
    void end() override;

    void drawTiles(const TileMap* tileMap, const std::vector<Texture2D>& tiles, int layerIndex);

    std::unordered_map<std::string, std::shared_ptr<Sprite>> spriteMap; // keep named references to certain sprites
    std::shared_ptr<Sprite> player;  // keep a player variable for direct frequent access
    Sprite* getSprite(const std::string& name);
    std::shared_ptr<Sprite> spawnEnemy(const std::string& name, int tileX, int tileY); // helper function, TODO: gets replaced later
    std::optional<std::string> currentWeapon = std::nullopt;
    const TileMap* tileMap;
    void loadTilemap(const std::string& name);
    Camera2D camera = {};
    CutsceneManager cutsceneManager;

    void resolveAxisX(const std::shared_ptr<Sprite>& sprite, const Rectangle& obstacle);
    void resolveAxisY(const std::shared_ptr<Sprite>& sprite, const Rectangle& obstacle);

private:
    int worldWidth;
    int worldHeight;
    int tileSize = 0;
};
