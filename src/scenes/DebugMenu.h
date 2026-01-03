#pragma once
#include "Scene.h"
#include "MenuSelect.h"
#include <iostream>
#include <array>
#include <memory>

enum class MenuType {
    Main,
    RoomSelect,
    LevelSelect,
    ItemCheat,
    NoClip,
    Count
};

class DebugMenu : public Scene {
public:
    DebugMenu(Game& game, const std::string& name);
    void startup() override;
    void update(float deltaTime) override;
    void draw() override;
    void end() override;

private:
    static const size_t numSubMenus = static_cast<size_t>(MenuType::Count);
    MenuType activeMenu = MenuType::Main;
    std::array<std::unique_ptr<MenuSelect>, numSubMenus> menus;

    size_t menuIndex() const { return static_cast<size_t>(activeMenu); }
};