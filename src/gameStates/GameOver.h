#pragma once

#include "Scene.h"
#include <iostream>
#include <string>

class GameOver : public Scene {
public:
    GameOver(const std::string& name) : Scene(name) {}
    void startup(Game* game) override;
    void events(Game* game) override;
    void update(Game* game, float dt) override;
    void draw(Game* game) override;
private:
    bool showText1 = false;
    bool showText2 = false;
};
