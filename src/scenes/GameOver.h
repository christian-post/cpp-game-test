#pragma once

#include "Scene.h"
#include "MenuSelect.h"
#include <iostream>
#include <string>

class GameOver : public Scene {
public:
    GameOver(Game& game, const std::string& name) : Scene(game, name), menu(MenuSelect(game)) {}
    void startup() override;
    void update(float deltaTime) override;
    void draw() override;
private:
    bool showText1 = false;
    bool showMenu = false;
    MenuSelect menu;
};
