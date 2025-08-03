#pragma once
#include <vector>
#include <string>
#include <functional>
#include "Game.h"

/*
Menu that lets the player select various items
mostly used in dedicated menu scenes
*/

struct MenuItem {
    std::string displayName;
    std::function<void()> callback;
};


class MenuSelect {
public:
    // TODO: optimize memory management
    void setItems(std::vector<MenuItem> items);
    void update();
    void draw();
    MenuSelect(Game& game);

private:
    Game& game;
    std::vector<MenuItem> menuItems;
    size_t menuIndex = 0;
    size_t fontsize = 10;
};