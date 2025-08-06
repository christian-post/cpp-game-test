#pragma once
#include "Scene.h"
#include "MenuSelect.h"

class SelectMenu : public Scene {
public:
    SelectMenu(Game& game, const std::string& name);
    void startup() override;
    void update(float deltaTime) override;
    void draw() override;

private:
    MenuSelect menu;
};