#pragma once
#include <iostream>
#include <memory>
#include <string>
#include "Scene.h"
#include "Controls.h"
#include "raylib.h"
#include "AssetLoader.h"
#include "Sprite.h"
#include "EventManager.h"

class Command;



class Game {
public:
    Game();
    // actual window resolution (can be resized later)
    static constexpr uint32_t windowWidth = 768;
    static constexpr uint32_t windowHeight = 576;
    // in-game resolution (stays constant, gets scaled up to window size)
    uint32_t gameScreenWidth = 256;
    uint32_t gameScreenHeight = 192;

    RenderTexture2D target; // texture surface for the ingame graphics

    // basic game loop
    void events();
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
    void registerScene(const std::string& name) {
        sceneRegistry[name] = [](const std::string& sceneName) {
            return std::make_unique<T>(sceneName);
            };
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

    AssetLoader loader;

    // game objects
    std::vector<std::unique_ptr<Rectangle>> walls; // everything with static collision
    std::vector<std::shared_ptr<Sprite>> sprites; // dynamic objects

    void killSprite(const std::shared_ptr<Sprite>& sprite) {}

    // store a reference to the player sprite in case a scene other than InGame needs it
    Sprite* getPlayer();

    bool debug = false;

private:
    bool running = true;
    std::unordered_map<std::string, std::unique_ptr<Scene>> scenes; // contains active game scenes
    std::unordered_map<std::string, std::function<std::unique_ptr<Scene>(const std::string&)>> sceneRegistry;
    void setSceneState(const std::string& name, bool active, bool paused);
};