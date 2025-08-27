#pragma once
#include "Scene.h"
#include "MenuSelect.h"
#include <iostream>

class LoadSavegameMenu : public Scene {
public:
    LoadSavegameMenu(Game& game, const std::string& name);
    void startup() override;
    void update(float deltaTime) override;
    void draw() override;

private:
    MenuSelect menu;
    std::vector<std::string> fileInfo;
};