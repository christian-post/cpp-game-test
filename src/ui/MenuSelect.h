#pragma once
#include <vector>
#include <string>
#include <functional>
#include "Game.h"

/*
Menu that lets the player select various items
mostly used in dedicated menu scenes
*/
enum class MenuPosition {
    CENTER,
    LEFT,
    RIGHT
};


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
    const size_t getCurrentIndex() const { return menuIndex; }
    const MenuItem getCurrentItem() const { return menuItems[menuIndex]; }
    size_t getFontSize() const { return fontsize; }
    void setFontSize(size_t size) { fontsize = size; }
    size_t getYMargin() const { return yMargin; }
    void setYMargin(size_t size) { yMargin = size; }
    size_t getXMargin() const { return xMargin; }
    void setXMargin(size_t size) { xMargin = size; }
    void setPosition(MenuPosition pos) { position = pos; }
    void restrictHeight(size_t h, size_t offset);
    MenuSelect(Game& game);
    MenuSelect() = default;

private:
    Game& game;
    std::vector<MenuItem> menuItems;
    size_t menuIndex = 0;
    size_t fontsize = 10;
    bool heightLimited = false;
    size_t yOffset = 0; // only used when height is limited
    size_t heightLimit = 0; // only used when height is limited
    size_t yMargin = 20; // space between elements
    size_t xMargin = 40; // only used when aliged LEFT or RIGHT
    MenuPosition position = MenuPosition::CENTER;
    
};