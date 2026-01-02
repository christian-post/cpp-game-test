#include "MenuSelect.h"
#include "Controls.h"
#include <cstdint>


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

//void MenuSelect::addItem(const std::string& displayName, std::function<void()> callback)
//{
//    MenuItem item;
//    item.displayName = displayName;
//    item.type = MenuItemType::Action;
//    item.callback = callback;
//    menuItems.push_back(item);
//}

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

    MenuItem& currentItem = menuItems[menuIndex];

    // Handle Number type when active (editing mode)
    if (currentItem.type == MenuItemType::Number && currentItem.isActive) {
        // In edit mode, up/down change the value
        if (game.buttonsPressed & CONTROL_UP) {
            currentItem.numberValue = std::min(currentItem.numberValue + currentItem.step, currentItem.maxValue);
            if (currentItem.numberCallback)
                currentItem.numberCallback(currentItem.numberValue);
            game.playSound("menuCursor");
        }
        if (game.buttonsPressed & CONTROL_DOWN) {
            currentItem.numberValue = std::max(currentItem.numberValue - currentItem.step, currentItem.minValue);
            if (currentItem.numberCallback)
                currentItem.numberCallback(currentItem.numberValue);
            game.playSound("menuCursor");
        }
        // Confirm or cancel exits edit mode
        if (game.buttonsPressed & (CONTROL_CONFIRM | CONTROL_CANCEL)) {
            currentItem.isActive = false;
            game.playSound("menuCursor");
        }
        return;
    }

    // Normal navigation with up/down
    if (game.buttonsPressed & CONTROL_DOWN) {
        menuIndex = (menuIndex + 1) % menuItems.size();
        game.playSound("menuCursor");
    }
    if (game.buttonsPressed & CONTROL_UP) {
        menuIndex = (menuIndex + menuItems.size() - 1) % menuItems.size();
        game.playSound("menuCursor");
    }

    // Handle left/right for Cycle type
    if (currentItem.type == MenuItemType::Cycle) {
        if (game.buttonsPressed & CONTROL_LEFT) {
            currentItem.currentOption = (currentItem.currentOption + currentItem.options.size() - 1) % currentItem.options.size();
            if (currentItem.cycleCallback)
                currentItem.cycleCallback(currentItem.currentOption);
            game.playSound("menuCursor");
        }
        if (game.buttonsPressed & CONTROL_RIGHT) {
            currentItem.currentOption = (currentItem.currentOption + 1) % currentItem.options.size();
            if (currentItem.cycleCallback)
                currentItem.cycleCallback(currentItem.currentOption);
            game.playSound("menuCursor");
        }
    }

    // Handle confirm button based on type
    if (game.buttonsPressed & (CONTROL_CONFIRM | CONTROL_ACTION1)) {
        switch (currentItem.type) {
        case MenuItemType::Action:
            if (!currentItem.callback) {
                TraceLog(LOG_ERROR, "No Callback set for this menu item.");
                return;
            }
            currentItem.callback();
            break;
        case MenuItemType::Cycle:
            // Cycle to next option on confirm
            currentItem.currentOption = (currentItem.currentOption + 1) % currentItem.options.size();
            if (currentItem.cycleCallback)
                currentItem.cycleCallback(currentItem.currentOption);
            game.playSound("menuCursor");
            break;
        case MenuItemType::Number:
            // Enter edit mode
            currentItem.isActive = true;
            game.playSound("menuCursor");
            break;
        }
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
        if (startY + menuIndex * (yMargin + fontsize) > height - itemScrollStart * rowHeight)
            startY -= ((menuIndex + 1) - (itemsOnScreen - itemScrollStart)) * rowHeight; // shift top of the list. e.g., if 10 items fit on the screen and the selected index is 12, shift up by 3
    }

    for (size_t i = 0; i < menuItems.size(); i++) {
        const MenuItem& item = menuItems[i];
        std::string displayText = item.displayName;

        // Append current value/option for Cycle and Number types
        if (item.type == MenuItemType::Cycle && !item.options.empty()) {
            displayText += ": < " + item.options[item.currentOption] + " >";
        }
        else if (item.type == MenuItemType::Number) {
            displayText += ": " + std::to_string(item.numberValue);
            if (item.isActive)
                displayText += " [EDIT]";
        }

        Color color = DARKGRAY;
        if (i == menuIndex)
            color = LIGHTGRAY;

        int textWidth = MeasureText(displayText.c_str(), static_cast<int>(fontsize));
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
        DrawText(displayText.c_str(), x, static_cast<int>(startY + i * rowHeight), static_cast<int>(fontsize), color);
    }
}