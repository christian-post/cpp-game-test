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
    // confirm button activates the callback
    if (game.buttonsPressed & CONTROL_CONFIRM) {
        if (!menuItems[menuIndex].callback) {
            TraceLog(LOG_ERROR, "No Callback set for this menu item.");
            return;
        }
        menuItems[menuIndex].callback();
    }
}

void MenuSelect::draw() {
    ClearBackground(BLACK);

    for (size_t i = 0; i < menuItems.size(); i++) {
        // TODO: could this be done with less string copying?
        std::string item = menuItems[i].displayName;
        if (i == menuIndex)
            item = "< " + item + " >";
        else
            item = "  " + item + "  ";
        DrawText(item.c_str(), 50, 50 + i * 30, 10, LIGHTGRAY);
    }
}