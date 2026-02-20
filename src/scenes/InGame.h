#pragma once
#include "Scene.h"
#include "Sprite.h"
#include "TileMap.h"
#include "Dungeon.h"
#include "Utils.h"
#include "CircleOverlay.h"
#include "TilemapRenderer.h"
#include "CameraController.h"
#include "json.hpp"
#include <memory>
#include <utility>
#include <cstdint>

struct SaveGame;
struct Emitter;
class LuaEventManager;
class EventTriggerManager;

class InGame : public Scene {
public:
    InGame(Game& game, const std::string& name);
    void startup() override;
    void update(float deltaTime) override;
    void draw() override;
    void end() override;
    void onPause() override;

    // Function that handles room transitions
    void loadTilemap();
    Sprite* getSprite(const std::string& name);

    const TileMap* tileMap;
    TilemapRenderer tilemapRenderer;
    CameraController cameraController;
    std::unique_ptr<LuaEventManager> luaEventManager; // Lua API layer
    std::unique_ptr<EventTriggerManager> eventTriggerManager; // processes event triggers for the luaEventManager

    // keep a player variable for direct frequent access
    std::shared_ptr<Sprite> player;
    // pair is (weapon_key, isActive)
    std::array<std::optional<std::pair<std::string, bool>>, 2> currentWeapon = { std::nullopt, std::nullopt };
    // light effects
    // TODO: these need to be dynamic and shouldn't be hard-coded here.
    // > use a vector instead
    Light lights[MAX_LIGHTS] = {
        {{0.0f, 0.0f}, 12.0f},
        {{0.0f, 0.0f}, 12.0f},
        {{0.0f, 0.0f}, 12.0f},
        {{0.0f, 0.0f}, 12.0f},
        {{0.0f, 0.0f}, 12.0f},
        {{0.0f, 0.0f}, 12.0f},
        {{0.0f, 0.0f}, 12.0f},
        {{0.0f, 0.0f}, 12.0f},
        {{0.0f, 0.0f}, 12.0f},
        {{0.0f, 0.0f}, 12.0f}
    };
    static const int lightCount = MAX_LIGHTS;

    bool lampIsOn = false; // dark rooms become lit up when the player equips the lamp

private:
    void spawnWeapon(size_t index);
    void checkRoomTransition();
    void setupEventListeners();
    void handleDeadSprites();
    void loadWorldFromSave(std::shared_ptr<SaveGame> save);
    // input handling
    std::unordered_map<uint32_t, std::function<void()>> buttonCallbacks;
    void setupInputCallbacks();
    // Callback functions
    void onActionButton2();
    void onActionButton3();
    void onInventoryButton();
    void onMenuButton();
    void onDebugMenuButton();
    void onDebugButton1();
    void onDebugButton3();
    void handlePlayerInput(float deltaTime);

    void takeScreenshot();

    bool playerMovementLocked = false; // arrow keys won't move the player
    bool cameraHasBounds = true;  // only for debugging
    bool checkEnemyCount = true; // if true, checks if there are enemies on the map. reset when entering a new map 
    std::shared_ptr<Emitter> wpnHitEffect = nullptr;
};
