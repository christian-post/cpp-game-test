#pragma once
#include <iostream>
#include <memory>
#include <string>
#include "Scene.h"
#include "raylib.h"
#include "AssetLoader.h"
#include "Sprite.h"
#include "EventManager.h"
#include "CutsceneManager.h"
#include "InventoryManager.h"
#include "Emitter.h"
#include "Dungeon.h"
#include "Savegame.h"
#include "json.hpp"
#include <stdexcept>

#define DARKBURGUNDY { 20, 0, 8, 255 }
#define LIGHTBURGUNDY { 40, 0, 16, 255 }


// Debug flags
//#define TEST_ROOM



class Command;

class Game {
private:
    const nlohmann::json* settings = nullptr;

public:
    Game();
    // in-game resolution (stays constant, gets scaled up to window size)
    uint32_t gameScreenWidth = 256;
    uint32_t gameScreenHeight = 192;

    RenderTexture2D target; // texture surface for the ingame graphics

    AssetLoader loader;
    const nlohmann::json& getSetting(const std::string& key) const;

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

    EventManager eventManager; // event handling
    CutsceneManager cutsceneManager;
    InventoryManager inventory;

    bool isRunning() const { return running; }
    bool isRestartRequested() const { return restartRequested; }
    void end() { running = false; }
    void restart() {
        running = false;
        restartRequested = true; 
    }
    // saving and loading the game state
    void save();
    void load();
    std::shared_ptr<SaveGame> getSaveData();

    // input management
    uint32_t buttonsPressed;
    uint32_t buttonsDown;

    // game objects
    std::vector<std::unique_ptr<Rectangle>> walls; // everything with static collision
    std::vector<std::shared_ptr<Sprite>> sprites; // dynamic objects
    std::vector<Emitter> emitters; // particle emitters
    std::shared_ptr<Sprite> createSprite(std::string spriteName, Rectangle& rect); // TODO: or return a reference to the sprite?

    // Dungeon management
    std::unique_ptr<Dungeon> currentDungeon = nullptr; 
    void createDungeon(size_t roomsW, size_t roomsH);

    void killSprite(const std::shared_ptr<Sprite>& sprite);
    void clearSprites(bool clearPersistent = false);
    void processMarkedSprites();

    Sprite* getPlayer(); // store a reference to the player sprite in case a scene other than InGame needs it

    void playSound(const std::string& key);
    bool soundOn = false; // all sound, overwrites the other two
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
    std::vector<std::shared_ptr<Sprite>> spritesToAdd; // stores the sprites that are later added to the actual sprites vector (prevents changing the vector during the update loop)
    std::shared_ptr<SaveGame> savegame = nullptr; // store save data
};