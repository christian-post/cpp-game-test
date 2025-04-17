#pragma once

#include "Scene.h"
#include <iostream>
#include <string>

class GameOver : public Scene {
public:
    GameOver(Game& game, const std::string& name) : Scene(game, name) {}
    void startup() override;
    void update(float dt) override;
    void draw() override;
private:
    bool showText1 = false;
    bool showText2 = false;
};
