#pragma once
#include "Scene.h"
#include "MenuSelect.h"
#include <iostream>
#include <array>
#include <memory>

class DebugMenu : public Scene {
public:
    DebugMenu(Game& game, const std::string& name);
    void startup() override;
    void update(float deltaTime) override;
    void draw() override;
    void end() override;

private:
    static const size_t numSubMenus = 3;
    size_t activeMenu = 0;
    std::array<std::unique_ptr<MenuSelect>, numSubMenus> menus;
};