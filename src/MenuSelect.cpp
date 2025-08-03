#include "MenuSelect.h"
#include "Controls.h"

MenuSelect::MenuSelect(Game& game): game{ game }
{}

void MenuSelect::setItems(std::vector<MenuItem> items)
{
    menuItems = items;
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

void MenuSelect::draw() {
    ClearBackground(BLACK);

    uint32_t width = game.gameScreenWidth;
    uint32_t height = game.gameScreenHeight;
    // center the items on the screen
    size_t margin = 20;
    size_t totalHeight = menuItems.size() * (margin + fontsize);
    size_t startY = (height - totalHeight) / 2;

    for (size_t i = 0; i < menuItems.size(); i++) {
        std::string item = menuItems[i].displayName;
        if (i == menuIndex) // highlight the selected item
            item = "< " + item + " >";
        else
            item = "  " + item + "  ";
        int textWidth = MeasureText(item.c_str(), static_cast<int>(fontsize));
        int x = (width - textWidth) / 2;
        DrawText(item.c_str(), x, static_cast<int>(startY + i * (margin + fontsize)), static_cast<int>(fontsize), LIGHTGRAY);
    }
}