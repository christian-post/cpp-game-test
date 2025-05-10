#pragma once
#include <iostream>
#include <memory>
#include <string>
#include "Scene.h"
#include "raylib.h"
#include "AssetLoader.h"
#include "Sprite.h"
#include "EventManager.h"
#include "json.hpp"


#define DARKBURGUNDY { 20, 0, 8, 255 }

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
    void update(float dt);
    void draw();
    void run();

    // scene management
    void startScene(const std::string& name);
    void stopScene(const std::string& name);
    void sleepScene(const std::string& name);
    void wakeScene(const std::string& name);
    void pauseScene(const std::string& name);
    void resumeScene(const std::string& name);
    void processMarkedScenes();

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

    bool isRunning() const { return running; }
    void end() { running = false; }

    // input management
    uint32_t buttonsPressed;
    uint32_t buttonsDown;

    // game objects
    std::vector<std::unique_ptr<Rectangle>> walls; // everything with static collision
    std::vector<std::shared_ptr<Sprite>> sprites; // dynamic objects

    void killSprite(const std::shared_ptr<Sprite>& sprite);
    void clearSprites(bool clearPersistent = false);
    void processMarkedSprites();

    // store a reference to the player sprite in case a scene other than InGame needs it
    Sprite* getPlayer();

    bool debug = false;

private:
    bool running = true;
    std::unordered_map<std::string, std::unique_ptr<Scene>> scenes; // contains active game scenes
    std::unordered_map<std::string, std::function<std::unique_ptr<Scene>(const std::string&)>> sceneRegistry; // stores scene constructors
    std::unordered_map<std::string, int> scenePriorities; // stores the drawing order (TODO: also control the update order?)
    void setSceneState(const std::string& name, bool active, bool paused);
};