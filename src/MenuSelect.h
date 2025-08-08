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
    void setItems(std::vector<MenuItem> items); // sets all MenuItems at once
    void addItem(MenuItem item);
    void update();
    void draw();
    size_t getFontSize() const { return fontsize; };
    void setFontSize(size_t size) { fontsize = size; };
    size_t getMargin() const { return margin; };
    void setMargin(size_t size) { margin = size; };
    MenuSelect(Game& game);

private:
    Game& game;
    std::vector<MenuItem> menuItems;
    size_t menuIndex = 0;
    size_t fontsize = 10;
    size_t margin = 20;
};