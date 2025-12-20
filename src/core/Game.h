#pragma once
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <stdexcept>

#include "json.hpp"
#include "raylib.h"

#include "Scene.h"
#include "AssetLoader.h"
#include "Sprite.h"
#include "EventManager.h"
#include "WindowEventHandler.h"
#include "CutsceneManager.h"
#include "InventoryManager.h"
#include "Dungeon.h"
#include "Savegame.h"

#define DARKBURGUNDY { 20, 0, 8, 255 }
#define LIGHTBURGUNDY { 40, 0, 16, 255 }


class Command;
struct CollisionObject;
struct Emitter;

class Game {
private:
    nlohmann::json* settings = nullptr;

public:
    Game();
    ~Game();
    // in-game resolution (stays constant, gets scaled up to window size)
    uint32_t gameScreenWidth = 256;
    uint32_t gameScreenHeight = 192;
    bool isFullscreen = false;  // start in windowed mode, TODO put this in settings.json
    int lastWindowX = 0;
    int lastWindowY = 0; // store the position in windowed mode to return to after fullscreen

    RenderTexture2D target; // texture surface for the ingame graphics

    AssetLoader loader;
    nlohmann::json getSetting(const std::string& key, nlohmann::json defaultValue = nlohmann::json()) const;
    void writeSetting(const std::string& key, nlohmann::json value);
    void saveSettings();

    // basic game loop
    void update(float deltaTime);
    void playMusic();
    void draw();
    void run();

    // scene management
    void startScene(const std::string& name); // calls Scene.startup()
    void stopScene(const std::string& name); // calls Scene.stop()
    void sleepScene(const std::string& name); // sets scene inactive (no update, draw or sound)
    void wakeScene(const std::string& name); // activates an inactive scene
    void pauseScene(const std::string& name); // stops Scene.update(), but still draws and plays sound
    void resumeScene(const std::string& name); // unpauses a pause scene
    void processMarkedScenes();
    void resetScenes();

    template <typename T>
    void registerScene(const std::string& name, int priority = 0) {
        sceneRegistry[name] = [this](const std::string& sceneName) {
            return std::make_unique<T>(*this, sceneName);
            };
        scenePriorities[name] = priority;
    }

    Scene* getScene(const std::string& name) {
        auto it = scenes.find(name);
        return (it != scenes.end()) ? it->second.get() : nullptr;
    }

    using SceneCallback = std::function<void()>;
    void setOnSceneComplete(const std::string& sceneName, SceneCallback callback) {
        sceneCallbacks[sceneName] = callback;
    }

    EventManager eventManager; // event handling
    WindowEventHandler windowEvents; // processes raylib window changes specifically
    CutsceneManager cutsceneManager;
    InventoryManager inventory;

    bool isRunning() const { return running; }
    bool isRestartRequested() const { return restartRequested; }
    void end() { running = false; }
    void restart();
    void cleanup(); // cleans up after the game loop ended

    // saving and loading the game state
    void save(std::string& filename);
    void load(std::string& filename);
    std::shared_ptr<SaveGame> getSaveData();

    // input management
    uint32_t buttonsPressed;
    uint32_t buttonsDown;

    // game objects
    std::vector<std::unique_ptr<CollisionObject>> walls; // everything with static collision
    std::vector<std::shared_ptr<Sprite>> sprites; // dynamic objects
    std::unordered_map<std::string, std::shared_ptr<Sprite>> spriteMap; // keep named references to certain sprites
    std::vector<std::shared_ptr<Emitter>> emitters;  // particle emitters
    std::shared_ptr<Sprite> createSprite(std::string spriteName, Rectangle& rect); // TODO: return a shared pointer, or a reference to the sprite?

    // Dungeon management
    std::unique_ptr<Dungeon> currentDungeon = nullptr; 
    void createDungeon(size_t roomsW, size_t roomsH, size_t numLevels);

    // World bounds (set by InGame when loading tilemap)
    // TODO these need to be here (and not in CameraController.h) because behaviors need to have access to this information 
    float worldWidth = 0.0f;
    float worldHeight = 0.0f;

    void setWorldBounds(float width, float height) {
        worldWidth = width;
        worldHeight = height;
    }

    bool isInWorldBounds(const Vector2& pos) const {
        return pos.x >= 0.0f && pos.x < worldWidth &&
            pos.y >= 0.0f && pos.y < worldHeight;
    }

    bool isInWorldBounds(const Rectangle& rect) const {
        return rect.x >= 0.0f &&
            rect.y >= 0.0f &&
            rect.x + rect.width <= worldWidth &&
            rect.y + rect.height <= worldHeight;
    }

    // sprite management
    void killSprite(const std::shared_ptr<Sprite>& sprite);
    void clearSprites(bool clearPersistent = false);
    void processMarkedSprites();
    // other game objects
    void clearEmitters(bool clearPersistent = false);

    Sprite* getPlayer(); // store a reference to the player sprite in case a scene other than InGame needs it

    void playSound(const std::string& key);

    bool soundOn = true; // all sound, overwrites the other two
    bool musicOn = true;
    bool sfxOn = true;

    bool debug = false; // controls the debug menu and functions

private:
    bool running = true;
    bool restartRequested = false; // triggers a restart (Game instance is recreated)
    std::unordered_map<std::string, std::unique_ptr<Scene>> scenes; // contains active game scenes
    std::unordered_map<std::string, std::function<std::unique_ptr<Scene>(const std::string&)>> sceneRegistry; // stores scene constructors
    std::unordered_map<std::string, int> scenePriorities; // stores the drawing order (TODO: also control the update order?)
    void setSceneState(const std::string& name, bool active, bool paused);
    std::unordered_map<std::string, SceneCallback> sceneCallbacks; // temporarily hold scene completion callbacks for scenes that haven't been started yet
    std::vector<std::shared_ptr<Sprite>> spritesToAdd; // stores the sprites that are later added to the actual sprites vector (prevents changing the vector during the update loop)
    std::shared_ptr<SaveGame> savegame = nullptr; // store save data
    void toggleFullscreen();
};