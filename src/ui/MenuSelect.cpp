#include "MenuSelect.h"
#include "Controls.h"
#include <cstdint>


static std::string getKeyName(int keycode)
{
    // values < 32 are gamepad buttons
    if (keycode < 32)
    {
        static const std::unordered_map<int, std::string> gamepadNames = {
            {GAMEPAD_BUTTON_LEFT_FACE_UP, "D-Pad Up"},
            {GAMEPAD_BUTTON_LEFT_FACE_RIGHT, "D-Pad Right"},
            {GAMEPAD_BUTTON_LEFT_FACE_DOWN, "D-Pad Down"},
            {GAMEPAD_BUTTON_LEFT_FACE_LEFT, "D-Pad Left"},
            {GAMEPAD_BUTTON_RIGHT_FACE_UP, "Y"},
            {GAMEPAD_BUTTON_RIGHT_FACE_RIGHT, "B"},
            {GAMEPAD_BUTTON_RIGHT_FACE_DOWN, "A"},
            {GAMEPAD_BUTTON_RIGHT_FACE_LEFT, "X"},
            {GAMEPAD_BUTTON_LEFT_TRIGGER_1, "LB"},
            {GAMEPAD_BUTTON_LEFT_TRIGGER_2, "LT"},
            {GAMEPAD_BUTTON_RIGHT_TRIGGER_1, "RB"},
            {GAMEPAD_BUTTON_RIGHT_TRIGGER_2, "RT"},
            {GAMEPAD_BUTTON_MIDDLE_LEFT, "Select"},
            {GAMEPAD_BUTTON_MIDDLE, "Home"},
            {GAMEPAD_BUTTON_MIDDLE_RIGHT, "Start"},
            {GAMEPAD_BUTTON_LEFT_THUMB, "L-Stick"},
            {GAMEPAD_BUTTON_RIGHT_THUMB, "R-Stick"}
        };

        auto it = gamepadNames.find(keycode);
        if (it != gamepadNames.end())
            return it->second;

        return "Button" + std::to_string(keycode);
    }

    // keyboard keys (>= 32)
    static const std::unordered_map<int, std::string> keyNames = {
        // letters
        {KEY_A, "A"}, {KEY_B, "B"}, {KEY_C, "C"}, {KEY_D, "D"}, {KEY_E, "E"},
        {KEY_F, "F"}, {KEY_G, "G"}, {KEY_H, "H"}, {KEY_I, "I"}, {KEY_J, "J"},
        {KEY_K, "K"}, {KEY_L, "L"}, {KEY_M, "M"}, {KEY_N, "N"}, {KEY_O, "O"},
        {KEY_P, "P"}, {KEY_Q, "Q"}, {KEY_R, "R"}, {KEY_S, "S"}, {KEY_T, "T"},
        {KEY_U, "U"}, {KEY_V, "V"}, {KEY_W, "W"}, {KEY_X, "X"}, {KEY_Y, "Y"},
        {KEY_Z, "Z"},
        // numbers
        {KEY_ZERO, "0"}, {KEY_ONE, "1"}, {KEY_TWO, "2"}, {KEY_THREE, "3"},
        {KEY_FOUR, "4"}, {KEY_FIVE, "5"}, {KEY_SIX, "6"}, {KEY_SEVEN, "7"},
        {KEY_EIGHT, "8"}, {KEY_NINE, "9"},
        // special keys
        {KEY_SPACE, "Space"}, {KEY_ENTER, "Enter"}, {KEY_BACKSPACE, "Backspace"},
        {KEY_TAB, "Tab"}, {KEY_ESCAPE, "Escape"},
        // arrow keys
        {KEY_UP, "Up"}, {KEY_DOWN, "Down"}, {KEY_LEFT, "Left"}, {KEY_RIGHT, "Right"},
        // function keys
        {KEY_F1, "F1"}, {KEY_F2, "F2"}, {KEY_F3, "F3"}, {KEY_F4, "F4"},
        {KEY_F5, "F5"}, {KEY_F6, "F6"}, {KEY_F7, "F7"}, {KEY_F8, "F8"},
        {KEY_F9, "F9"}, {KEY_F10, "F10"}, {KEY_F11, "F11"}, {KEY_F12, "F12"},
        // modifiers
        {KEY_LEFT_SHIFT, "L-Shift"}, {KEY_RIGHT_SHIFT, "R-Shift"},
        {KEY_LEFT_CONTROL, "L-Ctrl"}, {KEY_RIGHT_CONTROL, "R-Ctrl"},
        {KEY_LEFT_ALT, "L-Alt"}, {KEY_RIGHT_ALT, "R-Alt"},
        // numpad
        {KEY_KP_0, "Num0"}, {KEY_KP_1, "Num1"}, {KEY_KP_2, "Num2"}, {KEY_KP_3, "Num3"},
        {KEY_KP_4, "Num4"}, {KEY_KP_5, "Num5"}, {KEY_KP_6, "Num6"}, {KEY_KP_7, "Num7"},
        {KEY_KP_8, "Num8"}, {KEY_KP_9, "Num9"},
        {KEY_KP_DECIMAL, "Num."}, {KEY_KP_DIVIDE, "Num/"}, {KEY_KP_MULTIPLY, "Num*"},
        {KEY_KP_SUBTRACT, "Num-"}, {KEY_KP_ADD, "Num+"},
        // other common keys
        {KEY_INSERT, "Insert"}, {KEY_DELETE, "Delete"}, {KEY_HOME, "Home"},
        {KEY_END, "End"}, {KEY_PAGE_UP, "PgUp"}, {KEY_PAGE_DOWN, "PgDn"},
        {KEY_CAPS_LOCK, "Caps"}, {KEY_SCROLL_LOCK, "Scroll"}, {KEY_NUM_LOCK, "NumLock"},
        {KEY_PRINT_SCREEN, "PrtSc"}, {KEY_PAUSE, "Pause"}
    };

    auto it = keyNames.find(keycode);
    if (it != keyNames.end())
        return it->second;

    return "Key" + std::to_string(keycode);
}


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

    MenuItem& currentItem = menuItems[menuIndex];

    // handle keybind type when active (waiting for key or button press)
    if (currentItem.type == MenuItemType::KeyBind && currentItem.isActive)
    {
        // wait for any key press
        int key = GetKeyPressed();
        if (key != 0)
        {
            currentItem.boundKey = key;
            currentItem.isActive = false;
            if (currentItem.keyBindCallback)
                currentItem.keyBindCallback(key);
            game.playSound("menuCursor");
            return;
        }

        // also check for gamepad button presses
        if (IsGamepadAvailable(0))
        {
            for (int button = 0; button <= GAMEPAD_BUTTON_RIGHT_THUMB; button++)
            {
                if (IsGamepadButtonPressed(0, button))
                {
                    currentItem.boundKey = button;
                    currentItem.isActive = false;
                    if (currentItem.keyBindCallback)
                        currentItem.keyBindCallback(button);
                    game.playSound("menuCursor");
                    return;
                }
            }
        }

        // cancel on escape or cancel button
        if (game.buttonsPressed & CONTROL_CANCEL)
        {
            currentItem.isActive = false;
            game.playSound("menuCursor");
        }
        return;
    }

    // Handle Number type when active (editing mode)
    if (currentItem.type == MenuItemType::Number && currentItem.isActive)
    {
        // In edit mode, up/down or left/right to change the value
        if (game.buttonsPressed & (CONTROL_UP | CONTROL_LEFT))
        {
            currentItem.numberValue = std::min(currentItem.numberValue + currentItem.step, currentItem.maxValue);
            if (currentItem.numberCallback)
                currentItem.numberCallback(currentItem.numberValue);
            game.playSound("menuCursor");
        }
        if (game.buttonsPressed & (CONTROL_DOWN | CONTROL_RIGHT))
        {
            currentItem.numberValue = std::max(currentItem.numberValue - currentItem.step, currentItem.minValue);
            if (currentItem.numberCallback)
                currentItem.numberCallback(currentItem.numberValue);
            game.playSound("menuCursor");
        }
        // button exits edit mode
        if (game.buttonsPressed & (CONTROL_ACTION1 | CONTROL_CONFIRM | CONTROL_CANCEL))
        {
            currentItem.isActive = false;
            game.playSound("menuCursor");
        }
        return;
    }

    // Normal navigation with up/down
    if (game.buttonsPressed & CONTROL_DOWN)
    {
        menuIndex = (menuIndex + 1) % menuItems.size();
        game.playSound("menuCursor");
    }
    if (game.buttonsPressed & CONTROL_UP)
    {
        menuIndex = (menuIndex + menuItems.size() - 1) % menuItems.size();
        game.playSound("menuCursor");
    }

    // Handle left/right for Cycle type
    if (currentItem.type == MenuItemType::Cycle)
    {
        if (game.buttonsPressed & CONTROL_LEFT)
        {
            currentItem.currentOption = (currentItem.currentOption + currentItem.options.size() - 1) % currentItem.options.size();
            if (currentItem.cycleCallback)
                currentItem.cycleCallback(currentItem.currentOption);
            game.playSound("menuCursor");
        }
        if (game.buttonsPressed & CONTROL_RIGHT)
        {
            currentItem.currentOption = (currentItem.currentOption + 1) % currentItem.options.size();
            if (currentItem.cycleCallback)
                currentItem.cycleCallback(currentItem.currentOption);
            game.playSound("menuCursor");
        }
    }

    // Handle confirm button based on type
    if (game.buttonsPressed & (CONTROL_CONFIRM | CONTROL_ACTION1))
    {
        switch (currentItem.type)
        {
        case MenuItemType::Action:
            if (!currentItem.callback)
            {
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
        case MenuItemType::KeyBind:
            // enter key capture mode
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
    {
        if (heightLimited)
        {
            height = heightLimit;
            startY += yOffset + (height - totalHeight) / 2;
        }
        else
        {
            startY = (height - totalHeight) / 2;
        }
    }
    else
    {
        // the menu does not fit on the screen
        startY = yMargin;
        // check if the currently selected index points to an item below the screen
        size_t itemsOnScreen = height / rowHeight;
        size_t itemScrollStart = itemsOnScreen / 2; // how many items from the bottem the list starts to scroll
        if (startY + menuIndex * (yMargin + fontsize) > height - itemScrollStart * rowHeight)
            startY -= ((menuIndex + 1) - (itemsOnScreen - itemScrollStart)) * rowHeight; // shift top of the list. e.g., if 10 items fit on the screen and the selected index is 12, shift up by 3
    }

    for (size_t i = 0; i < menuItems.size(); i++)
    {
        const MenuItem& item = menuItems[i];
        std::string displayText = item.displayName;

        // Append current value/option for Cycle and Number types
        if (item.type == MenuItemType::Cycle && !item.options.empty()) 
        {
            displayText += ": < " + item.options[item.currentOption] + " >";
        }
        else if (item.type == MenuItemType::Number)
        {
            displayText += ": " + std::to_string(item.numberValue);
            if (item.isActive)
                displayText += " [EDIT]";
        }
        else if (item.type == MenuItemType::KeyBind)
        {
            if (item.isActive)
            {
                displayText += ": [PRESS KEY]";
            }
            else if (item.boundKey != 0)
            {
                displayText += ": " + getKeyName(item.boundKey);
            }
            else
            {
                displayText += ": [UNBOUND]";
            }
        }

        Color color = DARKGRAY;
        if (i == menuIndex)
            color = LIGHTGRAY;

        int textWidth = MeasureText(displayText.c_str(), static_cast<int>(fontsize));
        // menu x position depending on the setting
        int x = 0;
        switch (position)
        {
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