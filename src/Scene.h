#pragma once
#include <string>

class Game;

class Scene {
    // scene base class
public:
    Scene(std::string sceneName) : name(std::move(sceneName)) {}
    virtual ~Scene() = default;
    virtual void startup(Game* game) {}
    virtual void events(Game* game) {}
    virtual void update(Game* game, float dt) {}
    virtual void draw(Game* game) {}
    virtual void end() {}

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

    bool markedForStarting = false;

private:
    std::string name;

protected:
    bool active = false;
    bool paused = false;
    bool markedForDeletion = false;
};



// Copy paste this when creating a child scene

// -- SceneName.h
// 
//#include "Scene.h"
//#include <iostream>
//
//class Preload : public Scene {
//public:
//    Preload(const std::string& name) : Scene(name) {}
//    void startup(Game* game) override;
//    void events(Game* game, int event) override;
//    void update(Game* game, float dt) override;
//    void draw(Game* game) override;
//    void end() override;
//};

// -- SceneName.cpp

//#include "Name.h"
//#include "Game.h"
//#include "raylib.h"
//
//void Name::startup(Game* game) {
//}
//
//void Name::events(Game* game) {
//    // TODO 
//}
//
//void Name::update(Game* game, float dt) {
//
//}
//
//void Name::draw(Game* game) {
//
//}
//
//void Name::end() {
//
//}