#include <cstdint>
#include "MenuSelect.h"
#include "Controls.h"


MenuSelect::MenuSelect(Game& game): game{ game }
{}

void MenuSelect::setItems(std::vector<MenuItem> items)
{
    menuItems = items;
}

void MenuSelect::addItem(MenuItem item)
{
    menuItems.push_back(item);
}

void MenuSelect::updateItemText(size_t index, std::string txt)
{
    menuItems[index].displayName = txt;
}

void MenuSelect::restrictHeight(size_t h, size_t offset)
{
    heightLimited = true;
    heightLimit = h;
    yOffset = offset;
}

void MenuSelect::update()
{
    if (menuItems.size() == 0)
        return;
    // move the cursor
    if (game.buttonsPressed & CONTROL_DOWN) {
        menuIndex = (menuIndex + 1) % menuItems.size();
        game.playSound("menuCursor");
    }
    if (game.buttonsPressed & CONTROL_UP) {
        menuIndex = (menuIndex + menuItems.size() - 1) % menuItems.size();
        game.playSound("menuCursor");
    }
    // button activates the callback
    if (game.buttonsPressed & (CONTROL_CONFIRM | CONTROL_ACTION1)) {
        if (!menuItems[menuIndex].callback) {
            TraceLog(LOG_ERROR, "No Callback set for this menu item.");
            return;
        }
        menuItems[menuIndex].callback();
    }
}

void MenuSelect::draw() 
{
    ClearBackground(BLACK);

    uint32_t width = game.gameScreenWidth;
    uint32_t height = game.gameScreenHeight;
    size_t rowHeight = yMargin + fontsize;
    // center the items on the screen
    size_t totalHeight = menuItems.size() * rowHeight;
    size_t startY = 0;
    if (totalHeight <= height)
        if (heightLimited) {
            height = heightLimit;
            startY += yOffset + (height - totalHeight) / 2;
        }
        else {
            startY = (height - totalHeight) / 2;
        }
    else {
        // the menu does not fit on the screen
        startY = yMargin;
        // check if the currently selected index points to an item below the screen
        size_t itemsOnScreen = height / rowHeight;
        size_t itemScrollStart = itemsOnScreen / 2; // how many items from the bottem the list starts to scroll
        if (startY + menuIndex * (yMargin + fontsize) > height - itemScrollStart * rowHeight) {
            startY -= ((menuIndex + 1) - (itemsOnScreen - itemScrollStart)) * rowHeight; // shift top of the list. e.g., if 10 items fit on the screen and the selected index is 12, shift up by 3
        }
    }

    for (size_t i = 0; i < menuItems.size(); i++) {
        std::string item = menuItems[i].displayName;
        Color color = DARKGRAY;
        if (i == menuIndex) {
            color = LIGHTGRAY;
        }
        int textWidth = MeasureText(item.c_str(), static_cast<int>(fontsize));
        // menu x position depending on the setting
        int x = 0;
        switch (position) {
        case MenuPosition::CENTER:
            x = (width - textWidth) / 2;
            break;
        case MenuPosition::LEFT:
            x = xMargin;
            break;
        case MenuPosition::RIGHT:
            // TODO measure the size of the largest item in setItems and addItem
            break;
        }
        DrawText(item.c_str(), x, static_cast<int>(startY + i * rowHeight), static_cast<int>(fontsize), color);
    }
}