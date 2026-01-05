#pragma once
#include "Game.h"
#include <vector>
#include <string>
#include <functional>

/*
Menu that lets the player select various items
mostly used in dedicated menu scenes
*/
enum class MenuPosition
{
    CENTER,
    LEFT,
    RIGHT
};

enum class MenuItemType
{
    Action,   // Normal callback on confirm
    Cycle,    // Cycle through string options
    Number,    // Increment/decrement number
    KeyBind   // Capture key/button press
};

struct MenuItem
{
    std::string displayName;
    MenuItemType type = MenuItemType::Action;
    std::function<void()> callback;  // for Action type

    // For Cycle type
    std::vector<std::string> options;
    size_t currentOption = 0;
    std::function<void(size_t)> cycleCallback;  // receives selected index

    // For Number type
    int numberValue = 0;
    int minValue = 0;
    int maxValue = 100;
    int step = 1;
    std::function<void(int)> numberCallback;  // receives current value
    bool isActive = false;  // whether we're editing the number

    // For KeyBind type
    int boundKey = 0;  // the currently bound keycode
    std::function<void(int)> keyBindCallback;  // receives the new keycode
};


class MenuSelect
{
public:
    // TODO: optimize memory management
    void update();
    void draw();
    void addItem(MenuItem item);
    void setItems(std::vector<MenuItem> items); // sets all MenuItems at once
    void updateItemText(size_t index, std::string txt); // just changes the text, TODO: do I also need to update the callback?
    size_t getCurrentIndex() const { return menuIndex; }
    MenuItem getCurrentItem() const { return menuItems[menuIndex]; }
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
    size_t yMargin = 12; // space between elements
    size_t xMargin = 40; // only used when aliged LEFT or RIGHT
    MenuPosition position = MenuPosition::CENTER;
};