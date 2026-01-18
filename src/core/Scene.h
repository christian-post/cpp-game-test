#pragma once
#include "raylib.h"
#include <string>
#include <any>
#include <unordered_map>
#include <functional>

class Game;

enum inventoryState
{
    NONE,
    OPENING,
    OPENED,
    CLOSING,
    SLIDING_LEFT,
    SLIDING_RIGHT
};

class Scene
{
    // scene base class
public:
    Scene(Game& game, std::string sceneName) : game{ game }, name(std::move(sceneName)) {}
    virtual ~Scene() = default;
    virtual void startup() {}
    virtual void update(float deltaTime) {}
    virtual void draw() {}
    virtual void end() {}
    virtual void onPause() {}

    std::string getName() const { return name; }
    bool isActive() const { return active; }
    bool isPaused() const { return paused; }

    void setActive(bool state) { active = state; }
    void setPaused(bool state) { paused = state; }

    // some flags that allow to delay the activation and deletion
    // until the end of the game loop
    bool isMarkedForDeletion() const { return markedForDeletion; }
    void markForDeletion() { markedForDeletion = true; }
    bool ismarkedForStarting() const { return markedForStarting; }
    void markForStarting() { markedForStarting = true; }

    void setDrawPriority(int prio) { drawPriority = prio; }
    int getDrawPriority() const { return drawPriority; }
    Game& getGame() { return game; }
    inventoryState getState() { return state; }

    // TODO does this functionally overlap with Scene::end()?
    using CompletionCallback = std::function<void()>;
    void setOnComplete(CompletionCallback callback)
    {
        onComplete = callback;
    }
    void complete()
    {
        if (onComplete) 
            onComplete();
    }

    bool markedForStarting = false;

    Music* music = nullptr;
    std::string currentMusicKey;

private:
    std::string name;
    int drawPriority = 0;

protected:
    Game& game;
    bool active = false;
    bool paused = false;
    bool markedForDeletion = false;
    inventoryState state = NONE;
    CompletionCallback onComplete;
};



// Copy paste this when creating a child scene

// -- SceneName.h
//#pragma once
//#include "Scene.h"
//#include <iostream>
//
//class Preload : public Scene 
//{
//public:
//    Preload(Game& game, const std::string& name) : Scene(game, name) {}
//    void startup() override;
//    void update(float deltaTime) override;
//    void draw() override;
//    void end() override;
//};

// -- SceneName.cpp

//#include "Name.h"
//#include "Game.h"
//#include "raylib.h"
//
//void Name::startup()
//{
//}
//
//void Name::update(float deltaTime)
//{
//
//}
//
//void Name::draw()
// {
//
//}
//
//void Name::end()
//{
//
//}