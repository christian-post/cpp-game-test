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
    // actual window resolution
    static constexpr int windowWidth = 768;
    static constexpr int windowHeight = 576;
    // in-game resolution
    int gameScreenWidth = 256;
    int gameScreenHeight = 192;

    RenderTexture2D target;

    void events();
    void update(float dt);
    void draw();
    void run();
    // scene managing
    void startScene(const std::string& name);
    void stopScene(const std::string& name);
    void processMarkedScenes();

    void sleepScene(const std::string& name);
    void wakeScene(const std::string& name);
    void pauseScene(const std::string& name);
    void resumeScene(const std::string& name);

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

    EventManager eventManager;

    bool isRunning() const { return running; }
    void end() { running = false; }

    uint32_t buttonsPressed;
    uint32_t buttonsDown;
    AssetLoader loader;

    // game objects
    std::vector<std::unique_ptr<Rectangle>> walls;  // everything with static collision
    std::vector<std::shared_ptr<Sprite>> sprites;

    void killSprite(const std::shared_ptr<Sprite>& sprite) {
        sprites.erase(std::remove(sprites.begin(), sprites.end(), sprite), sprites.end());
    }

    // store a reference to the player sprite in case a scene other than InGame needs it
    Sprite* getPlayer();

    bool debug = false;

private:
    bool running = true;
    std::unordered_map<std::string, std::unique_ptr<Scene>> scenes; // contains active game scenes
    std::unordered_map<std::string, std::function<std::unique_ptr<Scene>(const std::string&)>> sceneRegistry;
    void setSceneState(const std::string& name, bool active, bool paused);
};