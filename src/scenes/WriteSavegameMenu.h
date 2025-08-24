#pragma once
#include "Scene.h"
#include "MenuSelect.h"
#include <iostream>

class WriteSavegameMenu : public Scene {
public:
    WriteSavegameMenu(Game& game, const std::string& name);
    void startup() override;
    void update(float deltaTime) override;
    void draw() override;
    void end() override;

private:
    MenuSelect menu;
};