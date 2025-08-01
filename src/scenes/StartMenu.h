#pragma once
#include "Scene.h"
#include "MenuSelect.h"
#include <functional>

class StartMenu : public Scene {
public:
    StartMenu(Game& game, const std::string& name);
    void startup() override;
    void update(float deltaTime) override;
    void draw() override;

private:
    MenuSelect menu;
};